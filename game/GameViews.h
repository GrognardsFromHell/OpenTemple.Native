
#pragma once

#include <QtANGLE/GLES2/gl2.h>
#include "gl_headers.h"

#include <QmlNet/types/NetReference.h>
#include <QObject>
#include <optional>

#include "ManagedObject.h"
#include "managed_delegate.h"

struct ID3D11Texture2D;

class QSGTexture;

class QQuickWindow;

class GameViews : public QObject {
  friend class GameView;

  friend struct GameViewsCallbacks;

  Q_OBJECT
 public:
  explicit GameViews(QSharedPointer<NetReference> handle, QObject *parent = nullptr);

  std::unique_ptr<GameView> create();

 private:
  static std::function<NetReferenceContainer*(NetReferenceContainer*)> create_game_view;
  const QSharedPointer<NetReference> _handle;
};

class GameView : QObject {
  friend struct GameViewsCallbacks;

  Q_OBJECT
 public:
  explicit GameView(QSharedPointer<NetReference> handle) : _handle(std::move(handle)) {}

  ~GameView() override;

  QSGTexture* createTexture(QQuickWindow *);

  QSize size() const { return _size; }

  void setSize(QSize size);

  void setUpdateCallback(const std::function<void()> &updateCallback);

  const std::function<void()> &getUpdateCallback() const;

  [[nodiscard]] const QSharedPointer<NetReference> &handle() const {
    return _handle;
  }

 private:
  static std::function<void(NetReferenceContainer*)>
      dispose;
  static std::function<bool(NetReferenceContainer*, ID3D11Texture2D **texture, int *width, int *height)>
      get_texture;
  static std::function<void(NetReferenceContainer*, int width, int height)> set_size;

  const QSharedPointer<NetReference> _handle;
  std::function<void()> _updateCallback;
  QSize _size;
  std::optional<GLuint> _textureId;
  std::optional<EGLDisplay> _display;
  std::optional<EGLSurface> _surface;
};
