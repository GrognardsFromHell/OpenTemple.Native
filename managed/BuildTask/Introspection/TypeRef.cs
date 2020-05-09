using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security;
using OpenTemple.Interop;

namespace QmlBuildTasks.Introspection
{
    public readonly struct TypeRef
    {
        private readonly IntPtr _handle;

        public TypeRef(IntPtr handle)
        {
            _handle = handle;
        }

        public TypeRefKind Kind => TypeRef_kind(_handle);

        public BuiltInType BuiltIn
        {
            get
            {
                Trace.Assert(Kind == TypeRefKind.BuiltIn);
                return TypeRef_builtIn(_handle);
            }
        }

        public string EnumName
        {
            get
            {
                Trace.Assert(Kind == TypeRefKind.TypeInfoEnum);
                return TypeRef_enumName(_handle);
            }
        }

        public TypeInfo TypeInfo
        {
            get
            {
                Trace.Assert(Kind == TypeRefKind.TypeInfo || Kind == TypeRefKind.QmlList || Kind == TypeRefKind.TypeInfoEnum);
                var handle = TypeRef_typeInfo(_handle);
                if (handle == IntPtr.Zero)
                {
                    throw new InvalidOperationException();
                }

                return new TypeInfo(handle);
            }
        }

        /// <summary>
        /// True if this refers to a type that has a 1:1 representation between the managed and unmanaged side.
        /// That means the C# type satisfies the "unamanged" generic constraint in C#.
        /// </summary>
        public bool IsUnmanaged
        {
            get
            {
                if (Kind == TypeRefKind.BuiltIn)
                {
                    return BuiltIn switch
                    {
                        BuiltInType.Bool => true,
                        BuiltInType.Int32 => true,
                        BuiltInType.UInt32 => true,
                        BuiltInType.Int64 => true,
                        BuiltInType.UInt64 => true,
                        BuiltInType.Double => true,
                        BuiltInType.Char => true,
                        BuiltInType.OpaquePointer => true,
                        _ => false
                    };
                }

                if (Kind == TypeRefKind.TypeInfoEnum)
                {
                    return true;
                }
                return false;
            }
        }

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern TypeRefKind TypeRef_kind(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern BuiltInType TypeRef_builtIn(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        [return:MarshalAs(UnmanagedType.LPStr)]
        private static extern string TypeRef_enumName(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr TypeRef_typeInfo(IntPtr handle);
    }

    public enum TypeRefKind
    {
        Void = 0,
        TypeInfo,
        BuiltIn,
        QmlList,
        TypeInfoEnum
    }

    public enum BuiltInType
    {
        Bool = 0,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Double,
        Char,
        String,
        ByteArray,
        DateTime,
        Date,
        Time,
        Color,
        Size,
        SizeFloat,
        Rectangle,
        RectangleFloat,
        Point,
        PointFloat,
        Url,
        OpaquePointer,
        CompletionSource
    }
}