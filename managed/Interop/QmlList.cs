using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTemple.Interop
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct QmlList<T> : IList<T> where T : QObjectBase
    {
        private readonly IntPtr obj;
        private readonly IntPtr data;

        private readonly IntPtr append;
        private readonly IntPtr count;
        private readonly IntPtr at;
        private readonly IntPtr clear;
        private readonly IntPtr replace;
        private readonly IntPtr removeLast;

        public IEnumerator<T> GetEnumerator()
        {
            throw new NotImplementedException();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public void Add(T item)
        {
            if (append == IntPtr.Zero)
            {
                throw new NotSupportedException();
            }

            unsafe
            {
                var value = item != null ? item.NativePointer : null;
                fixed (void* ptr = &this)
                {
                    QmlListInterop.QmlListInterop_append(ptr, value);
                }
            }
        }

        public void Clear()
        {
            if (clear == IntPtr.Zero)
            {
                throw new NotSupportedException();
            }

            unsafe
            {
                fixed (void* ptr = &this)
                {
                    QmlListInterop.QmlListInterop_clear(ptr);
                }
            }
        }

        public bool Contains(T item)
        {
            return IndexOf(item) != -1;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            throw new NotImplementedException();
        }

        public bool Remove(T item)
        {
            throw new NotImplementedException();
        }

        public int Count
        {
            get
            {
                if (count == IntPtr.Zero)
                {
                    return 0;
                }

                unsafe
                {
                    fixed (void* ptr = &this)
                    {
                        return QmlListInterop.QmlListInterop_count(ptr);
                    }
                }
            }
        }

        public bool IsReadOnly => removeLast == IntPtr.Zero && replace == IntPtr.Zero && append == IntPtr.Zero;

        public int IndexOf(T item)
        {
            unsafe
            {
                var value = item != null ? item.NativePointer : null;
                fixed (void* ptr = &this)
                {
                    return QmlListInterop.QmlListInterop_indexOf(ptr, value);
                }
            }
        }

        public void Insert(int index, T item)
        {
            throw new NotImplementedException();
        }

        public void RemoveAt(int index)
        {
            throw new NotImplementedException();
        }

        public T this[int index]
        {
            get
            {
                if (at == IntPtr.Zero)
                {
                    throw new IndexOutOfRangeException();
                }

                unsafe
                {
                    fixed (void* ptr = &this)
                    {
                        return QObjectBase.GetQObjectProxy<T>(QmlListInterop.QmlListInterop_at(ptr, index));
                    }
                }
            }
            set
            {
                if (replace == IntPtr.Zero)
                {
                    throw new IndexOutOfRangeException();
                }

                unsafe
                {
                    fixed (void* ptr = &this)
                    {
                        QmlListInterop.QmlListInterop_replace(ptr, index, value != null ? value.NativePointer : null);
                    }
                }
            }
        }
    }

    [SuppressUnmanagedCodeSecurity]
    internal class QmlListInterop
    {
        static QmlListInterop()
        {
            var expectedSize = QQmlListProperty_size();
            int actualSize;
            unsafe
            {
                actualSize = sizeof(QmlList<QObjectBase>);
            }

            if (actualSize != expectedSize)
            {
                throw new InvalidOperationException(
                    $"Expected size {expectedSize} of QmlList differs from actual {actualSize}.");
            }
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern int QQmlListProperty_size();

        [DllImport(OpenTempleLib.Path)]
        internal static extern unsafe void QmlListInterop_append(void* list, void* value);

        [DllImport(OpenTempleLib.Path)]
        internal static extern unsafe int QmlListInterop_count(void* list);

        [DllImport(OpenTempleLib.Path)]
        internal static extern unsafe IntPtr QmlListInterop_at(void* list, int index);

        [DllImport(OpenTempleLib.Path)]
        internal static extern unsafe void QmlListInterop_clear(void* list);

        [DllImport(OpenTempleLib.Path)]
        internal static extern unsafe void QmlListInterop_replace(void* list, int index, void* value);

        [DllImport(OpenTempleLib.Path)]
        internal static extern unsafe void QmlListInterop_removeLast(void* list);

        [DllImport(OpenTempleLib.Path)]
        internal static extern unsafe int QmlListInterop_indexOf(void* list, void* entry);
    }
}