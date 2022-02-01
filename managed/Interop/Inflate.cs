using System;
using System.IO;
using System.Runtime.InteropServices;

namespace OpenTemple.Interop;

public static class Inflate
{
    public static unsafe void Uncompress(ReadOnlySpan<byte> source, Span<byte> destination)
    {
        if (source.Length == 0 || destination.Length == 0)
        {
            return;
        }

        var srcLength = (nuint) source.Length;
        var destLength = (nuint) destination.Length;

        int result;
        fixed (byte* sourcePtr = source)
        {
            fixed (byte* destPtr = destination)
            {
                result = Inflate_Uncompress(destPtr, &destLength, sourcePtr, &srcLength);
            }
        }

        switch (result)
        {
            case 0:
                return; // OK
            case 1:
                throw new OutOfMemoryException();
            case 2:
                throw new InvalidDataException(
                    "The destination buffer didn't have enough space for the uncompressed data."
                );
            case 3:
                throw new InvalidDataException("The compressed data is corrupted.");
            default:
                throw new Exception("An unknown error occurred during decompression");
        }
    }

    [DllImport(OpenTempleLib.Path)]
    private static extern unsafe int Inflate_Uncompress(byte* dest, nuint* destLen, [In] byte* src, nuint* srcLen);
}
