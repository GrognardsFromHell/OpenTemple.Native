using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security;
using OpenTemple.Interop;
using QmlBuildTasks.CodeGen;

namespace QmlBuildTasks.Introspection
{
    [SuppressUnmanagedCodeSecurity]
    public sealed class QmlInfo : IDisposable
    {
        private IntPtr _handle;

        private Options _options;

        public QmlInfo(Options options)
        {
            _options = options;

            _handle = QmlInfo_new(
                NativeDelegate.Create<LoggerDelegate>(Log),
                options.BaseDirectory
            );

            foreach (var importPath in options.ImportPaths)
            {
                QmlInfo_addImportPath(_handle, importPath);
            }

            foreach (var pattern in options.ExcludePatterns)
            {
                if (!QmlInfo_addExcludePattern(_handle, pattern))
                {
                    throw new ArgumentException("Invalid RegExp pattern: " + pattern);
                }
            }
        }

        public void Dispose()
        {
            if (_handle != IntPtr.Zero)
            {
                QmlInfo_delete(_handle);
            }

            _handle = IntPtr.Zero;
        }

        public void LoadFile(string path)
        {
            if (!QmlInfo_addFile(_handle, path))
            {
                throw new ArgumentException(QmlInfo_error(_handle));
            }
        }

        public void AddNativeModule(string uri, int majorVersion)
        {
            if (!QmlInfo_addNativeModule(_handle, uri, majorVersion))
            {
                throw new ArgumentException(QmlInfo_error(_handle));
            }
        }

        public void AddMetaClass(string metaClassName)
        {
            if (!QmlInfo_addMetaClass(_handle, metaClassName + "*"))
            {
                throw new ArgumentException(QmlInfo_error(_handle));
            }
        }

        public string Process()
        {
            var generatedCode = new StringWriter();

            var generator = new ProxiesGenerator(_options);

            void VisitTypeInfo(IntPtr typeLibraryHandle)
            {
                var typeLibrary = new TypeLibrary(typeLibraryHandle);
                try
                {
                    generatedCode.Write(generator.Generate(typeLibrary.Types));
                }
                catch (Exception e)
                {
                    Console.WriteLine("Failed to generate code: " + e);
                }
            }

            QmlInfo_visitTypeLibrary(_handle, VisitTypeInfo);

            return generatedCode.ToString();
        }

        private void Log(LogLevel level, string message)
        {
            Console.WriteLine(message);
        }

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern IntPtr QmlInfo_new(NativeDelegate logger, string basePath);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern string QmlInfo_error(IntPtr qmlInfo);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern void QmlInfo_addImportPath(IntPtr qmlInfo, string path);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool QmlInfo_addExcludePattern(IntPtr qmlInfo, string pattern);

        [DllImport(OpenTempleLib.Path)]
        private static extern void QmlInfo_delete(IntPtr qmlInfo);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool QmlInfo_addFile(
            IntPtr qmlInfo,
            string path);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool QmlInfo_addNativeModule(
            IntPtr qmlInfo,
            string uri,
            int majorVersion);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Ansi)]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool QmlInfo_addMetaClass(
            IntPtr qmlInfo,
            string metaClassName);

        private delegate void TypeInfoVisitor(IntPtr handle);

        [DllImport(OpenTempleLib.Path)]
        private static extern bool QmlInfo_visitTypeLibrary(
            IntPtr qmlInfo,
            TypeInfoVisitor visitor);

        private enum LogLevel
        {
            Warning = 0,
            Error = 1
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        private delegate void LoggerDelegate(LogLevel level, string message);
    }
}