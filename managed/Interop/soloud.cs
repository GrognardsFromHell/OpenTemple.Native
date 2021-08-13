
using System;

namespace SoLoud
{

    public partial class Wav
    {
        public unsafe int loadMem(ReadOnlySpan<byte> aMem, uint aLength, bool aCopy = false, bool aTakeOwnership = true)
        {
            fixed (void* aMemPtr = aMem)
            {
                return Wav_loadMemEx(objhandle, new IntPtr(aMemPtr), aLength, aCopy, aTakeOwnership);
            }
        }
    }

}
