using System;
using System.Collections.Generic;
using System.Drawing;
using System.Numerics;
using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Drawing;

/// <summary>
/// A text rendering engine.
/// </summary>
public partial class NativeDrawingEngine : IDisposable
{
    private static readonly char[] EmptyText = new[] { '\0' };

    private nint _native;

    /// <param name="d3d11Device">Pointer to a ID3D11Device. RefCount should not be incremented.</param>
    /// <param name="debugDevice">Enable the D2D debug layer.</param>
    public NativeDrawingEngine(nint d3d11Device, bool debugDevice)
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

    public void SetRenderTarget(nint d3d11Texture)
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
        if (!DrawingEngine_RenderBackgroundAndBorder(_native, x, y, width, height, in style, out var error))
        {
            throw new InvalidOperationException("Couldn't render background and border: " + error);
        }
    }

    private void ReleaseUnmanagedResources()
    {
        if (_native != nint.Zero)
        {
            DrawingEngine_Free(_native);
            _native = nint.Zero;
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

    [LibraryImport(OpenTempleLib.Path)]
    [SuppressGCTransition]
    private static partial void DrawingEngine_GetStructSizes(
        out int paragraphStylesSize,
        out int textStylesSize
    );

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool DrawingEngine_Create(
        nint d3d11Device,
        [MarshalAs(UnmanagedType.Bool)] bool debugDevice,
        out nint drawingEngine,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string error);

    [LibraryImport(OpenTempleLib.Path)]
    private static partial void DrawingEngine_Free(nint drawingEngine);

    [LibraryImport(OpenTempleLib.Path)]
    private static partial void DrawingEngine_BeginDraw(nint drawingEngine);

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool DrawingEngine_EndDraw(nint drawingEngine,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string error);

    [LibraryImport(OpenTempleLib.Path)]
    private static partial void DrawingEngine_PushClipRect(nint drawingEngine, float left, float top,
        float right, float bottom, [MarshalAs(UnmanagedType.Bool)] bool antiAliased);

    [LibraryImport(OpenTempleLib.Path)]
    private static partial void DrawingEngine_PopClipRect(nint drawingEngine);

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool DrawingEngine_SetRenderTarget(nint drawingEngine,
        nint d3dRenderTarget,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string error
    );

    [DllImport(OpenTempleLib.Path)]
    private static extern void DrawingEngine_SetTransform(nint drawingEngine, ref Matrix3x2 matrix);

    [DllImport(OpenTempleLib.Path)]
    private static extern void DrawingEngine_GetTransform(nint drawingEngine, out Matrix3x2 matrix);

    [LibraryImport(OpenTempleLib.Path)]
    private static partial void DrawingEngine_SetCanvasSize(nint drawingEngine, float width, float height);

    [LibraryImport(OpenTempleLib.Path)]
    private static partial void
        DrawingEngine_GetCanvasScale(nint drawingEngine, out float width, out float height);

    [LibraryImport(OpenTempleLib.Path)]
    private static unsafe partial void DrawingEngine_AddFontFile(
        nint drawingEngine,
        [MarshalAs(UnmanagedType.LPStr)]
        string filename,
        byte* data,
        int dataLength
    );

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool DrawingEngine_ReloadFontFamilies(
        nint drawingEngine,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string error);

    [LibraryImport(OpenTempleLib.Path)]
    [SuppressGCTransition]
    private static partial int DrawingEngine_GetFontFamiliesCount(nint drawingEngine);

    [LibraryImport(OpenTempleLib.Path)]
    [return: MarshalAs(UnmanagedType.LPWStr)]
    private static partial string DrawingEngine_GetFontFamilyName(nint drawingEngine, int index);

    [DllImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static extern unsafe bool DrawingEngine_CreateTextLayout(
        nint drawingEngine,
        [In]
        ref NativeParagraphStyle paragraphStyle,
        [In]
        ref NativeTextStyle textStyle,
        char* text,
        int textLength,
        float maxWidth,
        float maxHeight,
        out nint textLayout,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string error
    );

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool DrawingEngine_RenderTextLayout(
        nint drawingEngine,
        nint textLayout,
        float x,
        float y,
        float opacity,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string error
    );

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool DrawingEngine_RenderBackgroundAndBorder(
        nint drawingEngine,
        float x,
        float y,
        float width,
        float height,
        in NativeBackgroundAndBorderStyle style,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string error
    );

    #endregion
}