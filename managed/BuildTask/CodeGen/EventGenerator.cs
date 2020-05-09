using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using QmlBuildTasks.Introspection;
using static Microsoft.CodeAnalysis.CSharp.SyntaxFactory;
using TypeInfo = QmlBuildTasks.Introspection.TypeInfo;


namespace QmlBuildTasks
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
                return TransformExpressionNativeToManaged(ptrValue, signalParam.Type);
            }).Select(Argument).ToArray();

            // Call the delegate
            yield return ExpressionStatement(InvocationExpression(
                IdentifierName("dlgt"), ArgumentList(SeparatedList(methodParams))
            ));
        }

        private ExpressionSyntax DerefPointerToPrimitive(string name, ExpressionSyntax voidPtr) =>
            PrefixUnaryExpression(SyntaxKind.PointerIndirectionExpression,
                CastExpression(IdentifierName(name + "*"), voidPtr)
            );

        private ExpressionSyntax TransformExpressionNativeToManaged(ExpressionSyntax expression, TypeRef type)
        {
            // Transform expression (a void*) to the target type
            switch (type.Kind)
            {
                case TypeRefKind.TypeInfo:
                    return ObjectCreationExpression(_support.GetTypeName(type))
                        .WithArgumentList(
                            ArgumentList(SeparatedList(new[] {Argument(CastVoidPtrToIntPtr(expression))})));
                case TypeRefKind.TypeInfoEnum:
                    // Storage representation should be the same
                    return DerefPointerToPrimitive(_support.GetTypeName(type).ToFullString(), expression);
                case TypeRefKind.BuiltIn:
                    switch (type.BuiltIn)
                    {
                        case BuiltInType.Bool:
                            return DerefPointerToPrimitive("bool", expression);
                        case BuiltInType.Int32:
                            return DerefPointerToPrimitive("int", expression);
                        case BuiltInType.UInt32:
                            return DerefPointerToPrimitive("uint", expression);
                        case BuiltInType.Int64:
                            return DerefPointerToPrimitive("long", expression);
                        case BuiltInType.UInt64:
                            return DerefPointerToPrimitive("ulong", expression);
                        case BuiltInType.Double:
                            return DerefPointerToPrimitive("double", expression);
                        case BuiltInType.Char:
                            return DerefPointerToPrimitive("char", expression);
                        case BuiltInType.OpaquePointer:
                            return DerefPointerToPrimitive("IntPtr", expression);
                        case BuiltInType.String:
                            // We need special deserialization for String, sadly
                            return InvocationExpression(ParseName("QtBuiltInTypeInterop.QString_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        case BuiltInType.ByteArray:
                            // We need special deserialization for String, sadly
                            return InvocationExpression(ParseName("QtBuiltInTypeInterop.QByteArray_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        case BuiltInType.Size:
                            return InvocationExpression(ParseName("QtBuiltInTypeInterop.QSize_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        case BuiltInType.SizeFloat:
                            return InvocationExpression(ParseName("QtBuiltInTypeInterop.QSizeF_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        case BuiltInType.Rectangle:
                            return InvocationExpression(ParseName("QtBuiltInTypeInterop.QRect_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        case BuiltInType.RectangleFloat:
                            return InvocationExpression(ParseName("QtBuiltInTypeInterop.QRectF_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        case BuiltInType.Url:
                            return InvocationExpression(ParseName("QtBuiltInTypeInterop.QUrl_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        case BuiltInType.Color:
                            return InvocationExpression(ParseName("QtBuiltInTypeInterop.QColor_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        default:
                            throw new ArgumentOutOfRangeException("type.BuiltIn", type.BuiltIn,
                                "unknown built in type");
                    }
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private ExpressionSyntax CastVoidPtrToIntPtr(ExpressionSyntax voidPtr) =>
            PrefixUnaryExpression(SyntaxKind.PointerIndirectionExpression,
                CastExpression(IdentifierName("System.IntPtr*"), voidPtr)
            );
    }
}