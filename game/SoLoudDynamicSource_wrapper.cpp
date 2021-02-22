
#include "SoLoudDynamicSource.h"
#include "utils.h"

NATIVE_API SoLoudDynamicSource *SoLoudDynamicSource_Create(int channelCount, int sampleRate) {
  return new SoLoudDynamicSource(channelCount, sampleRate);
}

NATIVE_API void SoLoudDynamicSource_PushSamples(SoLoudDynamicSource *source,
                                                float *planes[],
                                                int sampleCount,
                                                int interleaved) {
  source->pushSamples(planes, sampleCount, interleaved != 0);
}

NATIVE_API void SoLoudDynamicSource_Free(SoLoudDynamicSource *source) {
  source->stop();
  delete source;
}
