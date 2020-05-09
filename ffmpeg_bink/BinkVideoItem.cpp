
#include <QQuickWindow>
#include <QTimer>

#include <soloud/soloud.h>
#include "videoplayer.h"

#include "BinkVideoItem.h"
#include "VideoAudioSource.h"
#include "VideoMaterial.h"

void BinkVideoItem::open(const QString &path) {
  stop();

  QMutexLocker lock(&m_frameMutex);

  _player = std::make_unique<VideoPlayer>();
  if (!_player->open(path.toLocal8Bit())) {
    _player.reset();
    QMetaObject::invokeMethod(this, &BinkVideoItem::ended, Qt::QueuedConnection);
    return;
  }

  setImplicitWidth(_player->width());
  setImplicitHeight(_player->height());

  if (_player->hasAudio() && _soloud) {
    _audioSource =
        std::make_unique<VideoAudioSource>(_player->audioChannels(), _player->audioSampleRate());

    _soloud->play(*_audioSource);
  }

  _player->play(
      [this](double time, uint8_t **planes, const int *strides) {
        QMutexLocker lock(&m_frameMutex);
        m_frameChanged = true;
        m_frame =
            std::make_shared<VideoFrame>(_player->width(), _player->height(), planes, strides);
        QMetaObject::invokeMethod(this, &QQuickItem::update, Qt::QueuedConnection);
      },
      [this](float *planes[], int sampleCount, bool interleaved) {
        QMutexLocker lock(&m_frameMutex);
        if (_audioSource) {
          _audioSource->pushSamples(planes, sampleCount, interleaved);
        }
      },
      [this]() { QMetaObject::invokeMethod(this, &BinkVideoItem::ended, Qt::QueuedConnection); });
}

void BinkVideoItem::stop() {
  if (_audioSource) {
    _audioSource->stop();
    _audioSource.reset();
  }

  if (_player) {
    _player.reset();
  }
}

BinkVideoItem::BinkVideoItem(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
}

QSGNode *BinkVideoItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *data) {
  Q_UNUSED(data);
  auto videoNode = static_cast<QSGVideoNode *>(oldNode);

  QMutexLocker lock(&m_frameMutex);

  bool isFrameModified = false;
  if (m_frameChanged) {
    if (!m_frame) {
      m_frameChanged = false;
      return nullptr;
    }

    if (!videoNode) {
      videoNode = new QSGVideoNode;
    }
  } else {
    if (!videoNode) {
      return nullptr;
    }
  }

  updateGeometry();

  // Negative rotations need lots of %360
  videoNode->setTexturedRectGeometry(m_renderedRect, m_sourceTextureRect);
  if (m_frameChanged) {
    videoNode->setCurrentFrame(m_frame);

    // TODO
    //    if ((q->flushMode() == QDeclarativeVideoOutput::FirstFrame && !m_frameOnFlush.isValid())
    //        || q->flushMode() == QDeclarativeVideoOutput::LastFrame) {
    //      m_frameOnFlush = m_surfaceFormat.handleType() == QAbstractVideoBuffer::NoHandle
    //                       ? m_frame
    //                       : m_frame.image();
    //    }

    m_frameChanged = false;
    m_frame.reset();
  }
  return videoNode;
}

void BinkVideoItem::updateGeometry() {
  const QSizeF frameSize(_player->width(), _player->height());
  const QRectF rect(0, 0, width(), height());

  QRectF m_contentRect;
  if (frameSize.isEmpty()) {
    // this is necessary for item to receive the
    // first paint event and configure video surface.
    m_contentRect = rect;
  } else if (m_fillMode == Stretch) {
    m_contentRect = rect;
  } else if (m_fillMode == PreserveAspectFit || m_fillMode == PreserveAspectCrop) {
    QSizeF scaled = frameSize;
    scaled.scale(rect.size(), m_fillMode == PreserveAspectFit ? Qt::KeepAspectRatio
                                                              : Qt::KeepAspectRatioByExpanding);

    m_contentRect = QRectF(QPointF(), scaled);
    m_contentRect.moveCenter(rect.center());
  }

  const QRectF viewport(0, 0, _player->width(), _player->height());
  const QRectF normalizedViewport(0, 0, 1, 1);
  if (frameSize.isEmpty()) {
    m_renderedRect = rect;
    m_sourceTextureRect = normalizedViewport;
  } else if (m_fillMode == Stretch) {
    m_renderedRect = rect;
    m_sourceTextureRect = normalizedViewport;
  } else if (m_fillMode == PreserveAspectFit) {
    m_sourceTextureRect = normalizedViewport;
    m_renderedRect = m_contentRect;
  } else if (m_fillMode == PreserveAspectCrop) {
    m_renderedRect = rect;
    const qreal contentHeight = m_contentRect.height();
    const qreal contentWidth = m_contentRect.width();

    // Calculate the size of the source rectangle without taking the viewport into account
    const qreal relativeOffsetLeft = -m_contentRect.left() / contentWidth;
    const qreal relativeOffsetTop = -m_contentRect.top() / contentHeight;
    const qreal relativeWidth = rect.width() / contentWidth;
    const qreal relativeHeight = rect.height() / contentHeight;

    // Now take the viewport size into account
    const qreal totalOffsetLeft =
        normalizedViewport.x() + relativeOffsetLeft * normalizedViewport.width();
    const qreal totalOffsetTop =
        normalizedViewport.y() + relativeOffsetTop * normalizedViewport.height();
    const qreal totalWidth = normalizedViewport.width() * relativeWidth;
    const qreal totalHeight = normalizedViewport.height() * relativeHeight;

    m_sourceTextureRect = QRectF(totalOffsetLeft, totalOffsetTop, totalWidth, totalHeight);
  }

  // TODO if (m_surfaceFormat.scanLineDirection() == QVideoSurfaceFormat::BottomToTop) {
  //    qreal top = m_sourceTextureRect.top();
  //    m_sourceTextureRect.setTop(m_sourceTextureRect.bottom());
  //    m_sourceTextureRect.setBottom(top);
  //  }

  // TODO if (m_surfaceFormat.property("mirrored").toBool()) {
  //    qreal left = m_sourceTextureRect.left();
  //    m_sourceTextureRect.setLeft(m_sourceTextureRect.right());
  //    m_sourceTextureRect.setRight(left);
  //  }
}

BinkVideoItem::~BinkVideoItem() = default;
