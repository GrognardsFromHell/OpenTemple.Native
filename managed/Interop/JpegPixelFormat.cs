namespace OpenTemple.Interop;

/// <summary>
/// Specifies the pixel format of the uncompressed data
/// when encoding or decoding a JPEG image.
/// </summary>
public enum JpegPixelFormat : int
{
    RGB,
    BGR,
    RGBX,
    BGRX,
    XBGR,
    XRGB
}