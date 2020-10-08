using System;
using System.Collections.Generic;
using System.Drawing;
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

        public bool IsJavaScriptOwned
        {
            get => QObject_getObjectOwnership(Handle);
            set => QObject_setObjectOwnership(Handle, value);
        }

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
        [return: MarshalAs(UnmanagedType.I1)]
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

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool
            QMetaType_resolveQmlType(IntPtr exampleInstance, string sourceUrl, string inlineComponentName,
                out int metaTypeId,
                out IntPtr metaObject);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        protected unsafe delegate void DelegateSlotCallback(GCHandle delegateHandle, void** args);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void DelegateSlotFree(GCHandle delegateHandle);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern void QObject_setObjectOwnership(IntPtr obj, [MarshalAs(UnmanagedType.I1)]
            bool jsOwnership);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getObjectOwnership(IntPtr obj);

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

        protected bool Equals(QGadgetBase other)
        {
            return _handle.Equals(other._handle);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj.GetType() != this.GetType())
            {
                return false;
            }

            return Equals((QGadgetBase) obj);
        }

        public override int GetHashCode()
        {
            return _handle.GetHashCode();
        }

        public static bool operator ==(QGadgetBase left, QGadgetBase right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(QGadgetBase left, QGadgetBase right)
        {
            return !Equals(left, right);
        }

        #region Native Structure Property Support

        protected unsafe void SetPropertyQByteArray(int index, ReadOnlySpan<byte> value)
        {
            fixed (byte* valuePtr = value)
            {
                if (!QObject_setPropertyQByteArray(_handle, index, valuePtr, value.Length))
                {
                    throw new InvalidOperationException("Couldn't set property " + index);
                }
            }
        }

        protected byte[] GetPropertyQByteArray(int index)
        {
            if (!QObject_getPropertyQByteArray(_handle, index, out var data, out _))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return data;
        }

        protected void SetPropertyQColor(int index, System.Drawing.Color value)
        {
            if (!QObject_setPropertyQColor(_handle, index, value.ToArgb()))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected System.Drawing.Color GetPropertyQColor(int index)
        {
            if (!QObject_getPropertyQColor(_handle, index, out var argb))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return Color.FromArgb(argb);
        }

        protected void SetPropertyQSize(int index, Size value)
        {
            if (!QObject_setPropertyQSize(_handle, index, value.Width, value.Height))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected Size GetPropertyQSize(int index)
        {
            if (!QObject_getPropertyQSize(_handle, index, out var width, out var height))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return new Size(width, height);
        }

        protected void SetPropertyQSizeF(int index, SizeF value)
        {
            if (!QObject_setPropertyQSizeF(_handle, index, value.Width, value.Height))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected SizeF GetPropertyQSizeF(int index)
        {
            if (!QObject_getPropertyQSizeF(_handle, index, out var width, out var height))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return new SizeF((float) width, (float) height);
        }

        protected void SetPropertyQRect(int index, Rectangle value)
        {
            if (!QObject_setPropertyQRect(_handle, index, value.X, value.Y, value.Width, value.Height))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected Rectangle GetPropertyQRect(int index)
        {
            if (!QObject_getPropertyQRect(_handle, index, out var x, out var y, out var width, out var height))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return new Rectangle(x, y, width, height);
        }

        protected void SetPropertyQRectF(int index, RectangleF value)
        {
            if (!QObject_setPropertyQRectF(_handle, index, value.X, value.Y, value.Width, value.Height))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected RectangleF GetPropertyQRectF(int index)
        {
            if (!QObject_getPropertyQRectF(_handle, index, out var x, out var y, out var width, out var height))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return new RectangleF((float) x, (float) y, (float) width, (float) height);
        }

        protected void SetPropertyQPoint(int index, Point value)
        {
            if (!QObject_setPropertyQPoint(_handle, index, value.X, value.Y))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected Point GetPropertyQPoint(int index)
        {
            if (!QObject_getPropertyQPoint(_handle, index, out var x, out var y))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return new Point(x, y);
        }

        protected void SetPropertyQPointF(int index, PointF value)
        {
            if (!QObject_setPropertyQPointF(_handle, index, value.X, value.Y))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected PointF GetPropertyQPointF(int index)
        {
            if (!QObject_getPropertyQPointF(_handle, index, out var x, out var y))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return new PointF((float) x, (float) y);
        }

        protected void SetPropertyQDateTime(int index, DateTime value)
        {
            var msecsSinceEpoch = new DateTimeOffset(value).ToUnixTimeMilliseconds();
            var localTime = value.Kind == DateTimeKind.Local;

            if (!QObject_setPropertyQDateTime(_handle, index, msecsSinceEpoch, localTime))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected DateTime GetPropertyQDateTime(int index)
        {
            if (!QObject_getPropertyQDateTime(_handle, index, out var msecsSinceEpoch, out var localTime))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            var dateTimeOffset = DateTimeOffset.FromUnixTimeMilliseconds(msecsSinceEpoch);
            return localTime ? dateTimeOffset.LocalDateTime : dateTimeOffset.UtcDateTime;
        }

        protected void SetPropertyQUrl(int index, string url)
        {
            if (!QObject_setPropertyQUrl(_handle, index, url, url.Length))
            {
                throw new InvalidOperationException("Couldn't set property " + index);
            }
        }

        protected string GetPropertyQUrl(int index)
        {
            if (!QObject_getPropertyQUrl(_handle, index, out var url))
            {
                throw new InvalidOperationException("Couldn't get property " + index);
            }

            return url;
        }

        #endregion

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

        protected QmlList<T> GetPropertyQmlList<T>(int index) where T : QObjectBase
        {
            return GetPropertyPrimitive<QmlList<T>>(index);
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
        private static extern unsafe bool QObject_setPropertyQByteArray(IntPtr target, int idx, [In]
            byte* data, int dataLength);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQByteArray(IntPtr target, int idx,
            [Out, MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 3)]
            out byte[] data, out int dataLength);

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
        private static extern bool QObject_setPropertyQSize(IntPtr target, int idx, int width, int height);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQSize(IntPtr target, int idx, out int width, out int height);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_setPropertyQSizeF(IntPtr target, int idx, double width, double height);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQSizeF(IntPtr target, int idx, out double width,
            out double height);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool
            QObject_setPropertyQRect(IntPtr target, int idx, int x, int y, int width, int height);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQRect(IntPtr target, int idx, out int x, out int y, out int width,
            out int height);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_setPropertyQRectF(IntPtr target, int idx, double x, double y, double width,
            double height);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQRectF(IntPtr target, int idx, out double x, out double y,
            out double width, out double height);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_setPropertyQPoint(IntPtr target, int idx, int x, int y);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQPoint(IntPtr target, int idx, out int x, out int y);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_setPropertyQPointF(IntPtr target, int idx, double x, double y);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQPointF(IntPtr target, int idx, out double x,
            out double y);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_setPropertyQUrl(IntPtr target, int idx, string urlString, int urlStringLen);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQUrl(IntPtr target, int idx, out string urlString);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_setPropertyQColor(IntPtr target, int idx, int argb);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQColor(IntPtr target, int idx, out int argb);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_setPropertyQDateTime(IntPtr target, int idx, long msecsSinceEpoch,
            [MarshalAs(UnmanagedType.I1)]
            bool localTime);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QObject_getPropertyQDateTime(IntPtr target, int idx, out long msecsSinceEpoch,
            [MarshalAs(UnmanagedType.I1)]
            out bool localTime);

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