using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTemple.Interop
{
    [SuppressUnmanagedCodeSecurity]
    public static class QtBuiltInTypeInterop
    {
        #region QString

        public static readonly int QStringSize = QString_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QString_size();

        // Reads the UTF-16 string from a pointer to a QString instance
        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        public static extern unsafe string QString_read(void* instance);

        #endregion

        #region QByteArray

        public static readonly int QByteArraySize = QByteArray_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QByteArray_size();

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QByteArray_ctor(void* memory, byte[] data, int dataLength);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QByteArray_ctor_default(void* memory);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QByteArray_dtor(void* memory);

        public static unsafe byte[] QByteArray_read(void* instance)
        {
            if (instance == null)
            {
                return null;
            }

            QByteArray_read(instance, out var data, out var dataSize);
            if (dataSize == 0)
            {
                return Array.Empty<byte>();
            }

            var result = new byte[dataSize];
            Marshal.Copy(data, result, 0, dataSize);

            return result;
        }

        // Reads the contents of a QByteArray
        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void QByteArray_read(void* instance, out IntPtr dataOut, out int sizeOut);

        #endregion

        #region QSize

        public static readonly int QSizeSize = QSize_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QSize_size();

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QSize_ctor(void* instance, int width, int height);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QSize_ctor_default(void* instance);

        public static unsafe Size QSize_read(void* qSizePtr)
        {
            QSize_read(qSizePtr, out var x, out var y);
            return new Size(x, y);
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void QSize_read(void* instance, out int x, out int y);

        #endregion

        #region QSizeF

        public static readonly int QSizeFSize = QSizeF_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QSizeF_size();

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QSizeF_ctor(void* instance, double width, double height);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QSizeF_ctor_default(void* instance);

        public static unsafe SizeF QSizeF_read(void* qSizePtr)
        {
            QSizeF_read(qSizePtr, out var width, out var height);
            return new SizeF((float) width, (float) height);
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void QSizeF_read(void* instance, out double width, out double height);

        #endregion

        #region QRect

        public static readonly int QRectSize = QRect_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QRect_size();

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QRect_ctor(void* instance, int x, int y, int width, int height);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QRect_ctor_default(void* instance);

        public static unsafe Rectangle QRect_read(void* qSizePtr)
        {
            QRect_read(qSizePtr, out var x, out var y, out var width, out var height);
            return new Rectangle(x, y, width, height);
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void QRect_read(void* instance, out int x, out int y, out int width,
            out int height);

        #endregion

        #region QRectF

        public static readonly int QRectFSize = QRectF_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QRectF_size();

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QRectF_ctor(void* instance, double x, double y, double width, double height);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QRectF_ctor_default(void* instance);

        public static unsafe RectangleF QRectF_read(void* qSizePtr)
        {
            QRectF_read(qSizePtr, out var x, out var y, out var width, out var height);
            return new RectangleF((float) x, (float) y, (float) width, (float) height);
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void QRectF_read(void* instance, out double x, out double y, out double width,
            out double height);

        #endregion

        #region QPoint

        public static readonly int QPointSize = QPoint_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QPoint_size();

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QPoint_ctor(void* instance, int x, int y);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QPoint_ctor_default(void* instance);

        public static unsafe Point QPoint_read(void* qPointPtr)
        {
            QPoint_read(qPointPtr, out var x, out var y);
            return new Point(x, y);
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void QPoint_read(void* instance, out int x, out int y);

        #endregion

        #region QPointF

        public static readonly int QPointFSize = QPointF_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QPointF_size();

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QPointF_ctor(void* instance, double x, double y);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QPointF_ctor_default(void* instance);

        public static unsafe PointF QPointF_read(void* qPointPtr)
        {
            QPointF_read(qPointPtr, out var x, out var y);
            return new PointF((float) x, (float) y);
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void QPointF_read(void* instance, out double x, out double y);

        #endregion

        #region QColor

        public static readonly int QColorSize = QColor_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QColor_size();

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QColor_ctor(void* instance, int argb);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QColor_ctor_default(void* instance);

        public static unsafe Color QColor_read(void* qColorPtr)
        {
            QColor_read(qColorPtr, out var argb);
            return Color.FromArgb(argb);
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void QColor_read(void* qColorPtr, out int argb);

        #endregion

        #region QUrl

        public static readonly int QUrlSize = QUrl_size();

        [DllImport(OpenTempleLib.Path)]
        private static extern int QUrl_size();

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        public static extern unsafe void QUrl_ctor(void* memory, string url, int urlLen);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QUrl_ctor_default(void* memory);

        [DllImport(OpenTempleLib.Path)]
        public static extern unsafe void QUrl_dtor(void* memory);

        [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
        public static extern unsafe string QUrl_read(void* instance);

        #endregion
    }
}