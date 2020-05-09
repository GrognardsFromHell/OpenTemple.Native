using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using QmlBuildTasks.Introspection;
using static Microsoft.CodeAnalysis.CSharp.SyntaxFactory;
using TypeInfo = QmlBuildTasks.Introspection.TypeInfo;

namespace QmlBuildTasks
{
    public class ProxiesGenerator
    {
        private readonly Options _options;

        private readonly PropertyGenerator _properties;

        private readonly EventGenerator _events;

        private readonly GeneratorSupport _support;

        public ProxiesGenerator(Options options)
        {
            _options = options;
            _support = new GeneratorSupport(options);
            _properties = new PropertyGenerator(options, _support);
            _events = new EventGenerator(options, _support);
        }

        private static NameSyntax InteropNamespace => IdentifierName("OpenTemple.Interop");

        public string Generate(IEnumerable<TypeInfo> types)
        {
            var typeInfos = types.ToList();
            return CompilationUnit()
                .WithUsings(List(new[]
                    {
                        UsingDirective(IdentifierName("System")),
                        UsingDirective(IdentifierName("System.Runtime.InteropServices")),
                        UsingDirective(InteropNamespace)
                    }
                ))
                .WithMembers(
                    List<MemberDeclarationSyntax>(
                        typeInfos.GroupBy(t => _support.GetNamespace(t).ToFullString())
                            .Select(CreateNamespace).Concat(new[]
                                {
                                    NamespaceDeclaration(InteropNamespace)
                                        .WithMembers(List(CreateSupportMembers(typeInfos)))
                                }
                            )
                    )
                )
                .WithAttributeLists(
                    // Emit an assembly attribute referencing the type registry so that
                    // we can circumvent issues with not calling static constructors
                    ParseCompilationUnit(
                        "[assembly: OpenTemple.Interop.QmlTypeRegistry(typeof(GeneratedTypesRegistry))]"
                    ).AttributeLists
                )
                .NormalizeWhitespace()
                .ToFullString();
        }

        private IEnumerable<MemberDeclarationSyntax> CreateSupportMembers(IEnumerable<TypeInfo> types)
        {
            yield return CreateTypeRegistry(types);
        }

        /// Creates the partial for QObjectTypeRegistry that maps from runtime info to the generated classes
        private MemberDeclarationSyntax CreateTypeRegistry(IEnumerable<TypeInfo> types)
        {
            IEnumerable<StatementSyntax> CreateTypeRegistrations()
            {
                var registrationArgs = new List<ExpressionSyntax>();
                foreach (var typeInfo in types)
                {
                    registrationArgs.Clear();
                    string registrationMethod;

                    if (typeInfo.Kind == TypeInfoKind.QmlQObject)
                    {
                        registrationMethod = "QObjectTypeRegistry.RegisterQmlFile";
                        registrationArgs.Add(LiteralExpression(SyntaxKind.StringLiteralExpression,
                            Literal(typeInfo.QmlSourceUrl)));

                        // For inline components, their name is part of the runtime type info as well
                        if (typeInfo.IsInlineComponent)
                        {
                            registrationArgs.Add(LiteralExpression(SyntaxKind.StringLiteralExpression,
                                Literal(typeInfo.InlineComponentName)));
                        }
                        else
                        {
                            registrationArgs.Add(LiteralExpression(SyntaxKind.NullLiteralExpression));
                        }
                    }
                    else if (typeInfo.Kind == TypeInfoKind.CppQObject)
                    {
                        registrationMethod = "QObjectTypeRegistry.RegisterMetaClass";
                        registrationArgs.Add(LiteralExpression(SyntaxKind.StringLiteralExpression,
                            Literal(typeInfo.MetaClassName)));
                    }
                    else
                    {
                        // Gadgets cannot be resolved from pointers since they always pass by value
                        continue;
                    }

                    // add the constructor callback
                    registrationArgs.Add(SimpleLambdaExpression(
                        Parameter(Identifier("handle")),
                        ObjectCreationExpression(_support.GetQualifiedName(typeInfo))
                            .WithArgumentList(ArgumentList(
                                SingletonSeparatedList(Argument(IdentifierName("handle")))
                            ))
                    ));

                    yield return ExpressionStatement(InvocationExpression(
                        IdentifierName(registrationMethod),
                        ArgumentList(SeparatedList(
                            registrationArgs.Select(Argument)
                        ))));
                }
            }

            MemberDeclarationSyntax CreateRegistrationMethod()
            {
                return ((ConstructorDeclarationSyntax) ParseMemberDeclaration("static GeneratedTypesRegistry() {}"))
                    .WithBody(Block(CreateTypeRegistrations()));
            }

            return ((ClassDeclarationSyntax) ParseMemberDeclaration(
                    "public class GeneratedTypesRegistry {}")
                )
                .WithMembers(SingletonList(CreateRegistrationMethod()));
        }

        private NamespaceDeclarationSyntax CreateNamespace(IEnumerable<TypeInfo> types)
        {
            var typesList = types.ToList();
            var namespaceContent = typesList.Select(CreateProxyClass).ToList();

            for (var i = namespaceContent.Count - 1; i >= 0; i--)
            {
                var proxyClass = namespaceContent[i];
                var enclosingType = typesList[i].EnclosingType;
                if (!enclosingType.HasValue)
                {
                    continue;
                }

                // Find the matching type
                int otherIndex = typesList.IndexOf(enclosingType.Value);
                if (otherIndex == -1)
                {
                    throw new InvalidOperationException("Inline component " + typesList[i].Name + "#" +
                                                        typesList[i].InlineComponentName
                                                        + " has no parent!");
                }

                // Remove it from the top level namespace declarations, and add it to the members instead
                namespaceContent[otherIndex] = namespaceContent[otherIndex]
                    .WithMembers(namespaceContent[otherIndex].Members.Add(proxyClass));
                namespaceContent.RemoveAt(i);
            }

            return NamespaceDeclaration(_support.GetNamespace(typesList.First()))
                .WithMembers(List(namespaceContent.Cast<MemberDeclarationSyntax>()));
        }

        private ClassDeclarationSyntax CreateProxyClass(TypeInfo type)
        {
            return ClassDeclaration(_support.GetTypeName(type).Identifier)
                    .WithBaseList(GetBaseList(type))
                    .WithPublicAccess()
                    .WithMembers(List(CreateProxyMembers(type)))
                ;
        }

        private IEnumerable<MemberDeclarationSyntax> CreateProxyMembers(TypeInfo type)
        {
            // Emit a static constructor that ENSURES the type registry has been initialized
            yield return ParseMemberDeclaration(
                $"static {_support.GetTypeName(type).Identifier}() {{\nSystem.Runtime.CompilerServices.RuntimeHelpers.RunClassConstructor(typeof(OpenTemple.Interop.GeneratedTypesRegistry).TypeHandle);\n}}");

            // To get the MetaObject for a QML type, we need an example object
            var lazyMetaObjectInitCall = type.Kind == TypeInfoKind.QmlQObject
                ? "LazyInitializeMetaObject(handle)"
                : "LazyInitializeMetaObject()";

            foreach (var part in CreateTypeInitializer(type))
            {
                yield return part;
            }

            foreach (var enumInfo in type.Enums)
            {
                yield return CreateEnum(enumInfo, type);
            }

            // Create constructor
            if (true || type.Kind == TypeInfoKind.CppQObject || type.Kind == TypeInfoKind.QmlQObject)
            {
                yield return ConstructorDeclaration(_support.GetTypeName(type).Identifier)
                    .WithPublicAccess()
                    .WithParameterList(
                        ParameterList(
                            SingletonSeparatedList(
                                Parameter(
                                        Identifier("handle"))
                                    .WithType(
                                        IdentifierName("IntPtr")))))
                    .WithInitializer(
                        ConstructorInitializer(
                            SyntaxKind.BaseConstructorInitializer,
                            ArgumentList(
                                SingletonSeparatedList(
                                    Argument(
                                        IdentifierName("handle"))))))
                    .WithBody(Block(ParseStatement(lazyMetaObjectInitCall + ";")));
            }

            // If the type is a C++ type, we should be able to construct it without a QML engine
            if (type.Kind == TypeInfoKind.CppQObject)
            {
                yield return ParseMemberDeclaration(
                    $"public {_support.GetTypeName(type).Identifier}() : base(QObjectBase.CreateInstance({lazyMetaObjectInitCall})\n) {{}}");
            }

            foreach (var prop in type.Props)
            {
                foreach (var syntax in _properties.CreateProperty(prop))
                {
                    yield return syntax;
                }
            }

            foreach (var method in type.Methods)
            {
                foreach (var syntax in CreateMethod(method))
                {
                    yield return syntax;
                }
            }

            // We do not support signal overloading, which Qt _does_ support,
            // so instead, we'll have to use the signal with the longest unambiguous
            // parameter list
            var signals = type.Signals.ToList();
            for (var i = signals.Count - 1; i >= 0; i--)
            {
                var name = signals[i].Name;
                var paramCount = signals[i].Params.Count();
                for (var j = i - 1; j >= 0; j--)
                {
                    if (signals[j].Name == name)
                    {
                        // Conflicting definition found
                        var otherLen = signals[j].Params.Count();
                        if (otherLen == paramCount)
                        {
                            Console.WriteLine("Ambiguous signal found: " + name);
                        }
                        else if (otherLen < paramCount)
                        {
                            // the other one has less signifiance, swap and remove
                            signals[j] = signals[i];
                            signals.RemoveAt(i);
                            break;
                        }
                        else if (otherLen > paramCount)
                        {
                            signals.RemoveAt(i);
                            break;
                        }
                    }
                }
            }

            foreach (var signal in signals)
            {
                foreach (var syntax in _events.CreateEvent(signal))
                {
                    yield return syntax;
                }
            }
        }

        private MemberDeclarationSyntax CreateEnum(EnumInfo enumInfo, TypeInfo type)
        {
            var enumName = enumInfo.Name;
            // annoying as hell, but if names are capitalized, we can introduce clashes with enums :|
            if (_options.PascalCase)
            {
                var clash = type.Props.Select(prop => _support.Capitalize(prop.Name))
                    .Concat(type.Methods.Select(method => _support.Capitalize(method.Name)))
                    .Contains(enumName);
                Console.WriteLine($"In type {type.Name}, enum {enumInfo.Name} clashes with a method or property.");
                // Prefer pluralization
                if (!enumName.EndsWith("s"))
                {
                    enumName += "s";
                }
                else
                {
                    enumName += "Enum";
                }
            }

            var result = EnumDeclaration(enumName)
                .WithPublicAccess()
                .WithMembers(SeparatedList(
                    enumInfo.Values.Select(value => EnumMemberDeclaration(value.Name)
                        .WithEqualsValue(EqualsValueClause(LiteralExpression(SyntaxKind.NumericLiteralExpression,
                            Literal(value.Value)))))
                ));

            if (enumInfo.IsFlags)
            {
                result = result.WithAttributeLists(SingletonList(
                    AttributeList(SingletonSeparatedList<AttributeSyntax>(
                        Attribute(IdentifierName("Flags"))
                    )))
                );
            }

            return result;
        }

        // Generates a private method InitializeMetaObject, which will be called exactly once
        // to initialize the type properties (it's unsafe, btw.) and relies on a property _metaObject;
        // NOTE: We CANNOT do this in the static constructor, because in case of QML types and even
        // CPP types that come from a QML plugin may be loaded in response to loading a .QML file.
        // An alternative approach may be remembering the int's and sanity checking only, allowing
        // the integers to be inlined.
        private IEnumerable<MemberDeclarationSyntax> CreateTypeInitializer(TypeInfo type)
        {
            yield return ParseMemberDeclaration("private static System.IntPtr _metaObject;");

            var method = MethodDeclaration(
                    IdentifierName("System.IntPtr"),
                    Identifier("LazyInitializeMetaObject")
                )
                .WithPrivateStatic()
                .WithBody(
                    Block(CreateTypeInitializerBody(type))
                );

            // QML objects need an example object
            if (type.Kind == TypeInfoKind.QmlQObject)
            {
                method = method.WithParameterList(
                    ParameterList(SingletonSeparatedList(
                        Parameter(Identifier("exampleInstance"))
                            .WithType(IdentifierName("IntPtr"))
                    ))
                );
            }

            yield return method;
        }

        private IEnumerable<StatementSyntax> CreateTypeInitializerBody(TypeInfo type)
        {
            // Do nothing if the meta object is already known
            yield return ParseStatement(@"if (_metaObject != IntPtr.Zero) return _metaObject;");

            // Create a local variable for the discovered meta object
            yield return LocalDeclarationStatement(
                VariableDeclaration(IdentifierName("var"))
                    .WithVariables(SingletonSeparatedList(
                        VariableDeclarator("metaObject")
                            .WithInitializer(EqualsValueClause(CreateMetaObjectDiscovery(type)))
                    ))
            );

            // Now write code that will lookup every property's index
            // in the discovered meta object for faster setting/getting
            foreach (var prop in type.Props)
            {
                foreach (var stmt in _properties.CreateInitializer(prop, "metaObject"))
                {
                    yield return stmt;
                }
            }

            // Same for signals & methods
            foreach (var signal in type.Signals)
            {
                var indexField = _events.GetSignalIndexField(signal);
                yield return ParseStatement(
                    $"FindMetaObjectMethod(metaObject, \"{signal.Signature}\", out {indexField});");
            }

            foreach (var method in type.Methods)
            {
                var indexField = GetMethodIndexField(method);
                yield return ParseStatement(
                    $"FindMetaObjectMethod(metaObject, \"{method.Signature}\", out {indexField});");
            }

            // Save the discovered meta object
            yield return ParseStatement(@"_metaObject = metaObject;");

            // To allow the meta object to be chained into a base constructor call
            yield return ParseStatement("return _metaObject;");
        }

        // How we can detect the meta object depends on which kind of type we're generating
        // the binding for. For C++ types, we will do it in advance using the C++ type name,
        // while for QML documents, we need an active Engine (sucks), so we'll just use
        // the meta object of the actual object passed in.
        private ExpressionSyntax CreateMetaObjectDiscovery(TypeInfo type)
        {
            // Class names of QML files are dynamic at runtime (depend on load order)
            if (type.Kind == TypeInfoKind.QmlQObject)
            {
                return InvocationExpression(IdentifierName("QObjectBase.GetQmlMetaObject"))
                    .WithArgumentList(ArgumentList(
                        SeparatedList(new[]
                        {
                            // We need to pass the example instance, because _really_ the
                            // QML file's are loaded just into the QML engine and their
                            // global registration is temporary
                            Argument(IdentifierName("exampleInstance")),
                            Argument(LiteralExpression(
                                SyntaxKind.StringLiteralExpression,
                                Literal(type.QmlSourceUrl))),
                            // Pass the inline component name as the third arg or null
                            Argument(
                                type.IsInlineComponent
                                    ? LiteralExpression(
                                        SyntaxKind.StringLiteralExpression,
                                        Literal(type.InlineComponentName))
                                    : LiteralExpression(SyntaxKind.NullLiteralExpression)
                            )
                        }))
                    );
            }

            if (type.QmlModule != null && type.QmlMajorVersion != -1)
            {
                return InvocationExpression(IdentifierName("QObjectBase.GetQmlCppMetaObject"))
                    .WithArgumentList(ArgumentList(
                        SeparatedList(new[]
                            {
                                Argument(LiteralExpression(SyntaxKind.StringLiteralExpression,
                                    Literal(type.MetaClassName))),
                                Argument(LiteralExpression(SyntaxKind.StringLiteralExpression,
                                    Literal(type.Name))),
                                Argument(LiteralExpression(SyntaxKind.StringLiteralExpression,
                                    Literal(type.QmlModule))),
                                Argument(LiteralExpression(SyntaxKind.NumericLiteralExpression,
                                    Literal(type.QmlMajorVersion))),
                                Argument(LiteralExpression(SyntaxKind.NumericLiteralExpression,
                                    Literal(type.QmlMinorVersion))),
                            }
                        ))
                    );
            }

            return InvocationExpression(IdentifierName("QObjectBase.GetCppMetaObject"))
                .WithArgumentList(ArgumentList(
                    SingletonSeparatedList(Argument(
                        LiteralExpression(
                            SyntaxKind.StringLiteralExpression,
                            Literal(type.MetaClassName)))))
                );
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
                            throw new ArgumentOutOfRangeException();
                    }
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private enum ReturnValueHandling
        {
            // Method has no return value
            None,

            // Declare primitive var on stack, pass pointer directly to C++
            Primitive,

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
                        return ReturnValueHandling.Primitive;
                    }
                case TypeRefKind.TypeInfo:
                    return ReturnValueHandling.QObject;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        private IEnumerable<MemberDeclarationSyntax> CreateMethod(MethodInfo method)
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
                else if (returnValueType != ReturnValueHandling.None)
                {
                    // Declare a stack variable to hold the native representation of the return value
                    yield return LocalDeclarationStatement(VariableDeclaration(
                        GetNativeRepresentation(method.ReturnType),
                        SingletonSeparatedList(VariableDeclarator("result"))
                    ));
                    yield return ParseStatement("argv[0] = &result;");
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

        private string GetMethodIndexField(MethodInfo method)
        {
            var fieldName = "_" + method.Name + "Index";
            if (method.OverloadIndex != -1)
            {
                fieldName += method.OverloadIndex;
            }

            return fieldName;
        }

        private BaseListSyntax GetBaseList(in TypeInfo type)
        {
            var parent = type.Parent;
            if (!parent.HasValue)
            {
                if (type.Kind == TypeInfoKind.CppGadget)
                {
                    return BaseList(SingletonSeparatedList<BaseTypeSyntax>(
                        SimpleBaseType(QualifiedName(InteropNamespace, IdentifierName("QGadgetBase")))
                    ));
                }

                return BaseList(SingletonSeparatedList<BaseTypeSyntax>(
                    SimpleBaseType(QualifiedName(InteropNamespace, IdentifierName("QObjectBase")))
                ));
            }

            return BaseList(SingletonSeparatedList<BaseTypeSyntax>(
                SimpleBaseType(_support.GetQualifiedName(parent.Value))
            ));
        }
    }

    internal static class SyntaxExtensions
    {
        internal static T WithPublicAccess<T>(this T syntax) where T : MemberDeclarationSyntax
        {
            return (T) syntax.WithModifiers(TokenList(Token(SyntaxKind.PublicKeyword)));
        }

        internal static T WithPrivateStatic<T>(this T syntax) where T : MemberDeclarationSyntax
        {
            return (T) syntax.WithModifiers(
                TokenList(Token(SyntaxKind.PrivateKeyword), Token(SyntaxKind.StaticKeyword)));
        }
    }
}