using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Text
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct NativeLineMetrics
    {
        public readonly int Length;
        public readonly int TrailingWhitespaceLength;
        public readonly int NewlineLength;
        public readonly float Height;
        public readonly float Baseline;
        public readonly bool IsTrimmed;
    }
}