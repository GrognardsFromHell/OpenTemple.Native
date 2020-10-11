
#pragma once

#include <QSize>
#include <memory>

class VideoFrame {
 public:
  VideoFrame(int width, int height, uint8_t **planes, const int *strides) {
    this->width = width;
    this->height = height;

    this->strides[0] = strides[0];
    this->strides[1] = strides[1];
    this->strides[2] = strides[2];

    // Set it up for YUV4:2:0
    this->planes[0] = std::make_unique<uint8_t[]>(strides[0] * height);
    memcpy(this->planes[0].get(), planes[0], strides[0] * height);

    this->planes[1] = std::make_unique<uint8_t[]>(strides[1] * height / 2);
    memcpy(this->planes[1].get(), planes[1], strides[1] * height / 2);

    this->planes[2] = std::make_unique<uint8_t[]>(strides[2] * height / 2);
    memcpy(this->planes[2].get(), planes[2], strides[2] * height / 2);
  }

  QSize size() const noexcept { return QSize(width, height); }

  int width;
  int height;
  std::unique_ptr<uint8_t[]> planes[3];
  int strides[3]{};
};
using SharedVideoFrame = std::shared_ptr<VideoFrame>;
