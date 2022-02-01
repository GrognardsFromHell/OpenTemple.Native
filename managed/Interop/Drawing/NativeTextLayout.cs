using System;
using System.Runtime.InteropServices;

namespace OpenTemple.Interop.Drawing;

public class NativeTextLayout : IDisposable
{
    public IntPtr NativePointer { get; private set; }

    public NativeTextLayout(IntPtr native)
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

    public bool HitTest(float x, float y, out int start, out int length, out bool trailingHit)
    {
        return TextLayout_HitTest(NativePointer, x, y, out start, out length, out trailingHit);
    }

    public NativeLineMetrics[] GetLineMetrics()
    {
        GetLineMetrics(Span<NativeLineMetrics>.Empty, out var count);
        var metrics = new NativeLineMetrics[count];
        GetLineMetrics(metrics, out _);
        return metrics;
    }

    public void SetMaxWidth(float maxWidth) => TextLayout_SetMaxWidth(NativePointer, maxWidth);

    public void SetMaxHeight(float maxHeight) => TextLayout_SetMaxHeight(NativePointer, maxHeight);

    private void ReleaseUnmanagedResources()
    {
        if (NativePointer != IntPtr.Zero)
        {
            TextLayout_Free(NativePointer);
            NativePointer = IntPtr.Zero;
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

    [DllImport(OpenTempleLib.Path)]
    private static extern bool TextLayout_Free(IntPtr textLayout);

    [DllImport(OpenTempleLib.Path)]
    private static extern bool TextLayout_SetStyle(IntPtr textLayout,
        uint start,
        uint length,
        NativeTextStyleProperty properties,
        ref NativeTextStyle style,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string error
    );

    [DllImport(OpenTempleLib.Path)]
    [SuppressGCTransition]
    private static extern void TextLayout_GetMetrics(IntPtr textLayout, out NativeMetrics metrics);

    [DllImport(OpenTempleLib.Path)]
    private static extern unsafe bool TextLayout_GetLineMetrics(IntPtr textLayout,
        NativeLineMetrics* metrics,
        int metricsCount,
        out int actualLineCount);

    [DllImport(OpenTempleLib.Path)]
    private static extern bool TextLayout_HitTest(
        IntPtr textLayout,
        float x, float y,
        out int start,
        out int length,
        out bool trailingHit
    );

    [DllImport(OpenTempleLib.Path)]
    private static extern void TextLayout_SetMaxWidth(
        IntPtr textLayout,
        float maxWidth
    );

    [DllImport(OpenTempleLib.Path)]
    private static extern void TextLayout_SetMaxHeight(
        IntPtr textLayout,
        float maxHeight
    );

    #endregion
}