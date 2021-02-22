using System;
using System.Runtime.InteropServices;
using SoLoud;

namespace OpenTemple.Interop
{
    public unsafe class SoLoudDynamicSource : SoloudObject, IDisposable
    {
        public SoLoudDynamicSource(int channelCount, int sampleRate)
        {
            objhandle = SoLoudDynamicSource_Create(channelCount, sampleRate);
        }

        public void PushSamples(ReadOnlySpan<float> channel1, ReadOnlySpan<float> channel2)
        {
            if (channel1.IsEmpty)
            {
                if (!channel2.IsEmpty)
                {
                    throw new ArgumentException("If channel1 is not provided, channel2 must not be provided either");
                }
                return; // No samples
            }

            if (!channel2.IsEmpty && channel1.Length != channel2.Length)
            {
                throw new ArgumentException("Both channel1 and channel2 must have the same number of samples.");
            }

            var planes = stackalloc float*[2];
            fixed (float* channel1Data = channel1)
            {
                planes[0] = channel1Data;
                if (channel2.IsEmpty)
                {
                    planes[1] = null;
                    SoLoudDynamicSource_PushSamples(objhandle, planes, channel1.Length, false);
                }
                else
                {
                    fixed (float* channel2Data = channel2)
                    {
                        planes[1] = channel2Data;
                        SoLoudDynamicSource_PushSamples(objhandle, planes, channel1.Length, false);
                    }
                }
            }
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern IntPtr SoLoudDynamicSource_Create(int channelCount, int sampleRate);

        [DllImport(OpenTempleLib.Path)]
        private static extern void SoLoudDynamicSource_PushSamples(IntPtr source,
            float** planes,
            int sampleCount,
            [MarshalAs(UnmanagedType.Bool)]
            bool interleaved);

        [DllImport(OpenTempleLib.Path)]
        private static extern void SoLoudDynamicSource_Free(IntPtr source);

        private void ReleaseUnmanagedResources()
        {
            if (objhandle != IntPtr.Zero)
            {
                SoLoudDynamicSource_Free(objhandle);
            }
            objhandle = IntPtr.Zero;
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        ~SoLoudDynamicSource()
        {
            ReleaseUnmanagedResources();
        }
    }
}