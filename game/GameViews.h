
#pragma once

#include <QtANGLE/GLES2/gl2.h>
#include "gl_headers.h"

#include <QObject>
#include <optional>

#include "managed_delegate.h"

struct ID3D11Texture2D;

class QSGTexture;

class QQuickWindow;

using GameViewHandle = void *;
using GameViewsHandle = void *;

class GameViews : public QObject {
  friend class GameView;

  friend struct GameViewsCallbacks;

  Q_OBJECT
 public:
  explicit GameViews(GameViewsHandle handle, QObject *parent = nullptr);

  ~GameViews() override;

  std::unique_ptr<GameView> create();

 private:
  // Release the handle underpinning the game views manager
  static std::function<void(GameViewsHandle)> destroy_game_views;
  static std::function<GameViewHandle(GameViewsHandle)> create_game_view;
  const GameViewsHandle _handle;
};

class GameView : QObject {
  friend struct GameViewsCallbacks;

  Q_OBJECT
 public:
  GameView(GameViewHandle handle) : _handle(handle) {}

  ~GameView() override;

  QSGTexture* createTexture(QQuickWindow *);

  QSize size() const { return _size; }

  void setSize(QSize size);

  void setUpdateCallback(const std::function<void()> &updateCallback);

  const std::function<void()> &getUpdateCallback() const;

 private:
  static std::function<void(GameViewHandle)> destroy_game_view;
  static std::function<bool(GameViewHandle, ID3D11Texture2D **texture, int *width, int *height)>
      get_texture;
  static std::function<void(GameViewHandle, int width, int height)> set_size;

  const GameViewHandle _handle;
  std::function<void()> _updateCallback;
  QSize _size;
  std::optional<GLuint> _textureId;
  std::optional<EGLDisplay> _display;
  std::optional<EGLSurface> _surface;
};
