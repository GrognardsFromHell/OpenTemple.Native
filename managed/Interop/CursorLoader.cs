using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTemple.Interop;

public static class CursorLoader
{
    [DllImport(OpenTempleLib.Path)]
    
    public static extern IntPtr Win32_LoadImageToCursor(
        [In]
        byte[] pixelData,
        int width,
        int height,
        int hotspotX,
        int hotspotY
    );
}