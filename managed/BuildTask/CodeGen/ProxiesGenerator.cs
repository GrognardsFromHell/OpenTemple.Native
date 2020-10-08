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

namespace QmlBuildTasks.CodeGen
{
    public class ProxiesGenerator
    {
        private readonly Options _options;

        private readonly PropertyGenerator _properties;

        private readonly MethodGenerator _methods;

        private readonly EventGenerator _events;

        private readonly GeneratorSupport _support;

        public ProxiesGenerator(Options options)
        {
            _options = options;
            _support = new GeneratorSupport(options);
            _properties = new PropertyGenerator(options, _support);
            _methods = new MethodGenerator(options, _support);
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
                foreach (var syntax in _methods.CreateMethod(method))
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
                var indexField = _methods.GetMethodIndexField(method);
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