using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Text
{
    public enum NativeTextAlign : int
    {
        Left = 0,
        Center,
        Right,
        Justified
    };

    public enum NativeParagraphAlign : int
    {
        Near = 0,
        Far,
        Center
    };

    public enum NativeWordWrap : int
    {
        Wrap = 0,
        NoWrap,
        EmergencyBreak,
        WholeWord,
        Character
    };

    public enum NativeTrimMode : int
    {
        None = 0,
        NoWrap,
        EmergencyBreak,
        WholeWord,
        Character
    };

    public enum NativeTrimmingSign : int
    {
        None = 0,
        Ellipsis
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeParagraphStyle
    {
        [MarshalAs(UnmanagedType.U1)]
        public bool HangingIndent;
        public float Indent;
        public float TabStopWidth;
        public NativeTextAlign TextAlignment;
        public NativeParagraphAlign ParagraphAlignment;
        public NativeWordWrap WordWrap;
        public NativeTrimMode TrimMode;
        public NativeTrimmingSign TrimmingSign;
    }
}