using System;
using System.Collections.Generic;
using System.Reflection.Metadata;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTemple.Interop
{
    [SuppressUnmanagedCodeSecurity]
    public abstract class QObjectBase : QGadgetBase
    {
        // ReSharper disable once PrivateFieldCanBeConvertedToLocalVariable
        private static readonly DelegateSlotFree ReleaseDelegate;

        // Maps QMetaObject memory addresses to the corresponding proxy type factories
        private static readonly Dictionary<IntPtr, Func<IntPtr, QObjectBase>> MetaObjectCache =
            new Dictionary<IntPtr, Func<IntPtr, QObjectBase>>();

        static QObjectBase()
        {
            ReleaseDelegate = HandleSlotRelease;
            DelegateSlotObject_setCallbacks(ReleaseDelegate);
        }

        protected QObjectBase(IntPtr handle) : base(handle)
        {
        }

        public static T GetQObjectProxy<T>(IntPtr qObjectPtr) where T : QObjectBase
        {
            RuntimeHelpers.RunClassConstructor(typeof(T).TypeHandle);

            if (qObjectPtr == IntPtr.Zero)
            {
                return null;
            }

            // Try looking up the correct proxy type based on the meta object pointer
            var metaObject = QObject_metaObject(qObjectPtr);
            if (metaObject == IntPtr.Zero)
            {
                throw new InvalidOperationException("Given QObject has no metaObject!");
            }

            Func<IntPtr, QObjectBase> constructor;
            lock (MetaObjectCache)
            {
                if (!MetaObjectCache.TryGetValue(metaObject, out constructor))
                {
                    // We'll need to try and figure out the constructor based on the live object :-(
                    if (!QObject_objectRuntimeType(qObjectPtr, out var isQmlFile, out var metaClassName,
                        out var qmlSourceUrl, out var qmlInlineComponent))
                    {
                        // TODO: We _could_ return a QObject proxy
                        throw new InvalidOperationException($"Failed to determine runtime type of object {qObjectPtr}");
                    }

                    if (!QObjectTypeRegistry.TryGetConstructor(isQmlFile, metaClassName, qmlSourceUrl,
                        qmlInlineComponent,
                        out constructor))
                    {
                        // TODO: We _could_ return a QObject proxy
                        throw new InvalidOperationException(
                            $"Failed to find constructor for {metaClassName} {qmlSourceUrl} {qmlInlineComponent}");
                    }

                    // Cache the constructor based on the meta object's ptr identity
                    MetaObjectCache[metaObject] = constructor;
                }
            }

            // We found one and can construct it right away
            return (T) constructor(qObjectPtr);
        }

        private static void HandleSlotRelease(GCHandle delegateHandle)
        {
            delegateHandle.Free();
        }

        protected static IntPtr GetQmlMetaObject(IntPtr exampleInstance, string sourceUrl, string inlineComponentName)
        {
            if (!QMetaType_resolveQmlType(exampleInstance, sourceUrl, inlineComponentName, out var metaTypeId,
                out var metaObject))
            {
                throw new ArgumentException("Failed to resolve QML type from source url " + sourceUrl);
            }

            return metaObject;
        }

        protected void AddSignalHandler(int signalIndex, Delegate @delegate, DelegateSlotCallback dispatcher)
        {
            var delegateHandle = GCHandle.Alloc(@delegate);
            if (!QObject_connect(_handle, signalIndex, delegateHandle, dispatcher))
            {
                delegateHandle.Free();
                throw new InvalidOperationException();
            }
        }

        protected void RemoveSignalHandler(int signalIndex, Delegate @delegate)
        {
            throw new NotImplementedException();
        }

        // Creates a new QObject instance
        protected static IntPtr CreateInstance(IntPtr metaObject)
        {
            var obj = QObject_create(metaObject);
            if (obj == IntPtr.Zero)
            {
                throw new InvalidOperationException("Failed to create object.");
            }

            return obj;
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr QObject_create(IntPtr metaObject);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern bool DelegateSlotObject_setCallbacks(
            DelegateSlotFree release
        );

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool
            QObject_connect(IntPtr instance, int signalIndex, GCHandle delegateHandle, DelegateSlotCallback dispatcher);

        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr QObject_metaObject(IntPtr instance);

        [DllImport(OpenTempleLib.Path)]
        private static extern bool QObject_objectRuntimeType(
            IntPtr instance,
            out bool isQmlFile,
            [MarshalAs(UnmanagedType.LPStr)]
            out string metaClassName,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string qmlSourceUrl,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string qmlInlineComponent
        );

        // Reads the UTF-16 string from a pointer to a QString instance
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        protected static extern unsafe string QString_read(void* instance);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern bool
            QMetaType_resolveQmlType(IntPtr exampleInstance, string sourceUrl, string inlineComponentName,
                out int metaTypeId,
                out IntPtr metaObject);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        protected unsafe delegate void DelegateSlotCallback(GCHandle delegateHandle, void** args);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void DelegateSlotFree(GCHandle delegateHandle);
    }

    /// <summary>
    ///     Keeps track of the subclasses of QObject that were known when the code was generated.
    ///     This is required to map a yet-unseen QObject to the proper generated .NET class.
    ///     We cannot prepopulate this with meta type ids, because QML files for instance may
    ///     not be loaded at program startup and have not been assigned a type id yet.
    /// </summary>
    public static class QObjectTypeRegistry
    {
        private static readonly Dictionary<RuntimeTypeId, Func<IntPtr, QObjectBase>> Types =
            new Dictionary<RuntimeTypeId, Func<IntPtr, QObjectBase>>();

        public static void RegisterQmlFile(string sourceUrl, string inlineComponentName,
            Func<IntPtr, QObjectBase> constructor)
        {
            Types[new RuntimeTypeId(true, null, sourceUrl, inlineComponentName)] = constructor;
        }

        public static void RegisterMetaClass(string className, Func<IntPtr, QObjectBase> constructor)
        {
            Types[new RuntimeTypeId(false, className, null, null)] = constructor;
        }

        public static bool TryGetConstructor(bool isQmlFile, string className, string qmlSourceUrl,
            string inlineComponentName,
            out Func<IntPtr, QObjectBase> constructor)
        {
            var typeId = new RuntimeTypeId(isQmlFile, className, qmlSourceUrl, inlineComponentName);
            return Types.TryGetValue(typeId, out constructor);
        }

        private readonly struct RuntimeTypeId
        {
            public readonly bool IsQmlFile;
            public readonly string ClassName;
            public readonly string QmlSourceUrl;
            public readonly string InlineComponentName;

            public RuntimeTypeId(bool isQmlFile, string className, string qmlSourceUrl, string inlineComponentName)
            {
                IsQmlFile = isQmlFile;
                ClassName = className;
                QmlSourceUrl = qmlSourceUrl;
                InlineComponentName = inlineComponentName;
            }

            public bool Equals(RuntimeTypeId other)
            {
                return IsQmlFile == other.IsQmlFile && ClassName == other.ClassName &&
                       QmlSourceUrl == other.QmlSourceUrl && InlineComponentName == other.InlineComponentName;
            }

            public override bool Equals(object obj)
            {
                return obj is RuntimeTypeId other && Equals(other);
            }

            public override int GetHashCode()
            {
                return HashCode.Combine(IsQmlFile, ClassName, QmlSourceUrl, InlineComponentName);
            }

            public static bool operator ==(RuntimeTypeId left, RuntimeTypeId right)
            {
                return left.Equals(right);
            }

            public static bool operator !=(RuntimeTypeId left, RuntimeTypeId right)
            {
                return !left.Equals(right);
            }
        }
    }

    [SuppressUnmanagedCodeSecurity]
    public abstract class QGadgetBase
    {
        protected IntPtr _handle;

        protected QGadgetBase(IntPtr handle)
        {
            _handle = handle;
        }

        public IntPtr Handle => _handle;

        public unsafe void* NativePointer => _handle.ToPointer();

        protected unsafe void SetPropertyQString(int index, string value)
        {
            ReadOnlySpan<char> strval = value;
            fixed (char* strptr = strval)
            {
                if (!QObject_setPropertyQString(_handle, index, strptr, strval.Length))
                {
                    throw new InvalidOperationException("Couldn't set property " + index);
                }
            }
        }

        protected T GetPropertyQObject<T>(int index) where T : QObjectBase
        {
            var handle = GetPropertyPrimitive<IntPtr>(index);
            return QObjectBase.GetQObjectProxy<T>(handle);
        }

        protected unsafe void SetPropertyQObject(int index, QObjectBase value)
        {
            var nativePointer = value?.Handle ?? IntPtr.Zero;
            if (!QObject_setProperty(_handle, index, &nativePointer))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected T GetPropertyQGadget<T>(int index) where T : QGadgetBase
        {
            throw new NotImplementedException();
        }

        protected void SetPropertyQGadget<T>(int index, T value) where T : QGadgetBase
        {
            throw new NotImplementedException();
        }

        protected unsafe T GetPropertyPrimitive<T>(int index) where T : unmanaged
        {
            T value;
            if (!QObject_getProperty(_handle, index, &value))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return value;
        }

        protected unsafe void SetPropertyPrimitive<T>(int index, T value) where T : unmanaged
        {
            if (!QObject_setProperty(_handle, index, &value))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected unsafe string GetPropertyQString(int index)
        {
            if (!QObject_getPropertyQString(_handle, index, out var strptr, out var strlen))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            if (strptr == null)
            {
                return null;
            }

            return new string(strptr, 0, strlen);
        }

        protected static IntPtr GetCppMetaObject(string className)
        {
            if (!QMetaType_resolveCppType(className + "*", out var metaTypeId, out var metaObject))
            {
                throw new ArgumentException("Failed to resolve Qt type " + className);
            }

            return metaObject;
        }

        protected static IntPtr GetQmlCppMetaObject(string metaClassName, string typeName, string moduleName,
            int majorVersion, int minorVersion)
        {
            if (!QMetaType_resolveQmlCppType(metaClassName, typeName, moduleName, majorVersion, minorVersion,
                out var metaTypeId,
                out var metaObject))
            {
                throw new ArgumentException("Failed to resolve Qt type " + moduleName + "/" + typeName);
            }

            return metaObject;
        }

        protected static void FindMetaObjectProperty(IntPtr metaObject, string name, out int propertyIndex)
        {
            propertyIndex = QMetaObject_indexOfProperty(metaObject, name);

            if (propertyIndex < 0)
            {
                throw new ArgumentException("Failed to find property " + name);
            }
        }

        protected static void FindMetaObjectMethod(IntPtr metaObject, string name, out int methodIndex)
        {
            methodIndex = QMetaObject_indexOfMethod(metaObject, name);

            if (methodIndex < 0)
            {
                throw new ArgumentException("Failed to find method " + name);
            }
        }

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern unsafe bool QObject_setPropertyQString(IntPtr target, int idx, char* value, int len);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern unsafe bool QObject_getPropertyQString(IntPtr target, int idx, out char* value,
            out int len);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern unsafe bool QObject_setProperty(IntPtr target, int idx, void* value);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern unsafe bool QObject_getProperty(IntPtr target, int idx, void* value);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        protected static extern unsafe bool QObject_callMethod(IntPtr target, int idx, void** args);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Ansi)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool
            QMetaType_resolveCppType(string className, out int metaTypeId, out IntPtr metaObject);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool
            QMetaType_resolveQmlCppType([MarshalAs(UnmanagedType.LPStr)]
                string metaClassName, string typeName, string moduleName, int majorVersion, int minorVersion,
                out int metaTypeId,
                out IntPtr metaObject);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Ansi)]
        private static extern int
            QMetaObject_indexOfProperty(IntPtr metaObject, string name);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Ansi)]
        private static extern int
            QMetaObject_indexOfMethod(IntPtr metaObject, string name);

        [StructLayout(LayoutKind.Sequential)]
        protected readonly ref struct QStringArg
        {
            private readonly IntPtr _handle;

            public QStringArg(string text) : this()
            {
                if (text == null)
                {
                    _handle = QString_create_null();
                }
                else
                {
                    _handle = QString_create(text, text.Length);
                }
            }

            public unsafe void* NativePointer => _handle.ToPointer();

            // This is possible since C#8
            public void Dispose()
            {
                QString_delete(_handle);
            }

            public override string ToString()
            {
                return QString_read(_handle);
            }

            [SuppressUnmanagedCodeSecurity]
            [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
            private static extern IntPtr QString_create(string charsPtr, int charsLength);

            [SuppressUnmanagedCodeSecurity]
            [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
            private static extern string QString_read(IntPtr handle);

            [SuppressUnmanagedCodeSecurity]
            [DllImport(OpenTempleLib.Path)]
            private static extern IntPtr QString_create_null();

            [SuppressUnmanagedCodeSecurity]
            [DllImport(OpenTempleLib.Path)]
            private static extern void QString_delete(IntPtr handle);
        }
    }
}