using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security;
using OpenTemple.Interop;

namespace QmlBuildTasks.Introspection
{
    public readonly struct EnumValueInfo
    {
        private readonly IntPtr _handle;

        public string Name => EnumValueInfo_name(_handle);

        public int Value => EnumValueInfo_value(_handle);

        public EnumValueInfo(IntPtr handle)
        {
            _handle = handle;
        }

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern string EnumValueInfo_name(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int EnumValueInfo_value(IntPtr handle);
    }

    public class EnumInfo
    {
        private readonly IntPtr _handle;

        public EnumInfo(IntPtr handle)
        {
            _handle = handle;
        }

        public string Name => EnumInfo_name(_handle);

        public IEnumerable<EnumValueInfo> Values
        {
            get
            {
                var count = EnumInfo_values_size(_handle);
                for (var i = 0; i < count; i++)
                {
                    var valueHandle = EnumInfo_values(_handle, i);
                    yield return new EnumValueInfo(valueHandle);
                }
            }
        }

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        private static extern string EnumInfo_name(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern int EnumInfo_values_size(IntPtr handle);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr EnumInfo_values(IntPtr handle, int index);
    }
}