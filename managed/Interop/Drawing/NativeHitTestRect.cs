using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Drawing;

[StructLayout(LayoutKind.Sequential)]
public readonly struct NativeHitTestRect
{
    public readonly float X;
    public readonly float Y;
    public readonly float Width;
    public readonly float Height;
}
