
#pragma once

#include <functional>
#include <memory>
#include <string>

struct VideoContext;

#if BUILDING_VIDEOPLAYER
#define VIDEOPLAYER_API __declspec(dllexport)
#else
#define VIDEOPLAYER_API __declspec(dllimport)
#endif

enum class BinkColorSpace { Jpeg, Mpeg };

class VIDEOPLAYER_API VideoPlayer {
 public:
  // Format is ALWAYS YUV420
  using OnVideoFrame = std::function<void(double time, uint8_t** planes, const int* strides)>;
  using OnAudioSamples = std::function<void(float* planes[], int sampleCount, bool interleaved)>;
  using OnStop = std::function<void()>;

  VideoPlayer();
  virtual ~VideoPlayer();

  bool open(const char* path);

  void play(OnVideoFrame onVideoFrame, OnAudioSamples onAudioSamples, OnStop onStop);
  void stop();

  bool atEnd() const noexcept;

  const std::string& error() const;

  bool hasVideo() const noexcept;
  int width() const noexcept;
  int height() const noexcept;

  /**
   * @return True if this video has audio data.
   */
  bool hasAudio() const noexcept;

  /**
   * @return The audio sample rate in samples per second.
   */
  int audioSampleRate() const noexcept;

  /**
   * @return The number of audio channels, guaranteed to be 1 or 2.
   */
  int audioChannels() const noexcept;

 private:
  std::unique_ptr<VideoContext> video;
};
