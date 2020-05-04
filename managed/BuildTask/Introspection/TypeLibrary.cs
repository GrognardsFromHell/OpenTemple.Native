using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security;
using OpenTemple.Interop;

namespace QmlBuildTasks.Introspection
{
    [SuppressUnmanagedCodeSecurity]
    public class TypeLibrary
    {
        private readonly IntPtr _handle;

        public List<TypeInfo> Types { get; }

        public TypeLibrary(IntPtr handle)
        {
            _handle = handle;

            Types = new List<TypeInfo>();
            TypeLibrary_visitTypes(_handle, infoHandle => Types.Add(new TypeInfo(infoHandle)));
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern void TypeLibrary_visitTypes(IntPtr handle, VisitTypesDelegate visitor);

        private delegate void VisitTypesDelegate(IntPtr typeInfoHandle);
    }
}