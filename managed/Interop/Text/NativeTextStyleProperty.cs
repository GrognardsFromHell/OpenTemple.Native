using System;

namespace OpenTemple.Interop.Text
{
    [Flags]
    public enum NativeTextStyleProperty
    {
        FontFace = 1 << 0,
        FontSize = 1 << 1,
        Color = 1 << 2,
        Underline = 1 << 3,
        LineThrough = 1 << 4,
        FontStretch = 1 << 5,
        FontStyle = 1 << 6,
        FontWeight = 1 << 7,
        DropShadow = 1 << 8,
        Outline = 1 << 9,
    }
}