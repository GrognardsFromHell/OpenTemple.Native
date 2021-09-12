using System;
using System.Collections.Generic;
using System.Drawing;
using System.Numerics;
using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Drawing
{
    /// <summary>
    /// A text rendering engine.
    /// </summary>
    public class NativeDrawingEngine : IDisposable
    {
        private static readonly char[] EmptyText = new[] { '\0' };

        private IntPtr _native;

        /// <param name="d3d11Device">Pointer to a ID3D11Device. RefCount should not be incremented.</param>
        /// <param name="debugDevice">Enable the D2D debug layer.</param>
        public NativeDrawingEngine(IntPtr d3d11Device, bool debugDevice)
        {
            CheckStructSizes();

            if (!DrawingEngine_Create(d3d11Device, debugDevice, out _native, out var error))
            {
                throw new InvalidOperationException("Couldn't create native TextEngine: " + error);
            }
        }

        private static void CheckStructSizes()
        {
            // Validate struct compatibility between C# and native
            DrawingEngine_GetStructSizes(out var paragraphStylesSize, out var textStylesSize);
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

        public void BeginDraw()
        {
            DrawingEngine_BeginDraw(_native);
        }

        public void EndDraw()
        {
            if (!DrawingEngine_EndDraw(_native, out var error))
            {
                throw new InvalidOperationException(error);
            }
        }

        public void PushClipRect(RectangleF clipRect, bool antiAliased)
        {
            DrawingEngine_PushClipRect(_native, clipRect.Left, clipRect.Top, clipRect.Right, clipRect.Bottom,
                antiAliased);
        }

        public void PopClipRect()
        {
            DrawingEngine_PopClipRect(_native);
        }

        public void SetTransform(ref Matrix3x2 matrix)
        {
            DrawingEngine_SetTransform(_native, ref matrix);
        }

        public void GetTransform(out Matrix3x2 matrix)
        {
            DrawingEngine_GetTransform(_native, out matrix);
        }

        public void SetCanvasSize(SizeF size)
        {
            DrawingEngine_SetCanvasSize(_native, size.Width, size.Height);
        }

        public SizeF GetCanvasScale()
        {
            DrawingEngine_GetCanvasScale(_native, out var width, out var height);
            return new SizeF(width, height);
        }

        public List<string> FontFamilies
        {
            get
            {
                var count = DrawingEngine_GetFontFamiliesCount(_native);
                var result = new List<string>(count);
                for (var i = 0; i < count; i++)
                {
                    result.Add(DrawingEngine_GetFontFamilyName(_native, i));
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
                DrawingEngine_AddFontFile(_native, filename, dataPtr, data.Length);
            }
        }

        public void ReloadFontFamilies()
        {
            if (!DrawingEngine_ReloadFontFamilies(_native, out var error))
            {
                throw new InvalidOperationException("Failed to reload font families: " + error);
            }
        }

        public void SetRenderTarget(IntPtr d3d11Texture)
        {
            if (!DrawingEngine_SetRenderTarget(_native, d3d11Texture, out var error))
            {
                throw new InvalidOperationException("Failed to set render target: " + error);
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
                if (!DrawingEngine_CreateTextLayout(
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
            if (!DrawingEngine_RenderTextLayout(_native, layout.NativePointer, x, y, opacity, out var error))
            {
                throw new InvalidOperationException("Couldn't render text layout: " + error);
            }
        }

        public void RenderBackgroundAndBorder(float x, float y, float width, float height,
            ref NativeBackgroundAndBorderStyle style)
        {
            if (!DrawingEngine_RenderBackgroundAndBorder(_native, x, y, width, height, ref style, out var error))
            {
                throw new InvalidOperationException("Couldn't render background and border: " + error);
            }
        }

        private void ReleaseUnmanagedResources()
        {
            if (_native != IntPtr.Zero)
            {
                DrawingEngine_Free(_native);
                _native = IntPtr.Zero;
            }
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        ~NativeDrawingEngine()
        {
            ReleaseUnmanagedResources();
        }

        #region P/Invoke

        [DllImport(OpenTempleLib.Path)]
        private static extern void DrawingEngine_GetStructSizes(
            out int paragraphStylesSize,
            out int textStylesSize
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern bool DrawingEngine_Create(
            IntPtr d3d11Device,
            bool debugDevice,
            out IntPtr drawingEngine,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error);

        [DllImport(OpenTempleLib.Path)]
        private static extern void DrawingEngine_Free(IntPtr drawingEngine);

        [DllImport(OpenTempleLib.Path)]
        private static extern void DrawingEngine_BeginDraw(IntPtr drawingEngine);

        [DllImport(OpenTempleLib.Path)]
        private static extern bool DrawingEngine_EndDraw(IntPtr drawingEngine,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error);

        [DllImport(OpenTempleLib.Path)]
        private static extern void DrawingEngine_PushClipRect(IntPtr drawingEngine, float left, float top,
            float right, float bottom, bool antiAliased);

        [DllImport(OpenTempleLib.Path)]
        private static extern void DrawingEngine_PopClipRect(IntPtr drawingEngine);

        [DllImport(OpenTempleLib.Path)]
        private static extern bool DrawingEngine_SetRenderTarget(IntPtr drawingEngine,
            IntPtr d3dRenderTarget,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern void DrawingEngine_SetTransform(IntPtr drawingEngine, ref Matrix3x2 matrix);

        [DllImport(OpenTempleLib.Path)]
        private static extern void DrawingEngine_GetTransform(IntPtr drawingEngine, out Matrix3x2 matrix);

        [DllImport(OpenTempleLib.Path)]
        private static extern void DrawingEngine_SetCanvasSize(IntPtr drawingEngine, float width, float height);

        [DllImport(OpenTempleLib.Path)]
        private static extern void
            DrawingEngine_GetCanvasScale(IntPtr drawingEngine, out float width, out float height);

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void DrawingEngine_AddFontFile(
            IntPtr drawingEngine,
            [MarshalAs(UnmanagedType.LPStr)]
            string filename,
            byte* data,
            int dataLength
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern bool DrawingEngine_ReloadFontFamilies(
            IntPtr drawingEngine,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error);

        [DllImport(OpenTempleLib.Path)]
        [SuppressGCTransition]
        private static extern int DrawingEngine_GetFontFamiliesCount(IntPtr drawingEngine);

        [DllImport(OpenTempleLib.Path)]
        [return: MarshalAs(UnmanagedType.LPWStr)]
        private static extern string DrawingEngine_GetFontFamilyName(IntPtr drawingEngine, int index);

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe bool DrawingEngine_CreateTextLayout(
            IntPtr drawingEngine,
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
        private static extern bool DrawingEngine_RenderTextLayout(
            IntPtr drawingEngine,
            IntPtr textLayout,
            float x,
            float y,
            float opacity,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern bool DrawingEngine_RenderBackgroundAndBorder(
            IntPtr drawingEngine,
            float x,
            float y,
            float width,
            float height,
            [In]
            ref NativeBackgroundAndBorderStyle style,
            [MarshalAs(UnmanagedType.LPWStr)]
            out string error
        );

        #endregion
    }
}