using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Text
{
    /// <summary>
    /// A text rendering engine.
    /// </summary>
    public class NativeTextEngine : IDisposable
    {
        private static readonly char[] EmptyText = new[] {'\0'};

        private IntPtr _native;

        /// <param name="renderTarget">Pointer to a ID2D1RenderTarget. RefCount should not be incremented.</param>
        public NativeTextEngine(IntPtr renderTarget)
        {
            CheckStructSizes();

            if (!TextEngine_Create(renderTarget, out _native, out var error))
            {
                throw new InvalidOperationException("Couldn't create native TextEngine: " + error);
            }
        }

        private static void CheckStructSizes()
        {
            // Validate struct compatibility between C# and native
            TextEngine_GetStructSizes(out var paragraphStylesSize, out var textStylesSize);
            var managedParagraphStylesSize = Marshal.SizeOf<NativeParagraphStyle>();
            if (paragraphStylesSize != managedParagraphStylesSize)
            {
                throw new InvalidOperationException(
                    $"Native size of ParagraphStyles is {paragraphStylesSize}, but managed is {managedParagraphStylesSize}");
            }

            var managedTextStylesSize = Marshal.SizeOf<NativeTextStyle>();
            if (textStylesSize != managedTextStylesSize)
            {
                throw new InvalidOperationException(
                    $"Native size of TextStyles is {textStylesSize}, but managed is {managedTextStylesSize}");
            }
        }

        public List<string> FontFamilies
        {
            get
            {
                var count = TextEngine_GetFontFamiliesCount(_native);
                var result = new List<string>(count);
                for (var i = 0; i < count; i++)
                {
                    result.Add(TextEngine_GetFontFamilyName(_native, i));
                }

                return result;
            }
        }

        public unsafe void AddFontFile(string filename, ReadOnlySpan<byte> data)
        {
            if (data.IsEmpty)
            {
                throw new InvalidOperationException("Cannot add a font-file with empty data");
            }

            fixed (byte* dataPtr = data)
            {
                TextEngine_AddFontFile(_native, filename, dataPtr, data.Length);
            }
        }

        public void ReloadFontFamilies()
        {
            if (!TextEngine_ReloadFontFamilies(_native, out var error))
            {
                throw new InvalidOperationException("Failed to reload font families: " + error);
            }
        }

        public unsafe NativeTextLayout CreateTextLayout(ref NativeParagraphStyle paragraphStyle,
            ref NativeTextStyle textStyle,
            ReadOnlySpan<char> text,
            float maxWidth,
            float maxHeight)
        {
            // This is to avoid a null-pointer which is rejected by DirectWrite, even though length 0
            // is acceptable
            if (text.IsEmpty)
            {
                text = EmptyText;
            }

            fixed (char* textPtr = text)
            {
                if (!TextEngine_CreateTextLayout(
                    _native,
                    ref paragraphStyle,
                    ref textStyle,
                    textPtr,
                    text.Length,
                    maxWidth,
                    maxHeight,
                    out var textLayoutNative,
                    out var error
                ))
                {
                    throw new InvalidOperationException("Failed to create TextLayout: " + error);
                }

                return new NativeTextLayout(textLayoutNative);
            }
        }

        public void RenderTextLayout(NativeTextLayout layout, float x, float y, float opacity)
        {
            if (!TextEngine_RenderTextLayout(_native, layout.NativePointer, x, y, opacity, out var error))
            {
                throw new InvalidOperationException("Couldn't render text layout: " + error);
            }
        }

        public void RenderBackgroundAndBorder(float x, float y, float width, float height,
            ref NativeBackgroundAndBorderStyle style)
        {
            if (!TextEngine_RenderBackgroundAndBorder(_native, x, y, width, height, ref style, out var error))
            {
                throw new InvalidOperationException("Couldn't render background and border: " + error);
            }
        }

        private void ReleaseUnmanagedResources()
        {
            if (_native != IntPtr.Zero)
            {
                TextEngine_Free(_native);
                _native = IntPtr.Zero;
            }
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        ~NativeTextEngine()
        {
            ReleaseUnmanagedResources();
        }

        #region P/Invoke

        [DllImport(OpenTempleLib.Path)]
        private static extern void TextEngine_GetStructSizes(
            out int paragraphStylesSize,
            out int textStylesSize
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern bool TextEngine_Create(
            IntPtr renderTarget,
            out IntPtr textEngine,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error);

        [DllImport(OpenTempleLib.Path)]
        private static extern void TextEngine_Free(IntPtr textEngine);

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void TextEngine_AddFontFile(
            IntPtr textEngine,
            [MarshalAs(UnmanagedType.LPStr)]
            string filename,
            byte* data,
            int dataLength
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern bool TextEngine_ReloadFontFamilies(
            IntPtr textEngine,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error);

        [DllImport(OpenTempleLib.Path)]
        [SuppressGCTransition]
        private static extern int TextEngine_GetFontFamiliesCount(IntPtr textEngine);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.LPWStr)]
        private static extern string TextEngine_GetFontFamilyName(IntPtr textEngine, int index);

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe bool TextEngine_CreateTextLayout(
            IntPtr textEngine,
            [In]
            ref NativeParagraphStyle paragraphStyle,
            [In]
            ref NativeTextStyle textStyle,
            char* text,
            int textLength,
            float maxWidth,
            float maxHeight,
            out IntPtr textLayout,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern bool TextEngine_RenderTextLayout(
            IntPtr textEngine,
            IntPtr textLayout,
            float x,
            float y,
            float opacity,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern bool TextEngine_RenderBackgroundAndBorder(
            IntPtr textEngine,
            float x,
            float y,
            float width,
            float height,
            [In] ref NativeBackgroundAndBorderStyle style,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error
        );

        #endregion
    }
}