
#pragma once

#include <QQuickItem>
#include <QtCore/QMutex>

#include "VideoFrame.h"

namespace SoLoud {
class Soloud;
}

class VideoAudioSource;
class VideoPlayer;

class BinkVideoItem : public QQuickItem {
  Q_OBJECT
  Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
  Q_ENUMS(FillMode)
  QML_ELEMENT

 public:
  explicit BinkVideoItem(QQuickItem *parent = nullptr);
  ~BinkVideoItem() override;

  enum FillMode {
    Stretch = Qt::IgnoreAspectRatio,
    PreserveAspectFit = Qt::KeepAspectRatio,
    PreserveAspectCrop = Qt::KeepAspectRatioByExpanding
  };

  void setSoloud(SoLoud::Soloud *soloud) noexcept { _soloud = soloud; }

  Q_INVOKABLE void open(const QString &path);

  FillMode fillMode() const { return m_fillMode; }
  void setFillMode(FillMode mode) {
    if (mode == m_fillMode)
      return;

    m_fillMode = mode;
    update();

    emit fillModeChanged(mode);
  }

 Q_SIGNALS:
  void fillModeChanged(BinkVideoItem::FillMode);
  void ended();

 protected:
  std::unique_ptr<VideoPlayer> _player;
  std::unique_ptr<VideoAudioSource> _audioSource;

  QMutex m_frameMutex;
  bool m_frameChanged = false;
  SharedVideoFrame m_frame;

  QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;

  SoLoud::Soloud *_soloud{};

 private:
  void updateGeometry();
  FillMode m_fillMode = PreserveAspectFit;

  QRectF m_renderedRect;       // Destination pixel coordinates, clipped
  QRectF m_sourceTextureRect;  // Source texture coordinates
};
