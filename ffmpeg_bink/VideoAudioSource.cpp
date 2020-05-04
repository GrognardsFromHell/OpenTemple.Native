
#include <algorithm>

#include "VideoAudioSource.h"

static int consumeSamples(std::vector<float> &queue, float *samplesOut,
                          unsigned int requestedCount) {
  auto copySamples = std::min<size_t>(requestedCount, queue.size());
  std::copy(queue.begin(), queue.begin() + copySamples, samplesOut);

  std::copy(queue.begin() + copySamples, queue.end(), queue.begin());
  queue.resize(queue.size() - copySamples);
  return copySamples;
}

class VideoAudioSourceInstance : public SoLoud::AudioSourceInstance {
 public:
  explicit VideoAudioSourceInstance(VideoAudioSource *source) : source(source) {}
  ~VideoAudioSourceInstance() override {
    if (source && source->_instance == this) {
      source->_instance = nullptr;
    }
  }

  unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead,
                        unsigned int aBufferSize) override {
    if (!source) {
      return 0;
    }

    std::lock_guard<std::mutex> lg(source->_mutex);
    auto copyCount = consumeSamples(source->leftSampleQueue, aBuffer, aSamplesToRead);
    if (source->mChannels == 2) {
      consumeSamples(source->rightSampleQueue, aBuffer, aSamplesToRead);
    }
    return copyCount;
  }

  bool hasEnded() override { return !source || source->_atEnd; }

  SoLoud::result seek(SoLoud::time aSeconds, float *mScratch, unsigned int mScratchSize) override {
    return SoLoud::NOT_IMPLEMENTED;
  }

  VideoAudioSource *source;
};

static void appendPlanarSamples(std::vector<float> &queue, float *samples, int count) {
  auto curSize = queue.size();
  queue.resize(curSize + count);
  std::copy(samples, samples + count, queue.begin() + curSize);
}

VideoAudioSource::VideoAudioSource(int channelCount, int sampleRate) {
  mFlags = SINGLE_INSTANCE | INAUDIBLE_TICK;
  mChannels = channelCount;
  mBaseSamplerate = (float)sampleRate;
}

void VideoAudioSource::pushSamples(float **planes, int sampleCount, bool interleaved) {
  if (interleaved) {
    std::abort();
  }

  std::lock_guard<std::mutex> lg(_mutex);
  appendPlanarSamples(leftSampleQueue, planes[0], sampleCount);
  if (mChannels == 2) {
    appendPlanarSamples(rightSampleQueue, planes[1], sampleCount);
  }
}

SoLoud::AudioSourceInstance *VideoAudioSource::createInstance() {
  std::lock_guard<std::mutex> lg(_mutex);
  if (_instance) {
    return nullptr;
  }

  _instance = new VideoAudioSourceInstance(this);
  return _instance;
}

VideoAudioSource::~VideoAudioSource() {
  std::lock_guard<std::mutex> lg(_mutex);
  if (_instance) {
    _instance->source = nullptr;
  }
}

void VideoAudioSource::ended() { _atEnd = true; }
