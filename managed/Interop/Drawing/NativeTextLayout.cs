using System;
using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Drawing;

public partial class NativeTextLayout : IDisposable
{
    public nint NativePointer { get; private set; }

    public NativeTextLayout(nint native)
    {
        NativePointer = native;
    }

    public void SetStyle(int start, int length, NativeTextStyleProperty properties, ref NativeTextStyle style)
    {
        if (!TextLayout_SetStyle(NativePointer, (uint) start, (uint) length, properties, ref style, out var error))
        {
            throw new InvalidOperationException("Couldn't set text style: " + error);
        }
    }

    public void GetMetrics(out NativeMetrics metrics)
    {
        TextLayout_GetMetrics(NativePointer, out metrics);
    }

    /// <returns>False if the metrics span was too small.</returns>
    public unsafe bool GetLineMetrics(Span<NativeLineMetrics> metrics, out int actualLineCount)
    {
        fixed (NativeLineMetrics* metricsPtr = metrics)
        {
            return TextLayout_GetLineMetrics(NativePointer, metricsPtr, metrics.Length, out actualLineCount);
        }
    }

    public bool HitTestPoint(float x, float y, out int start, out int length, out bool trailingHit)
    {
        return TextLayout_HitTestPoint(NativePointer, x, y, out start, out length, out trailingHit);
    }

    public NativeHitTestRect HitTestTextPosition(int textPosition, bool afterPosition)
    {
        TextLayout_HitTestTextPosition(NativePointer, textPosition, afterPosition, out var rect);
        return rect;
    }

    public NativeHitTestRect[] HitTestTextRange(int start, int length)
    {
        HitTestTextRange(start, length, Span<NativeHitTestRect>.Empty, out var count);
        var rects = new NativeHitTestRect[count];
        HitTestTextRange(start, length, rects, out _);
        return rects;
    }

    /// <returns>False if the metrics span was too small.</returns>
    public unsafe bool HitTestTextRange(int start, int length, Span<NativeHitTestRect> rects, out int actualRectsCount)
    {
        fixed (NativeHitTestRect* rectsPtr = rects)
        {
            return TextLayout_HitTestTextRange(NativePointer, start, length, rectsPtr, rects.Length, out actualRectsCount);
        }
    }

    public NativeLineMetrics[] GetLineMetrics()
    {
        GetLineMetrics(Span<NativeLineMetrics>.Empty, out var count);
        var metrics = new NativeLineMetrics[count];
        GetLineMetrics(metrics, out _);
        return metrics;
    }

    public void SetMaxWidth(float maxWidth)
    {
        if (!TextLayout_SetMaxWidth(NativePointer, maxWidth))
        {
            throw new ArgumentException("Invalid maximum width given: " + maxWidth);
        }
    }

    public void SetMaxHeight(float maxHeight)
    {
        if (!TextLayout_SetMaxHeight(NativePointer, maxHeight))
        {
            throw new ArgumentException("Invalid maximum width given: " + maxHeight);
        }
    }

    private void ReleaseUnmanagedResources()
    {
        if (NativePointer != nint.Zero)
        {
            TextLayout_Free(NativePointer);
            NativePointer = nint.Zero;
        }
    }

    public void Dispose()
    {
        ReleaseUnmanagedResources();
        GC.SuppressFinalize(this);
    }

    ~NativeTextLayout()
    {
        ReleaseUnmanagedResources();
    }

    #region P/Invoke

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    [SuppressGCTransition]
    private static partial void TextLayout_Free(nint textLayout);

    [DllImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static extern bool TextLayout_SetStyle(nint textLayout,
        uint start,
        uint length,
        NativeTextStyleProperty properties,
        ref NativeTextStyle style,
        [MarshalAs(UnmanagedType.LPWStr)] out string error
    );

    [LibraryImport(OpenTempleLib.Path)]
    [SuppressGCTransition]
    private static partial void TextLayout_GetMetrics(nint textLayout, out NativeMetrics metrics);

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static unsafe partial bool TextLayout_GetLineMetrics(nint textLayout,
        NativeLineMetrics* metrics,
        int metricsCount,
        out int actualLineCount);

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool TextLayout_HitTestPoint(
        nint textLayout,
        float x, float y,
        out int start,
        out int length,
        [MarshalAs(UnmanagedType.Bool)] out bool trailingHit
    );

    [LibraryImport(OpenTempleLib.Path)]
    private static partial void TextLayout_HitTestTextPosition(
        nint textLayout,
        int textPosition,
        [MarshalAs(UnmanagedType.Bool)] bool afterPosition,
        out NativeHitTestRect rect
    );

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static unsafe partial bool TextLayout_HitTestTextRange(
        nint textLayout,
        int start,
        int length,
        NativeHitTestRect* rects,
        int rectsCount,
        out int actualRectsCount
    );

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool TextLayout_SetMaxWidth(
        nint textLayout,
        float maxWidth
    );

    [LibraryImport(OpenTempleLib.Path)]
    [return:MarshalAs(UnmanagedType.Bool)]
    private static partial bool TextLayout_SetMaxHeight(
        nint textLayout,
        float maxHeight
    );

    #endregion
}