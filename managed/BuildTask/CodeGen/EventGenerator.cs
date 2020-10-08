using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using QmlBuildTasks.Introspection;
using static Microsoft.CodeAnalysis.CSharp.SyntaxFactory;


namespace QmlBuildTasks.CodeGen
{
    public class EventGenerator
    {
        private static TypeSyntax InferredType => IdentifierName("var");

        private readonly Options _options;

        private readonly GeneratorSupport _support;

        public EventGenerator(Options options, GeneratorSupport support)
        {
            _options = options;
            _support = support;
        }

        public string GetSignalIndexField(MethodInfo signal)
        {
            return "_" + signal.Name + "SignalIndex";
        }

        public IEnumerable<MemberDeclarationSyntax> CreateEvent(MethodInfo signal)
        {
            var eventName = _support.SanitizeIdentifier("On" + _support.Capitalize(signal.Name));

            var signalParams = signal.Params.ToList();

            // Declare a static field to save the index of the signal
            var indexFieldName = GetSignalIndexField(signal);
            yield return _support.CreateMetaObjectIndexField(indexFieldName);

            // Declare a static field that holds the delegate instance for marshalling the Qt signal parameters
            // back to the parameters expected by the actual user-facing delegate type
            var thunkDelegateName = eventName + "ThunkDelegate";
            var thunkMethodName = eventName + "Thunk";
            yield return ParseMemberDeclaration(
                $"private static readonly unsafe DelegateSlotCallback {thunkDelegateName} = {thunkMethodName};"
            );

            // Determine the type of the user-facing delegate. We will not support return values. Hence 'Action'
            TypeSyntax delegateType;
            if (signalParams.Count > 0)
            {
                delegateType = GenericName(Identifier("System.Action")).WithTypeArgumentList(TypeArgumentList(
                    SeparatedList(
                        signalParams.Select(p => _support.GetTypeName(p.Type))
                    )));
            }
            else
            {
                delegateType = IdentifierName("System.Action");
            }

            // Generate a method that will serve as the thunk for native calls
            yield return CreateSignalThunkMethod(thunkMethodName, signalParams, delegateType);

            var accessors = new List<AccessorDeclarationSyntax>
            {
                AccessorDeclaration(SyntaxKind.AddAccessorDeclaration)
                    .WithExpressionBody(
                        ArrowExpressionClause(
                            ParseExpression($"AddSignalHandler({indexFieldName}, value, {thunkDelegateName})")))
                    .WithSemicolonToken(Token(SyntaxKind.SemicolonToken)),
                AccessorDeclaration(SyntaxKind.RemoveAccessorDeclaration)
                    .WithExpressionBody(
                        ArrowExpressionClause(
                            ParseExpression($"RemoveSignalHandler({indexFieldName}, value)")))
                    .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
            };

            yield return EventDeclaration(delegateType, eventName)
                .WithPublicAccess()
                .WithAccessorList(AccessorList(List(accessors)));
        }

        private MemberDeclarationSyntax CreateSignalThunkMethod(string thunkMethodName,
            List<MethodParamInfo> signalParams,
            TypeSyntax delegateType)
        {
            var method = (MethodDeclarationSyntax)
                ParseMemberDeclaration(
                    $"private static unsafe void {thunkMethodName}(GCHandle delegateHandle, void** args) {{}}");

            return method.WithBody(Block(CreateSignalThunkMethodBody(signalParams, delegateType)));
        }

        private IEnumerable<StatementSyntax> CreateSignalThunkMethodBody(
            List<MethodParamInfo> signalParams,
            TypeSyntax delegateType)
        {
            // Cast the GCHandle's target to our action
            yield return LocalDeclarationStatement(
                VariableDeclaration(InferredType, SingletonSeparatedList(
                    VariableDeclarator("dlgt")
                        .WithInitializer(EqualsValueClause(CastExpression(delegateType,
                            ParseExpression("delegateHandle.Target"))))
                ))
            );

            var methodParams = signalParams.Select((signalParam, index) =>
            {
                // index +1 because first array entry is the return value
                var ptrValue = ParseExpression("args[" + (index + 1) + "]");
                return _support.TransformExpressionNativeToManaged(ptrValue, signalParam.Type);
            }).Select(Argument).ToArray();

            // Call the delegate
            yield return ExpressionStatement(InvocationExpression(
                IdentifierName("dlgt"), ArgumentList(SeparatedList(methodParams))
            ));
        }

    }
}