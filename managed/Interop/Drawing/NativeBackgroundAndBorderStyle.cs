using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Drawing;

[StructLayout(LayoutKind.Sequential)]
public struct NativeBackgroundAndBorderStyle
{
    public float RadiusX;
    public float RadiusY;
    public uint BackgroundColor;
    public uint BorderColor;
    public float BorderWidth;
}