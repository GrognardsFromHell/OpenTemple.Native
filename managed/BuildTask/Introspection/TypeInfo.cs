using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security;
using OpenTemple.Interop;

namespace QmlBuildTasks.Introspection
{
    public enum TypeInfoKind
    {
        CppQObject = 0,
        CppGadget = 1,
        QmlQObject = 2
    }

    public readonly struct TypeInfo
    {
        private readonly IntPtr _handle;

        public TypeInfo(IntPtr handle)
        {
            _handle = handle;
        }

        public TypeInfoKind Kind => TypeInfo_kind(_handle);

        public string MetaClassName => TypeInfo_metaClassName(_handle);

        public string Name => TypeInfo_name(_handle);

        public string QmlModule => TypeInfo_qmlModule(_handle);

        public int QmlMajorVersion => TypeInfo_qmlMajorVersion(_handle);

        public int QmlMinorVersion => TypeInfo_qmlMinorVersion(_handle);

        public bool IsQmlCreatable => TypeInfo_qmlCreatable(_handle);

        public string QmlSourceUrl => TypeInfo_qmlSourceUrl(_handle);

        public TypeInfo? EnclosingType
        {
            get
            {
                var enclosingTypePtr = TypeInfo_enclosingType(_handle);
                return enclosingTypePtr != IntPtr.Zero ? (TypeInfo?) new TypeInfo(enclosingTypePtr) : null;
            }
        }

        public bool IsInlineComponent => TypeInfo_enclosingType(_handle) != IntPtr.Zero;

        public string InlineComponentName => TypeInfo_inlineComponentName(_handle);

        public TypeInfo? Parent
        {
            get
            {
                var parentHandle = TypeInfo_parent(_handle);
                TypeInfo? result = null;
                if (parentHandle != IntPtr.Zero)
                {
                    result = new TypeInfo(parentHandle);
                }

                return result;
            }
        }

        public IEnumerable<PropInfo> Props
        {
            get
            {
                var count = TypeInfo_props_size(_handle);
                for (var i = 0; i < count; i++)
                {
                    yield return new PropInfo(TypeInfo_props(_handle, i));
                }
            }
        }

        public IEnumerable<MethodInfo> Signals
        {
            get
            {
                var count = TypeInfo_signals_size(_handle);
                for (var i = 0; i < count; i++)
                {
                    yield return new MethodInfo(TypeInfo_signals(_handle, i));
                }
            }
        }

        public IEnumerable<MethodInfo> Methods
        {
            get
            {
                var count = TypeInfo_methods_size(_handle);
                for (var i = 0; i < count; i++)
                {
                    yield return new MethodInfo(TypeInfo_methods(_handle, i));
                }
            }
        }

        public IEnumerable<MethodInfo> Constructors
        {
            get
            {
                var count = TypeInfo_constructors_size(_handle);
                for (var i = 0; i < count; i++)
                {
                    yield return new MethodInfo(TypeInfo_constructors(_handle, i));
                }
            }
        }

        public IEnumerable<EnumInfo> Enums
        {
            get
            {
                var count = TypeInfo_enums_size(_handle);
                for (var i = 0; i < count; i++)
                {
                    yield return new EnumInfo(TypeInfo_enums(_handle, i));
                }
            }
        }

        public bool Equals(TypeInfo other)
        {
            return _handle.Equals(other._handle);
        }

        public override bool Equals(object obj)
        {
            return obj is TypeInfo other && Equals(other);
        }

        public override int GetHashCode()
        {
            return _handle.GetHashCode();
        }

        public static bool operator ==(TypeInfo left, TypeInfo right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(TypeInfo left, TypeInfo right)
        {
            return !left.Equals(right);
        }

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern TypeInfoKind TypeInfo_kind(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern string TypeInfo_metaClassName(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern string TypeInfo_name(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern string TypeInfo_qmlModule(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int TypeInfo_qmlMajorVersion(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int TypeInfo_qmlMinorVersion(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool TypeInfo_qmlCreatable(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern string TypeInfo_qmlSourceUrl(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr TypeInfo_enclosingType(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern string TypeInfo_inlineComponentName(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr TypeInfo_parent(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int TypeInfo_props_size(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr TypeInfo_props(IntPtr handle, int index);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int TypeInfo_signals_size(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr TypeInfo_signals(IntPtr handle, int index);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int TypeInfo_methods_size(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr TypeInfo_methods(IntPtr handle, int index);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int TypeInfo_constructors_size(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr TypeInfo_constructors(IntPtr handle, int index);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int TypeInfo_enums_size(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr TypeInfo_enums(IntPtr handle, int index);
    }
}