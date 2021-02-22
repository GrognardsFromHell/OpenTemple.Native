
#pragma once

#include <mutex>
#include <vector>

#include <soloud/soloud.h>

class SoLoudDynamicSource : public SoLoud::AudioSource {
  friend class SoLoudDynamicSourceInstance;

 public:
  SoLoudDynamicSource(int channelCount, int sampleRate);
  ~SoLoudDynamicSource() override;

  void pushSamples(float* planes[], int sampleCount, bool interleaved);

  void ended();

  SoLoud::AudioSourceInstance* createInstance() override;

 private:
  std::mutex _mutex;

  SoLoudDynamicSourceInstance* _instance = nullptr;

  // Split queues per channel, inefficient but dumb and simple
  std::vector<float> leftSampleQueue;
  std::vector<float> rightSampleQueue;

  bool _atEnd = false;
};
