using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using QmlBuildTasks.Introspection;
using static Microsoft.CodeAnalysis.CSharp.SyntaxFactory;

namespace QmlBuildTasks.CodeGen
{
    public class MethodGenerator
    {
        private readonly GeneratorSupport _support;

        public MethodGenerator(Options options, GeneratorSupport support)
        {
            _support = support;
        }

        public IEnumerable<MemberDeclarationSyntax> CreateMethod(MethodInfo method)
        {
            // Declare a static field to save the index of the method
            var indexFieldName = GetMethodIndexField(method);
            yield return _support.CreateMetaObjectIndexField(indexFieldName);

            var parameters = method.Params.ToList();

            var returnValueType = GetReturnValueHandling(method.ReturnType);
            var managedParams = parameters.AsEnumerable();

            // Check for special case:
            // A method that returns void and has a CompletionSource as it's last parameter
            // will be auto-translated to an asynchronous method.
            if (returnValueType == ReturnValueHandling.None
                && parameters.Count > 0
                && parameters.Last().Type.Kind == TypeRefKind.BuiltIn
                && parameters.Last().Type.BuiltIn == BuiltInType.CompletionSource)
            {
                returnValueType = ReturnValueHandling.CompletionSource;
                // Do not append the completion source to the managed parameter list
                managedParams = parameters.Take(parameters.Count - 1);
            }

            IEnumerable<StatementSyntax> CreateBody()
            {
                // Declare the array to hold the argv for the call, index 0 is the return value pointer
                yield return ParseStatement($"void **argv = stackalloc void*[{parameters.Count + 1}];");

                foreach (var syntax in CreateReturnValueSetup(method, returnValueType))
                {
                    yield return syntax;
                }

                var setup = new List<StatementSyntax>();
                var cleanup = new List<StatementSyntax>();

                for (var i = 0; i < parameters.Count; i++)
                {
                    var parameter = parameters[i];
                    var paramName = GetMethodParameterName(parameter, i);
                    // Use the locally allocated completion source as the last parameter when transforming to async
                    if (returnValueType == ReturnValueHandling.CompletionSource
                        && i == parameters.Count - 1)
                    {
                        yield return ExpressionStatement(AssignmentExpression(
                            SyntaxKind.SimpleAssignmentExpression,
                            ParseExpression($"argv[{1 + i}]"),
                            ParseExpression("&completionSource")
                        ));
                        continue;
                    }

                    var unmanagedExpression = CreateUnmanagedParameterExpression(parameter, paramName, setup, cleanup);
                    foreach (var statement in setup)
                    {
                        yield return statement;
                    }

                    setup.Clear();

                    yield return ExpressionStatement(AssignmentExpression(
                        SyntaxKind.SimpleAssignmentExpression,
                        ParseExpression($"argv[{1 + i}]"),
                        unmanagedExpression
                    ));
                }

                var callStatement = ParseStatement($"QObject_callMethod(_handle, {indexFieldName}, argv);");

                if (cleanup.Count > 0)
                {
                    yield return TryStatement()
                        .WithBlock(Block(callStatement))
                        .WithFinally(FinallyClause(Block(
                            cleanup
                        )));
                }
                else
                {
                    yield return callStatement;
                }

                foreach (var syntax in CreateReturnStatement(method, returnValueType))
                {
                    yield return syntax;
                }
            }

            var methodName = _support.PascalifyName(method.Name);

            var returnTypeSyntax = _support.GetTypeName(method.ReturnType);
            if (returnValueType == ReturnValueHandling.CompletionSource)
            {
                returnTypeSyntax = ParseTypeName("System.Threading.Tasks.Task<IntPtr>");
            }

            yield return MethodDeclaration(returnTypeSyntax, methodName)
                .WithModifiers(TokenList(Token(SyntaxKind.PublicKeyword), Token(SyntaxKind.UnsafeKeyword)))
                .WithParameterList(CreateParameterList(managedParams))
                .WithBody(Block(CreateBody()));
        }

        private IEnumerable<StatementSyntax> CreateReturnValueSetup(MethodInfo method,
            ReturnValueHandling returnValueType)
        {
            if (returnValueType == ReturnValueHandling.None)
            {
                yield break; // No return value handling needed
            }

            if (returnValueType == ReturnValueHandling.String)
            {
                yield return ParseStatement("using var result = new QStringArg(null);");
                yield return ParseStatement("argv[0] = result.NativePointer;");
            }
            else if (returnValueType == ReturnValueHandling.CompletionSource)
            {
                yield return ParseStatement(
                    "var (nativeTask, completionSource) = OpenTemple.Interop.NativeCompletionSource.Create<IntPtr>();");
            }
            else if (returnValueType == ReturnValueHandling.Primitive
                     || returnValueType == ReturnValueHandling.QObject)
            {
                // Declare a stack variable to hold the native representation of the return value
                yield return LocalDeclarationStatement(VariableDeclaration(
                    GetNativeRepresentation(method.ReturnType),
                    SingletonSeparatedList(VariableDeclarator("result"))
                ));
                yield return ParseStatement("argv[0] = &result;");
            }
            else if (returnValueType == ReturnValueHandling.ValueType)
            {
                var interopPrefix = GetValueTypeInteropPrefix(method.ReturnType);

                // Use a stackalloc statement to allocate stack space to hold the value
                // Introduce a temporary for the stack-allocated QString object
                yield return ParseStatement(
                    $"var resultUnmanaged = stackalloc byte[QtBuiltInTypeInterop.{interopPrefix}Size];"
                );
                yield return ParseStatement(
                    $"QtBuiltInTypeInterop.{interopPrefix}_ctor_default(&resultUnmanaged);");
                yield return ParseStatement("argv[0] = &resultUnmanaged;");
            }
            else
            {
                throw new ArgumentOutOfRangeException(nameof(returnValueType), returnValueType,
                    "Unknown return value type.");
            }
        }

        private IEnumerable<StatementSyntax> CreateReturnStatement(MethodInfo method,
            ReturnValueHandling returnValueType)
        {
            if (returnValueType == ReturnValueHandling.None)
            {
                yield break;
            }

            if (returnValueType == ReturnValueHandling.Primitive)
            {
                yield return ReturnStatement(IdentifierName("result"));
            }
            else if (returnValueType == ReturnValueHandling.CompletionSource)
            {
                yield return ParseStatement("return nativeTask;");
            }
            else if (returnValueType == ReturnValueHandling.QObject)
            {
                // Result should be an IntPtr containing the native OQbject*
                yield return ParseStatement(
                    $"return QObjectBase.GetQObjectProxy<{_support.GetQualifiedName(method.ReturnType.TypeInfo)}>(result);");
            }
            else if (returnValueType == ReturnValueHandling.String)
            {
                yield return ParseStatement("return result.ToString();");
            }
            else if (returnValueType == ReturnValueHandling.ValueType)
            {
                // If no cleanup is required, return the value type directly
                if (!HasUnmanagedDestructor(method.ReturnType.BuiltIn))
                {
                    yield return ReturnStatement(
                        _support.TransformExpressionNativeToManaged(ParseExpression("&resultUnmanaged"),
                            method.ReturnType)
                    );
                }
                else
                {
                    // resultUnmanaged is set up in CreateReturnValueSetup
                    yield return LocalDeclarationStatement(VariableDeclaration(
                        GetNativeRepresentation(method.ReturnType),
                        SingletonSeparatedList(VariableDeclarator("result")
                            .WithInitializer(
                                EqualsValueClause(
                                    _support.TransformExpressionNativeToManaged(ParseExpression("&resultUnmanaged"),
                                        method.ReturnType)
                                )))
                    ));

                    // Now that we have read what we need from resultUnmanaged, clean it up
                    var interopPrefix = GetValueTypeInteropPrefix(method.ReturnType);
                    yield return ParseStatement($"QtBuiltInTypeInterop.{interopPrefix}_dtor(&resultUnmanaged);");

                    yield return ParseStatement("return result;");
                }
            }
            else
            {
                throw new ArgumentOutOfRangeException(nameof(returnValueType), returnValueType,
                    "Unknown return value type.");
            }
        }

        private enum ReturnValueHandling
        {
            // Method has no return value
            None,

            // Declare primitive var on stack, pass pointer directly to C++
            Primitive,

            // Built-In Value Type that may or may not have a ctor/dtor,
            // But is allocated on the managed stack
            ValueType,

            // Use QStringArg (ref struct), allocates QString on heap via P/Invoke
            // Could be optimized by introducing additional method on C++ side that
            // returns char16_t* directly.
            String,

            // Allocate new QObject proxy based on return value
            QObject,

            // Return the task of the completion source when method is being translated to an async method
            CompletionSource
        }

        private ReturnValueHandling GetReturnValueHandling(TypeRef typeRef)
        {
            if (typeRef.IsUnmanaged)
            {
                return ReturnValueHandling.Primitive;
            }

            switch (typeRef.Kind)
            {
                case TypeRefKind.Void:
                    return ReturnValueHandling.None;
                case TypeRefKind.BuiltIn:
                    if (typeRef.BuiltIn == BuiltInType.String)
                    {
                        return ReturnValueHandling.String;
                    }
                    else
                    {
                        return ReturnValueHandling.ValueType;
                    }
                case TypeRefKind.TypeInfo:
                    return ReturnValueHandling.QObject;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private static ExpressionSyntax CreateUnmanagedParameterExpression(MethodParamInfo parameter,
            string paramName, List<StatementSyntax> setup, List<StatementSyntax> cleanup)
        {
            var parameterType = parameter.Type;
            if (parameterType.IsUnmanaged)
            {
                return ParseExpression("&" + paramName);
            }
            else if (parameterType.Kind == TypeRefKind.BuiltIn)
            {
                if (parameterType.BuiltIn == BuiltInType.String)
                {
                    setup.Add(ParseStatement(
                        $"using var {paramName}Temp = new QGadgetBase.QStringArg({paramName});"
                    ));
                    return ParseExpression(paramName + "Temp.NativePointer");
                }

                var interopPrefix = GetValueTypeInteropPrefix(parameterType);

                // Introduce a temporary for the stack-allocated QString object
                setup.Add(ParseStatement(
                    $"var {paramName}Temp = stackalloc byte[QtBuiltInTypeInterop.{interopPrefix}Size];"
                ));
                setup.Add(ParseStatement(
                    $"QtBuiltInTypeInterop.{interopPrefix}_ctor({paramName}Temp, {GetManagedCtorArgs(parameterType.BuiltIn, paramName)});"));
                if (HasUnmanagedDestructor(parameterType.BuiltIn))
                {
                    cleanup.Add(ParseStatement($"QtBuiltInTypeInterop.{interopPrefix}_dtor({paramName}Temp);"));
                }

                return ParseExpression(paramName + "Temp");
            }
            else if (parameterType.Kind == TypeRefKind.TypeInfo)
            {
                // Qt expects a pointer to the actual QObject pointer, so we need a temp
                setup.Add(ParseStatement($"var {paramName}Temp = {paramName}.NativePointer;"));
                return ParseExpression("&" + paramName + "Temp");
            }
            else
            {
                throw new ArgumentOutOfRangeException();
            }
        }

        private static string GetValueTypeInteropPrefix(TypeRef parameterType)
        {
            var interopPrefix = parameterType.BuiltIn switch
            {
                BuiltInType.String => $"QString",
                BuiltInType.ByteArray => "QByteArray",
                BuiltInType.Color => "QColor",
                BuiltInType.Size => "QSize",
                BuiltInType.SizeFloat => "QSize",
                BuiltInType.Rectangle => "QRect",
                BuiltInType.RectangleFloat => "QRectF",
                BuiltInType.Point => "QPoint",
                BuiltInType.PointFloat => "QPointF",
                BuiltInType.Url => "QUrl",
                _ => throw new ArgumentOutOfRangeException(nameof(parameterType.BuiltIn), parameterType.BuiltIn,
                    "Unknown built in parameter type.")
            };
            return interopPrefix;
        }

        private TypeSyntax GetNativeRepresentation(TypeRef typeRef)
        {
            switch (typeRef.Kind)
            {
                case TypeRefKind.TypeInfo:
                    if (typeRef.TypeInfo.Kind == TypeInfoKind.CppGadget)
                    {
                        throw new ArgumentException("DUNNO HOW");
                    }

                    return IdentifierName("System.IntPtr");
                case TypeRefKind.BuiltIn:
                    switch (typeRef.BuiltIn)
                    {
                        case BuiltInType.Bool:
                            return PredefinedType(Token(SyntaxKind.BoolKeyword));
                        case BuiltInType.Int32:
                            return PredefinedType(Token(SyntaxKind.IntKeyword));
                        case BuiltInType.UInt32:
                            return PredefinedType(Token(SyntaxKind.UIntKeyword));
                        case BuiltInType.Int64:
                            return PredefinedType(Token(SyntaxKind.LongKeyword));
                        case BuiltInType.UInt64:
                            return PredefinedType(Token(SyntaxKind.ULongKeyword));
                        case BuiltInType.Double:
                            return PredefinedType(Token(SyntaxKind.DoubleKeyword));
                        case BuiltInType.Char:
                            return PredefinedType(Token(SyntaxKind.CharKeyword));
                        case BuiltInType.OpaquePointer:
                            return IdentifierName("System.IntPtr");
                        case BuiltInType.String:
                            // We are actually receiving a pointer to a QString...
                            return IdentifierName("System.IntPtr");
                        default:
                            throw new ArgumentOutOfRangeException(nameof(typeRef.BuiltIn), typeRef.BuiltIn,
                                "Unknown typeref builtin");
                    }
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        // See static_assert(std::is_trivially_destructible<T>()) in native code
        private static bool HasUnmanagedDestructor(BuiltInType type)
        {
            return type switch
            {
                BuiltInType.String => true,
                BuiltInType.ByteArray => true,
                BuiltInType.DateTime => true,
                BuiltInType.Url => true,
                _ => false
            };
        }

        // Gets the arguments to the interop ctor that will construct an unamanged instance
        // within managed stackspace
        private static string GetManagedCtorArgs(BuiltInType type, string variableName)
        {
            return type switch
            {
                // Incoming: string (nullable!)
                BuiltInType.String => $"{variableName}, {variableName}?.Length ?? 0",
                // Incoming: byte[] (nullable!)
                BuiltInType.ByteArray => $"{variableName}, {variableName}?.Length ?? 0",
                // TODO DateTime
                // TODO Date
                // TODO Time
                BuiltInType.Color => $"{variableName}.{nameof(Color.ToArgb)}()",
                BuiltInType.Size => $"{variableName}.Width, {variableName}.Height",
                BuiltInType.SizeFloat => $"{variableName}.Width, {variableName}.Height",
                BuiltInType.Rectangle =>
                $"{variableName}.X, {variableName}.Y, {variableName}.Width, {variableName}.Height",
                BuiltInType.RectangleFloat =>
                $"{variableName}.X, {variableName}.Y, {variableName}.Width, {variableName}.Height",
                BuiltInType.Point => $"{variableName}.X, {variableName}.Y",
                BuiltInType.PointFloat => $"{variableName}.X, {variableName}.Y",
                // Incoming: string (nullable!)
                BuiltInType.Url => $"{variableName}, {variableName}?.Length ?? 0",
                _ => throw new ArgumentOutOfRangeException(nameof(type), type, "No managed ctor available.")
            };
        }

        private ParameterListSyntax CreateParameterList(IEnumerable<MethodParamInfo> @params)
        {
            return ParameterList(SeparatedList(
                @params.Select((p, pIdx) => Parameter(Identifier(GetMethodParameterName(p, pIdx)))
                    .WithType(_support.GetTypeName(p.Type)))
            ));
        }

        private string GetMethodParameterName(MethodParamInfo p, int index)
        {
            return _support.SanitizeIdentifier(p.Name ?? $"arg{index + 1}");
        }

        public string GetMethodIndexField(MethodInfo method)
        {
            var fieldName = "_" + method.Name + "Index";
            if (method.OverloadIndex != -1)
            {
                fieldName += method.OverloadIndex;
            }

            return fieldName;
        }
    }
}