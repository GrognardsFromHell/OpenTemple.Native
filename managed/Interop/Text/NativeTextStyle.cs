using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Text
{
    public enum NativeFontStretch : int
    {
        UltraCondensed = 0,
        ExtraCondensed,
        Condensed,
        SemiCondensed,
        Normal,
        SemiExpanded,
        Expanded,
        ExtraExpanded,
        UltraExpanded
    };

    public enum NativeFontStyle : int
    {
        Normal = 0,
        Italic,
        Oblique
    };

    // See https://developer.mozilla.org/en-US/docs/Web/CSS/font-weight#common_weight_name_mapping
    // Maps directly to the DirectWrite weight enum
    public enum NativeFontWeight : int
    {
        Thin = 100,
        ExtraLight = 200,
        Light = 300,
        Regular = 400,
        Medium = 500,
        SemiBold = 600,
        Bold = 700,
        ExtraBold = 800,
        Black = 900,
        ExtraBlack = 950
    };

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct NativeTextStyle
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        public string FontFace;
        public float FontSize;
        // ARGB packed (from high to low)
        public uint Color;
        [MarshalAs(UnmanagedType.U1)]
        public bool Underline;
        [MarshalAs(UnmanagedType.U1)]
        public bool LineThrough;
        [MarshalAs(UnmanagedType.U1)]
        public bool Kerning;
        public NativeFontStretch FontStretch;
        public NativeFontStyle FontStyle;
        public NativeFontWeight FontWeight;
        public uint DropShadowColor;
        public uint OutlineColor;
        public float OutlineWidth;
    }
}