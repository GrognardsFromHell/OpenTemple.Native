
using System;

namespace SoLoud
{

    public partial class Wav
    {
        public unsafe int loadMem(ReadOnlySpan<byte> aMem)
        {
            fixed (void* aMemPtr = aMem)
            {
                return Wav_loadMemEx(objhandle, new IntPtr(aMemPtr), (uint) aMem.Length, true, false);
            }
        }
    }

}
