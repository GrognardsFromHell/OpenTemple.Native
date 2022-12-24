using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Drawing;

public enum NativeTextAlign : int
{
    Left = 0,
    Center,
    Right,
    Justified
}

public enum NativeParagraphAlign : int
{
    Near = 0,
    Far,
    Center
}

public enum NativeWordWrap : int
{
    Wrap = 0,
    NoWrap,
    EmergencyBreak,
    WholeWord,
    Character
}

public enum NativeTrimMode : int
{
    None = 0,
    NoWrap,
    EmergencyBreak,
    WholeWord,
    Character
}

public enum NativeTrimmingSign : int
{
    None = 0,
    Ellipsis
}

public enum NativeLineSpacingMode : int
{
    Default = 0, 
    Uniform, 
    Proportional
}

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
    public NativeLineSpacingMode LineSpacingMode;
    // If mode is proportional, this is a factor
    // If mode is default this is ignored
    // If mode is uniform, this is an absolute pixel value
    public float LineHeight;
}
