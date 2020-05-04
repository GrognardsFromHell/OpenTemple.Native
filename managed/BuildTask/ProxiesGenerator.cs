using System;
using System.Collections.Generic;
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

        public ProxiesGenerator(Options options)
        {
            _options = options;
        }

        private static TypeSyntax InferredType => IdentifierName("var");

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
                        typeInfos.GroupBy(t => GetNamespace(t).ToFullString())
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
                        ObjectCreationExpression(GetQualifiedName(typeInfo))
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

            return NamespaceDeclaration(GetNamespace(typesList.First()))
                .WithMembers(List(namespaceContent.Cast<MemberDeclarationSyntax>()));
        }

        private ClassDeclarationSyntax CreateProxyClass(TypeInfo type)
        {
            return ClassDeclaration(GetTypeName(type).Identifier)
                    .WithBaseList(GetBaseList(type))
                    .WithPublicAccess()
                    .WithMembers(List(CreateProxyMembers(type)))
                ;
        }

        private IEnumerable<MemberDeclarationSyntax> CreateProxyMembers(TypeInfo type)
        {
            // Emit a static constructor that ENSURES the type registry has been initialized
            yield return ParseMemberDeclaration(
                $"static {GetTypeName(type).Identifier}() {{\nSystem.Runtime.CompilerServices.RuntimeHelpers.RunClassConstructor(typeof(OpenTemple.Interop.GeneratedTypesRegistry).TypeHandle);\n}}");

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
                yield return ConstructorDeclaration(GetTypeName(type).Identifier)
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
                    $"public {GetTypeName(type).Identifier}() : base(QObjectBase.CreateInstance({lazyMetaObjectInitCall})\n) {{}}");
            }

            foreach (var prop in type.Props)
            {
                foreach (var syntax in CreateProperty(prop))
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
                foreach (var syntax in CreateEvent(signal))
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
                var clash = type.Props.Select(prop => Capitalize(prop.Name))
                    .Concat(type.Methods.Select(method => Capitalize(method.Name)))
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

            return EnumDeclaration(enumName)
                .WithPublicAccess()
                .WithMembers(SeparatedList(
                    enumInfo.Values.Select(value => EnumMemberDeclaration(value.Name)
                        .WithEqualsValue(EqualsValueClause(LiteralExpression(SyntaxKind.NumericLiteralExpression,
                            Literal(value.Value)))))
                ));
        }

        private TypeSyntax GetTypeName(TypeRef typeRef)
        {
            return typeRef.Kind switch
            {
                TypeRefKind.Void => PredefinedType(Token(SyntaxKind.VoidKeyword)),
                TypeRefKind.TypeInfo => GetQualifiedName(typeRef.TypeInfo),
                TypeRefKind.BuiltIn => PredefinedType(Token(typeRef.BuiltIn switch
                {
                    BuiltInType.Bool => SyntaxKind.BoolKeyword,
                    BuiltInType.Int32 => SyntaxKind.IntKeyword,
                    BuiltInType.UInt32 => SyntaxKind.UIntKeyword,
                    BuiltInType.Int64 => SyntaxKind.LongKeyword,
                    BuiltInType.UInt64 => SyntaxKind.ULongKeyword,
                    BuiltInType.Double => SyntaxKind.DoubleKeyword,
                    BuiltInType.Char => SyntaxKind.CharKeyword,
                    BuiltInType.String => SyntaxKind.StringKeyword,
                    _ => throw new ArgumentOutOfRangeException(typeRef.BuiltIn.ToString())
                })),
                _ => throw new ArgumentOutOfRangeException(typeRef.Kind.ToString())
            };
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
                var propertyIndexField = GetPropertyIndexField(prop);
                yield return ParseStatement(
                    $"FindMetaObjectProperty(metaObject, \"{prop.Name}\", out {propertyIndexField});");
            }

            // Same for signals & methods
            foreach (var signal in type.Signals)
            {
                var indexField = GetSignalIndexField(signal);
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

        private IEnumerable<MemberDeclarationSyntax> CreateProperty(PropInfo prop)
        {
            var accessors = new List<AccessorDeclarationSyntax>();

            var propertyIndexField = GetPropertyIndexField(prop);

            yield return CreateMetaObjectIndexField(propertyIndexField);

            if (prop.IsReadable)
            {
                if (prop.Type.Kind == TypeRefKind.BuiltIn)
                {
                    if (prop.Type.BuiltIn == BuiltInType.String)
                    {
                        accessors.Add(AccessorDeclaration(SyntaxKind.GetAccessorDeclaration)
                            .WithExpressionBody(
                                ArrowExpressionClause(
                                    InvocationExpression(IdentifierName("GetPropertyQString"))
                                        .WithArgumentList(ArgumentList(
                                            SingletonSeparatedList(Argument(IdentifierName(propertyIndexField)))))))
                            .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
                        );
                    }
                    else
                    {
                        accessors.Add(AccessorDeclaration(SyntaxKind.GetAccessorDeclaration)
                            .WithExpressionBody(
                                ArrowExpressionClause(
                                    InvocationExpression(GenericName("GetPropertyPrimitive")
                                            .WithTypeArgumentList(
                                                TypeArgumentList(SingletonSeparatedList(GetTypeName(prop.Type)))))
                                        .WithArgumentList(ArgumentList(
                                            SingletonSeparatedList(Argument(IdentifierName(propertyIndexField)))))))
                            .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
                        );
                    }
                }
                else if (prop.Type.Kind == TypeRefKind.TypeInfo)
                {
                    var propType = prop.Type.TypeInfo;
                    if (propType.Kind == TypeInfoKind.CppGadget)
                    {
                        accessors.Add(AccessorDeclaration(SyntaxKind.GetAccessorDeclaration)
                            .WithExpressionBody(ArrowExpressionClause(ParseExpression(
                                $"GetPropertyQGadget<{GetQualifiedName(propType).ToFullString()}>({propertyIndexField})"
                            )))
                            .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
                        );
                    }
                    else
                    {
                        accessors.Add(AccessorDeclaration(SyntaxKind.GetAccessorDeclaration)
                            .WithExpressionBody(ArrowExpressionClause(ParseExpression(
                                $"GetPropertyQObject<{GetQualifiedName(propType).ToFullString()}>({propertyIndexField})"
                            )))
                            .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
                        );
                    }
                }
            }

            if (prop.IsWritable)
            {
                if (prop.Type.Kind == TypeRefKind.BuiltIn)
                {
                    if (prop.Type.BuiltIn == BuiltInType.String)
                    {
                        accessors.Add(AccessorDeclaration(SyntaxKind.SetAccessorDeclaration)
                            .WithExpressionBody(
                                ArrowExpressionClause(
                                    InvocationExpression(IdentifierName("SetPropertyQString"))
                                        .WithArgumentList(ArgumentList(
                                            SeparatedList(new[]
                                            {
                                                Argument(IdentifierName(propertyIndexField)),
                                                Argument(IdentifierName("value"))
                                            })))))
                            .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
                        );
                    }
                    else
                    {
                        accessors.Add(AccessorDeclaration(SyntaxKind.SetAccessorDeclaration)
                            .WithExpressionBody(
                                ArrowExpressionClause(
                                    InvocationExpression(GenericName("SetPropertyPrimitive")
                                            .WithTypeArgumentList(
                                                TypeArgumentList(SingletonSeparatedList(GetTypeName(prop.Type)))))
                                        .WithArgumentList(ArgumentList(
                                            SeparatedList(new[]
                                            {
                                                Argument(IdentifierName(propertyIndexField)),
                                                Argument(IdentifierName("value"))
                                            })))))
                            .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
                        );
                    }
                }
                else
                {
                    var propType = prop.Type.TypeInfo;
                    if (propType.Kind == TypeInfoKind.CppGadget)
                    {
                        accessors.Add(AccessorDeclaration(SyntaxKind.SetAccessorDeclaration)
                            .WithExpressionBody(ArrowExpressionClause(ParseExpression(
                                $"SetPropertyQGadget<{GetQualifiedName(propType).ToFullString()}>({propertyIndexField}, value)"
                            )))
                            .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
                        );
                    }
                    else
                    {
                        accessors.Add(AccessorDeclaration(SyntaxKind.SetAccessorDeclaration)
                            .WithExpressionBody(ArrowExpressionClause(ParseExpression(
                                $"SetPropertyQObject({propertyIndexField}, value)"
                            )))
                            .WithSemicolonToken(Token(SyntaxKind.SemicolonToken))
                        );
                    }
                }
            }

            var propName = _options.PascalCase ? Capitalize(prop.Name) : prop.Name;
            yield return PropertyDeclaration(GetTypeName(prop.Type), SanitizeIdentifier(propName))
                .WithModifiers(TokenList(Token(SyntaxKind.PublicKeyword)))
                .WithAccessorList(AccessorList(List(accessors)));
        }

        private string Capitalize(string text)
        {
            if (text.Length == 0 || char.IsUpper(text[0]))
            {
                return text;
            }

            return text.Substring(0, 1).ToUpperInvariant() + text.Substring(1);
        }

        private IEnumerable<MemberDeclarationSyntax> CreateEvent(MethodInfo signal)
        {
            var eventName = SanitizeIdentifier("On" + Capitalize(signal.Name));

            var signalParams = signal.Params.ToList();

            // Declare a static field to save the index of the signal
            var indexFieldName = GetSignalIndexField(signal);
            yield return CreateMetaObjectIndexField(indexFieldName);

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
                        signalParams.Select(p => GetTypeName(p.Type))
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

        private ExpressionSyntax CastVoidPtrToIntPtr(ExpressionSyntax voidPtr) =>
            CastExpression(IdentifierName("System.IntPtr"), voidPtr);

        private ExpressionSyntax DerefPointerToPrimitive(string name, ExpressionSyntax voidPtr) =>
            PrefixUnaryExpression(SyntaxKind.PointerIndirectionExpression,
                CastExpression(IdentifierName(name + "*"), voidPtr)
            );

        private ExpressionSyntax TransformExpressionNativeToManaged(ExpressionSyntax expression, TypeRef type)
        {
            // Transform ptrValue (a void*) to the target type
            switch (type.Kind)
            {
                case TypeRefKind.TypeInfo:
                    return ObjectCreationExpression(GetTypeName(type))
                        .WithArgumentList(
                            ArgumentList(SeparatedList(new[] {Argument(CastVoidPtrToIntPtr(expression))})));
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
                        case BuiltInType.String:
                            // We need special deserialization for String, sadly
                            return InvocationExpression(IdentifierName("QString_read"))
                                .WithArgumentList(ArgumentList(SeparatedList(new[] {Argument(expression)})));
                        default:
                            throw new ArgumentOutOfRangeException();
                    }
                default:
                    throw new ArgumentOutOfRangeException();
            }
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
            QObject
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
            yield return CreateMetaObjectIndexField(indexFieldName);

            IEnumerable<StatementSyntax> CreateBody()
            {
                var returnValueType = GetReturnValueHandling(method.ReturnType);

                var parameters = method.Params.ToList();

                // Declare the array to hold the argv for the call, index 0 is the return value pointer
                yield return ParseStatement($"void **argv = stackalloc void*[{parameters.Count + 1}];");


                if (returnValueType == ReturnValueHandling.String)
                {
                    yield return ParseStatement("using var result = new QStringArg(null);");
                    yield return ParseStatement("argv[0] = result.NativePointer;");
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

                for (var i = 0; i < parameters.Count; i++)
                {
                    var parameter = parameters[i];
                    var paramName = GetMethodParameterName(parameter, i);
                    string valueSource;
                    var parameterType = parameter.Type;
                    if (parameterType.Kind == TypeRefKind.BuiltIn)
                    {
                        if (parameterType.BuiltIn == BuiltInType.String)
                        {
                            // Introduce a temporary for the heap-allocated QString object
                            valueSource = paramName + "Temp.NativePointer";
                            yield return ParseStatement(
                                $"using var {paramName}Temp = new QGadgetBase.QStringArg({paramName});"
                            );
                        }
                        else
                        {
                            valueSource = "&" + paramName;
                        }
                    }
                    else if (parameterType.Kind == TypeRefKind.TypeInfo)
                    {
                        valueSource = paramName + ".NativePointer";
                    }
                    else
                    {
                        throw new ArgumentOutOfRangeException();
                    }

                    yield return ExpressionStatement(AssignmentExpression(
                        SyntaxKind.SimpleAssignmentExpression,
                        ParseExpression($"argv[{1 + i}]"),
                        ParseExpression(valueSource)
                    ));
                }

                yield return ParseStatement($"QObject_callMethod(_handle, {indexFieldName}, argv);");

                if (returnValueType == ReturnValueHandling.Primitive)
                {
                    yield return ReturnStatement(IdentifierName("result"));
                }
                else if (returnValueType == ReturnValueHandling.QObject)
                {
                    // Result should be an IntPtr containing the native OQbject*
                    yield return ParseStatement(
                        $"return QObjectBase.GetQObjectProxy<{GetQualifiedName(method.ReturnType.TypeInfo)}>(result);");
                }
                else if (returnValueType == ReturnValueHandling.String)
                {
                    yield return ParseStatement("return result.ToString();");
                }
            }

            var methodName = _options.PascalCase ? Capitalize(method.Name) : method.Name;

            yield return MethodDeclaration(GetTypeName(method.ReturnType), methodName)
                .WithModifiers(TokenList(Token(SyntaxKind.PublicKeyword), Token(SyntaxKind.UnsafeKeyword)))
                .WithParameterList(CreateParameterList(method))
                .WithBody(Block(CreateBody()));
        }

        private ParameterListSyntax CreateParameterList(MethodInfo method)
        {
            return ParameterList(SeparatedList(
                method.Params.Select((p, pIdx) => Parameter(Identifier(GetMethodParameterName(p, pIdx)))
                    .WithType(GetTypeName(p.Type)))
            ));
        }

        private string GetMethodParameterName(MethodParamInfo p, int index)
        {
            return SanitizeIdentifier(p.Name ?? $"arg{index + 1}");
        }

        private string GetPropertyIndexField(PropInfo prop)
        {
            return "_" + prop.Name + "Index";
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

        private string GetSignalIndexField(MethodInfo signal)
        {
            return "_" + signal.Name + "SignalIndex";
        }

        // Creates a private field holding the actual index to somoe property/method/signal from a metaobject
        // can only be determined safely at runtime...
        private FieldDeclarationSyntax CreateMetaObjectIndexField(string propertyIndexField)
        {
            return FieldDeclaration(
                    VariableDeclaration(
                            PredefinedType(
                                Token(SyntaxKind.IntKeyword)))
                        .WithVariables(
                            SingletonSeparatedList(
                                VariableDeclarator(Identifier(propertyIndexField))
                                    .WithInitializer(
                                        EqualsValueClause(
                                            PrefixUnaryExpression(
                                                SyntaxKind.UnaryMinusExpression,
                                                LiteralExpression(
                                                    SyntaxKind.NumericLiteralExpression,
                                                    Literal(1))))))))
                .WithModifiers(TokenList(Token(SyntaxKind.PrivateKeyword), Token(SyntaxKind.StaticKeyword)));
        }

        private string SanitizeIdentifier(string name)
        {
            // Prefix with @ if it's a keyword
            var isKeyword = SyntaxFacts.GetKeywordKind(name) != SyntaxKind.None
                            || SyntaxFacts.GetContextualKeywordKind(name) != SyntaxKind.None;
            return isKeyword ? "@" + name : name;
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
                SimpleBaseType(GetQualifiedName(parent.Value))
            ));
        }

        private IdentifierNameSyntax GetTypeName(TypeInfo typeInfo)
        {
            if (typeInfo.IsInlineComponent)
            {
                return IdentifierName(typeInfo.InlineComponentName);
            }

            var className = typeInfo.Name;
            // If the type refers to a .qml file, append a user-supplied suffix
            if (_options.QmlClassSuffix != null && typeInfo.Kind == TypeInfoKind.QmlQObject)
            {
                className += _options.QmlClassSuffix;
            }

            return IdentifierName(className);
        }

        private NameSyntax GetQualifiedName(TypeInfo typeInfo)
        {
            var className = typeInfo.Name;
            // If the type refers to a .qml file, append a user-supplied suffix
            if (_options.QmlClassSuffix != null && typeInfo.Kind == TypeInfoKind.QmlQObject)
            {
                className += _options.QmlClassSuffix;
            }

            var name = QualifiedName(GetNamespace(typeInfo), IdentifierName(className));
            if (typeInfo.IsInlineComponent)
            {
                name = QualifiedName(name, IdentifierName(typeInfo.InlineComponentName));
            }

            return name;
        }

        private NameSyntax GetNamespace(TypeInfo typeInfo)
        {
            var ns = typeInfo.QmlModule;
            // Namespace for types that come from direct .QML files
            if (ns == null)
            {
                ns = "QmlFiles";
            }

            return IdentifierName(ns);
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