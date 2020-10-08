using System;
using System.Collections.Generic;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using QmlBuildTasks.Introspection;
using static Microsoft.CodeAnalysis.CSharp.SyntaxFactory;

namespace QmlBuildTasks.CodeGen
{
    public class PropertyGenerator
    {
        private readonly GeneratorSupport _support;

        public PropertyGenerator(Options options, GeneratorSupport support)
        {
            _support = support;
        }

        public string GetPropertyIndexField(PropInfo prop)
        {
            return "_" + prop.Name + "Index";
        }

        public IEnumerable<MemberDeclarationSyntax> CreateProperty(PropInfo prop)
        {
            var accessors = new List<AccessorDeclarationSyntax>();

            var propertyIndexField = GetPropertyIndexField(prop);

            yield return _support.CreateMetaObjectIndexField(propertyIndexField);

            if (prop.IsReadable)
            {
                accessors.Add(CreatePropertyGetAccessor(prop, propertyIndexField));
            }

            if (prop.IsWritable)
            {
                accessors.Add(CreatePropertySetAccessor(prop, propertyIndexField));
            }

            var propName = _support.PascalifyName(prop.Name);
            yield return PropertyDeclaration(_support.GetTypeName(prop.Type), _support.SanitizeIdentifier(propName))
                .WithModifiers(TokenList(Token(SyntaxKind.PublicKeyword)))
                .WithAccessorList(AccessorList(List(accessors)));
        }

        private AccessorDeclarationSyntax CreatePropertySetAccessor(PropInfo prop, string propertyIndexField)
        {
            if (prop.Type.IsUnmanaged)
            {
                return AccessorDeclaration(SyntaxKind.SetAccessorDeclaration)
                    .WithExpressionBody(
                        ArrowExpressionClause(
                            InvocationExpression(GenericName("SetPropertyPrimitive")
                                    .WithTypeArgumentList(
                                        TypeArgumentList(SingletonSeparatedList(_support.GetTypeName(prop.Type)))))
                                .WithArgumentList(ArgumentList(
                                    SeparatedList(new[]
                                    {
                                        Argument(IdentifierName(propertyIndexField)),
                                        Argument(IdentifierName("value"))
                                    })))))
                    .WithSemicolonToken(Token(SyntaxKind.SemicolonToken));
            }
            else if (prop.Type.Kind == TypeRefKind.BuiltIn)
            {
                var specializedPropertySetter = prop.Type.BuiltIn switch
                {
                    BuiltInType.String => "SetPropertyQString",
                    BuiltInType.ByteArray => "SetPropertyQByteArray",
                    BuiltInType.DateTime => "SetPropertyQDateTime",
                    BuiltInType.Date => "SetPropertyQDate",
                    BuiltInType.Time => "SetPropertyQTime",
                    BuiltInType.Color => "SetPropertyQColor",
                    BuiltInType.Size => "SetPropertyQSize",
                    BuiltInType.SizeFloat => "SetPropertyQSizeF",
                    BuiltInType.Rectangle => "SetPropertyQRect",
                    BuiltInType.RectangleFloat => "SetPropertyQRectF",
                    BuiltInType.Point => "SetPropertyQPoint",
                    BuiltInType.PointFloat => "SetPropertyQPointF",
                    BuiltInType.Url => "SetPropertyQUrl",
                    _ => throw new ArgumentOutOfRangeException("prop.Type.BuiltIn", prop.Type.BuiltIn,
                        "Unknown built-in type")
                };
                return AccessorDeclaration(SyntaxKind.SetAccessorDeclaration)
                    .WithExpressionBody(
                        ArrowExpressionClause(
                            InvocationExpression(IdentifierName(specializedPropertySetter))
                                .WithArgumentList(ArgumentList(
                                    SeparatedList(new[]
                                    {
                                        Argument(IdentifierName(propertyIndexField)),
                                        Argument(IdentifierName("value"))
                                    })))))
                    .WithSemicolonToken(Token(SyntaxKind.SemicolonToken));
            }
            else
            {
                var propType = prop.Type.TypeInfo;
                if (propType.Kind == TypeInfoKind.CppGadget)
                {
                    return AccessorDeclaration(SyntaxKind.SetAccessorDeclaration)
                        .WithExpressionBody(ArrowExpressionClause(ParseExpression(
                            $"SetPropertyQGadget<{_support.GetQualifiedName(propType).ToFullString()}>({propertyIndexField}, value)"
                        )))
                        .WithSemicolonToken(Token(SyntaxKind.SemicolonToken));
                }
                else
                {
                    return AccessorDeclaration(SyntaxKind.SetAccessorDeclaration)
                        .WithExpressionBody(ArrowExpressionClause(ParseExpression(
                            $"SetPropertyQObject({propertyIndexField}, value)"
                        )))
                        .WithSemicolonToken(Token(SyntaxKind.SemicolonToken));
                }
            }
        }

        private AccessorDeclarationSyntax CreatePropertyGetAccessor(PropInfo prop, string propertyIndexField)
        {
            if (prop.Type.IsUnmanaged)
            {
                return AccessorDeclaration(SyntaxKind.GetAccessorDeclaration)
                    .WithExpressionBody(
                        ArrowExpressionClause(
                            InvocationExpression(GenericName("GetPropertyPrimitive")
                                    .WithTypeArgumentList(
                                        TypeArgumentList(SingletonSeparatedList(_support.GetTypeName(prop.Type)))))
                                .WithArgumentList(ArgumentList(
                                    SingletonSeparatedList(Argument(IdentifierName(propertyIndexField)))))))
                    .WithSemicolonToken(Token(SyntaxKind.SemicolonToken));
            }
            else if (prop.Type.Kind == TypeRefKind.BuiltIn)
            {
                var specializedPropertyGetter = prop.Type.BuiltIn switch
                {
                    BuiltInType.String => "GetPropertyQString",
                    BuiltInType.ByteArray => "GetPropertyQByteArray",
                    BuiltInType.DateTime => "GetPropertyQDateTime",
                    BuiltInType.Date => "GetPropertyQDate",
                    BuiltInType.Time => "GetPropertyQTime",
                    BuiltInType.Color => "GetPropertyQColor",
                    BuiltInType.Size => "GetPropertyQSize",
                    BuiltInType.SizeFloat => "GetPropertyQSizeF",
                    BuiltInType.Rectangle => "GetPropertyQRect",
                    BuiltInType.RectangleFloat => "GetPropertyQRectF",
                    BuiltInType.Point => "GetPropertyQPoint",
                    BuiltInType.PointFloat => "GetPropertyQPointF",
                    BuiltInType.Url => "GetPropertyQUrl",
                    _ => throw new ArgumentOutOfRangeException("prop.Type.BuiltIn", prop.Type.BuiltIn,
                        "Unknown built-in type")
                };

                return AccessorDeclaration(SyntaxKind.GetAccessorDeclaration)
                    .WithExpressionBody(
                        ArrowExpressionClause(
                            InvocationExpression(IdentifierName(specializedPropertyGetter))
                                .WithArgumentList(ArgumentList(
                                    SingletonSeparatedList(Argument(IdentifierName(propertyIndexField)))))))
                    .WithSemicolonToken(Token(SyntaxKind.SemicolonToken));
            }
            else if (prop.Type.Kind == TypeRefKind.TypeInfo || prop.Type.Kind == TypeRefKind.QmlList)
            {
                var isList = prop.Type.Kind == TypeRefKind.QmlList;
                var propType = prop.Type.TypeInfo;
                string methodName;
                if (propType.Kind == TypeInfoKind.CppGadget)
                {
                    if (isList)
                    {
                        throw new InvalidOperationException("QGadget's cannot have list properties.");
                    }
                    methodName = "GetPropertyQGadget";
                }
                else
                {
                    methodName = isList ? "GetPropertyQmlList" : "GetPropertyQObject";
                }

                return AccessorDeclaration(SyntaxKind.GetAccessorDeclaration)
                    .WithExpressionBody(ArrowExpressionClause(ParseExpression(
                        $"{methodName}<{_support.GetQualifiedName(propType).ToFullString()}>({propertyIndexField})"
                    )))
                    .WithSemicolonToken(Token(SyntaxKind.SemicolonToken));
            }
            else
            {
                throw new ArgumentOutOfRangeException("prop.Type.Kind", prop.Type.Kind, "Unknown type kind");
            }
        }

        /// <summary>
        /// Creates statements that initialize this properties static state (such as it's index in the meta object).
        /// </summary>
        public IEnumerable<StatementSyntax> CreateInitializer(PropInfo prop, string metaObjectVar)
        {
            var propertyIndexField = GetPropertyIndexField(prop);
            yield return ParseStatement(
                $"FindMetaObjectProperty({metaObjectVar}, \"{prop.Name}\", out {propertyIndexField});");
        }
    }
}