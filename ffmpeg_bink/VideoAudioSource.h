
#pragma once

#include <mutex>
#include <vector>

#include <soloud/soloud.h>
#include "videoplayer.h"

class VideoAudioSource : public SoLoud::AudioSource {
  friend class VideoAudioSourceInstance;

 public:
  VideoAudioSource(int channelCount, int sampleRate);
  ~VideoAudioSource() override;

  void pushSamples(float* planes[], int sampleCount, bool interleaved);

  void ended();

  SoLoud::AudioSourceInstance* createInstance() override;

 private:
  std::mutex _mutex;

  VideoAudioSourceInstance* _instance = nullptr;

  // Split queues per channel, inefficient but dumb and simple
  std::vector<float> leftSampleQueue;
  std::vector<float> rightSampleQueue;

  bool _atEnd = false;
};
