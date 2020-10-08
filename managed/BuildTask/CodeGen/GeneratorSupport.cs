using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using QmlBuildTasks.Introspection;
using static Microsoft.CodeAnalysis.CSharp.SyntaxFactory;
using TypeInfo = QmlBuildTasks.Introspection.TypeInfo;

namespace QmlBuildTasks.CodeGen
{
    public class GeneratorSupport
    {
        private readonly Options _options;

        private readonly Dictionary<IntPtr, string> _enumNames = new Dictionary<IntPtr, string>();

        public GeneratorSupport(Options options)
        {
            _options = options;
        }

        public string GetEnumName(TypeInfo parentType, EnumInfo enumInfo)
        {
            if (_enumNames.TryGetValue(enumInfo.Handle, out var enumName))
            {
                return enumName;
            }

            enumName = enumInfo.Name;
            // annoying as hell, but if names are capitalized, we can introduce clashes with enums :|
            if (_options.PascalCase)
            {
                var clash = parentType.Props.Select(prop => Capitalize(prop.Name))
                    .Concat(parentType.Methods.Select(method => Capitalize(method.Name)))
                    .Contains(enumName);
                Console.WriteLine(
                    $"In type {parentType.Name}, enum {enumInfo.Name} clashes with a method or property.");
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

            _enumNames[enumInfo.Handle] = enumName;
            return enumName;
        }

        // Creates a private field holding the actual index to somoe property/method/signal from a metaobject
        // can only be determined safely at runtime...
        public FieldDeclarationSyntax CreateMetaObjectIndexField(string propertyIndexField)
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

        public string SanitizeIdentifier(string name)
        {
            // Prefix with @ if it's a keyword
            var isKeyword = SyntaxFacts.GetKeywordKind(name) != SyntaxKind.None
                            || SyntaxFacts.GetContextualKeywordKind(name) != SyntaxKind.None;
            return isKeyword ? "@" + name : name;
        }

        public string PascalifyName(string propName)
        {
            return _options.PascalCase ? Capitalize(propName) : propName;
        }

        public string Capitalize(string text)
        {
            if (text.Length == 0 || char.IsUpper(text[0]))
            {
                return text;
            }

            return text.Substring(0, 1).ToUpperInvariant() + text.Substring(1);
        }

        public NameSyntax GetQualifiedName(TypeInfo typeInfo)
        {
            var className = GetTypeName(typeInfo).ToFullString();

            var name = QualifiedName(GetNamespace(typeInfo), IdentifierName(className));
            if (typeInfo.IsInlineComponent)
            {
                name = QualifiedName(name, IdentifierName(typeInfo.InlineComponentName));
            }

            return name;
        }

        public NameSyntax GetNamespace(TypeInfo typeInfo)
        {
            var ns = typeInfo.QmlModule;
            // Namespace for types that come from direct .QML files
            if (ns == null)
            {
                ns = "QmlFiles";
            }

            return IdentifierName(ns);
        }

        public IdentifierNameSyntax GetTypeName(TypeInfo typeInfo)
        {
            if (typeInfo.IsInlineComponent)
            {
                return IdentifierName(typeInfo.InlineComponentName);
            }

            if (typeInfo.Kind == TypeInfoKind.CppGadget || typeInfo.Kind == TypeInfoKind.CppQObject)
            {
                return IdentifierName(typeInfo.MetaClassName);
            }

            var className = typeInfo.Name;
            // If the type refers to a .qml file, append a user-supplied suffix
            if (_options.QmlClassSuffix != null && typeInfo.Kind == TypeInfoKind.QmlQObject)
            {
                className += _options.QmlClassSuffix;
            }

            return IdentifierName(className);
        }

        public TypeSyntax GetTypeName(TypeRef typeRef)
        {
            return typeRef.Kind switch
            {
                TypeRefKind.Void => PredefinedType(Token(SyntaxKind.VoidKeyword)),
                TypeRefKind.QmlList => GenericName("QmlList").WithTypeArgumentList(
                    TypeArgumentList(SingletonSeparatedList<TypeSyntax>(GetQualifiedName(typeRef.TypeInfo)))
                ),
                TypeRefKind.TypeInfo => GetQualifiedName(typeRef.TypeInfo),
                TypeRefKind.TypeInfoEnum => GetEnumTypeRefName(typeRef.TypeInfo, typeRef.EnumName),
                TypeRefKind.BuiltIn => typeRef.BuiltIn switch
                {
                    BuiltInType.Bool => PredefinedType(Token(SyntaxKind.BoolKeyword)),
                    BuiltInType.Int32 => PredefinedType(Token(SyntaxKind.IntKeyword)),
                    BuiltInType.UInt32 => PredefinedType(Token(SyntaxKind.UIntKeyword)),
                    BuiltInType.Int64 => PredefinedType(Token(SyntaxKind.LongKeyword)),
                    BuiltInType.UInt64 => PredefinedType(Token(SyntaxKind.ULongKeyword)),
                    BuiltInType.Double => PredefinedType(Token(SyntaxKind.DoubleKeyword)),
                    BuiltInType.Char => PredefinedType(Token(SyntaxKind.CharKeyword)),
                    BuiltInType.String => PredefinedType(Token(SyntaxKind.StringKeyword)),
                    BuiltInType.ByteArray => ParseTypeName("byte[]"),
                    BuiltInType.DateTime => ParseTypeName("System.DateTime"),
                    BuiltInType.Date => ParseTypeName("System.DateTime"),
                    BuiltInType.Time => ParseTypeName("System.DateTime"),
                    BuiltInType.Color => ParseTypeName("System.Drawing.Color"),
                    BuiltInType.Size => ParseTypeName("System.Drawing.Size"),
                    BuiltInType.SizeFloat => ParseTypeName("System.Drawing.SizeF"),
                    BuiltInType.Rectangle => ParseTypeName("System.Drawing.Rectangle"),
                    BuiltInType.RectangleFloat => ParseTypeName("System.Drawing.RectangleF"),
                    BuiltInType.Point => ParseTypeName("System.Drawing.Point"),
                    BuiltInType.PointFloat => ParseTypeName("System.Drawing.PointF"),
                    BuiltInType.Url => ParseTypeName("string"),
                    BuiltInType.OpaquePointer => ParseTypeName("System.IntPtr"),
                    BuiltInType.CompletionSource => ParseTypeName("System.IntPtr"),
                    _ => throw new ArgumentOutOfRangeException(typeRef.BuiltIn.ToString())
                },
                _ => throw new ArgumentOutOfRangeException(typeRef.Kind.ToString())
            };
        }

        private TypeSyntax GetEnumTypeRefName(TypeInfo enclosingType, string localEnumName)
        {
            // We have to find the enum in the enclosing type to access the enum name mapping
            var enumName = localEnumName;

            foreach (var enclosingTypeEnum in enclosingType.Enums)
            {
                if (enclosingTypeEnum.Name == localEnumName)
                {
                    enumName = GetEnumName(enclosingType, enclosingTypeEnum);
                    break;
                }
            }

            return QualifiedName(GetQualifiedName(enclosingType), IdentifierName(enumName));
        }

        public ExpressionSyntax TransformExpressionNativeToManaged(ExpressionSyntax expression, TypeRef type)
        {
            // Transform expression (a void*) to the target type
            switch (type.Kind)
            {
                case TypeRefKind.TypeInfo:
                    return ObjectCreationExpression(GetTypeName(type))
                        .WithArgumentList(
                            ArgumentList(SeparatedList(new[] {Argument(CastVoidPtrToIntPtr(expression))})));
                case TypeRefKind.TypeInfoEnum:
                    // Storage representation should be the same
                    return DerefPointerToPrimitive(GetTypeName(type).ToFullString(), expression);
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

        private ExpressionSyntax DerefPointerToPrimitive(string name, ExpressionSyntax voidPtr) =>
            PrefixUnaryExpression(SyntaxKind.PointerIndirectionExpression,
                CastExpression(IdentifierName(name + "*"), voidPtr)
            );

        private ExpressionSyntax CastVoidPtrToIntPtr(ExpressionSyntax voidPtr) =>
            PrefixUnaryExpression(SyntaxKind.PointerIndirectionExpression,
                CastExpression(IdentifierName("System.IntPtr*"), voidPtr)
            );
    }
}