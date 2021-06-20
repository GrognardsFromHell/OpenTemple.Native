using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Text
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct NativeMetrics
    {
        public readonly float Left;
        public readonly float Top;
        public readonly float Width;
        public readonly float WidthIncludingTrailingWhitespace;
        public readonly float Height;
        public readonly float LayoutWidth;
        public readonly float LayoutHeight;
        public readonly int MaxBidiReorderingDepth;
        public readonly int LineCount;
    }
}