using System;
using System.Runtime.InteropServices;

namespace OpenTemple.Interop
{
    public enum NativeLogLevel
    {
        Error,
        Warn,
        Info,
        Debug
    }

    public delegate void NativeLogCallback(NativeLogLevel level, ReadOnlySpan<char> message);

    /// <summary>
    /// Controls how native code can log messages.
    /// </summary>
    public static class NativeLogger
    {
        private static NativeLogCallback _sink;

        public static NativeLogCallback Sink
        {
            set
            {
                _sink = value;
                if (_sink != null)
                {
                    unsafe
                    {
                        Logger_SetSink(&SinkCallback);
                    }
                }
                else
                {
                    Logger_ClearSink();
                }
            }
            get => _sink;
        }

        [UnmanagedCallersOnly]
        private static unsafe void SinkCallback(NativeLogLevel level, char* text, int textLength)
        {
            if (_sink != null)
            {
                _sink(level, new ReadOnlySpan<char>(text, textLength));
            }
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern unsafe void Logger_SetSink(
            delegate* unmanaged<NativeLogLevel, char*, int, void> callback
        );

        [DllImport(OpenTempleLib.Path)]
        private static extern void Logger_ClearSink();
    }
}