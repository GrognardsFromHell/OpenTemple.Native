using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTemple.Interop
{
    [SuppressUnmanagedCodeSecurity]
    public sealed class JumpListBuilder : IDisposable
    {
        private IntPtr _handle;

        public JumpListBuilder()
        {
            _handle = JumpListBuilder_Create();
            if (_handle == IntPtr.Zero)
            {
                throw new InvalidOperationException("Failed to create JumpListBuilder");
            }

            var result = JumpListBuilder_Init(_handle);
            if (result != 0)
            {
                Dispose();
                throw new InvalidOperationException($"Failed to initialize jump list builder: {result}");
            }
        }

        public void Dispose()
        {
            if (_handle != IntPtr.Zero)
            {
                JumpListBuilder_Free(_handle);
                _handle = IntPtr.Zero;
            }
        }

        public void AddTask(
            string arguments,
            string title,
            string iconPath,
            int iconIndex,
            string description = null
        )
        {
            if (_handle == IntPtr.Zero)
            {
                throw new ObjectDisposedException(nameof(_handle));
            }

            var result = JumpListBuilder_AddTask(_handle, arguments, title, iconPath, iconIndex, description);
            if (result != 0)
            {
                throw new InvalidOperationException($"Failed to add task to jump list. Error Code: {result}");
            }
        }

        public void Commit()
        {
            if (_handle == IntPtr.Zero)
            {
                throw new ObjectDisposedException(nameof(_handle));
            }

            var result = JumpListBuilder_Commit(_handle);
            if (result != 0)
            {
                throw new InvalidOperationException($"Failed to commit jump list. Error Code: {result}");
            }
        }

        #region P/Invoke

        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr JumpListBuilder_Create();

        [DllImport(OpenTempleLib.Path)]
        private static extern int JumpListBuilder_Init(IntPtr handle);

        [DllImport(OpenTempleLib.Path)]
        private static extern int JumpListBuilder_AddTask(IntPtr handle,
            [In, MarshalAs(UnmanagedType.LPWStr)]
            string arguments,
            [In, MarshalAs(UnmanagedType.LPWStr)]
            string title,
            [In, MarshalAs(UnmanagedType.LPWStr)]
            string iconPath,
            int iconIndex,
            [In, Optional, MarshalAs(UnmanagedType.LPWStr)]
            string description
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern int JumpListBuilder_Commit(IntPtr handle);

        [DllImport(OpenTempleLib.Path)]
        private static extern void JumpListBuilder_Free(IntPtr handle);

        #endregion
    }
}