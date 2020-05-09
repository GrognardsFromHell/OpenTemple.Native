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
    }
}