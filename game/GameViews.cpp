
#include "gl_headers.h"

#include <qpa/qplatformnativeinterface.h>
#include <QGuiApplication>
#include <QQuickView>

#include "GameViewItem.h"
#include "GameViews.h"
#include "ui.h"
#include "utils.h"

std::unique_ptr<GameView> GameViews::create() {
  NetReferenceContainer self{_handle};
  auto gameViewRef = create_game_view(&self);
  if (!gameViewRef || !gameViewRef->instance) {
    return nullptr;
  }
  auto result = std::make_unique<GameView>(std::move(gameViewRef->instance));
  delete gameViewRef;
  return result;
}

GameViews::GameViews(QSharedPointer<NetReference> handle, QObject *parent)
    : QObject(parent), _handle(std::move(handle)) {}

struct GameViewsCallbacks {
  NativeDelegate<NetReferenceContainer *(NetReferenceContainer *)> create_game_view;
  NativeDelegate<void(NetReferenceContainer *)> dispose_game_view;
  NativeDelegate<bool(NetReferenceContainer *, ID3D11Texture2D **texture, int *width, int *height)>
      get_texture;
  NativeDelegate<void(NetReferenceContainer *, int width, int height)> set_size;

  void install() {
    GameViews::create_game_view = create_game_view;

    GameView::dispose = dispose_game_view;
    GameView::get_texture = get_texture;
    GameView::set_size = set_size;
  }
};

NATIVE_API void gameviews_set_callbacks(GameViewsCallbacks callbacks) { callbacks.install(); }

NATIVE_API bool gameviews_install(Ui &ui, NetReferenceContainer *handle) {
  auto context = ui.engine()->rootContext();
  if (context->contextProperty(QStringLiteral("gameViews")).isValid()) {
    return false;
  }

  auto views = new GameViews(std::move(handle->instance));
  context->setContextProperty(QStringLiteral("gameViews"), views);
  QQmlEngine::setObjectOwnership(views, QQmlEngine::JavaScriptOwnership);
  return true;
}

NATIVE_API bool gameviews_uninstall(Ui &ui) {
  auto context = ui.engine()->rootContext();

  context->setContextProperty(QStringLiteral("gameViews"), QVariant());
  return true;
}

GameView::~GameView() {
  if (_handle) {
    NetReferenceContainer container{_handle};
    dispose(&container);
  }

  if (_textureId) {
    GLuint tid = _textureId.value();
    glDeleteTextures(1, &tid);

    if (_display && _surface) {
      eglDestroySurface(*_display, *_surface);
    }
  }
}

QSGTexture *GameView::createTexture(QQuickWindow *window) {
  ID3D11Texture2D *texture;
  int width, height;
  NetReferenceContainer container{_handle};
  if (!get_texture(&container, &texture, &width, &height)) {
    return nullptr;
  }

  GLuint tid;
  if (_textureId) {
    tid = *_textureId;
    glBindTexture(GL_TEXTURE_2D, tid);
    if (_display && _surface) {
      eglReleaseTexImage(*_display, *_surface, EGL_BACK_BUFFER);
      eglDestroySurface(*_display, *_surface);
      _display.reset();
      _surface.reset();
    }
  } else {
    glGenTextures(1, &tid);
    _textureId = tid;
    glBindTexture(GL_TEXTURE_2D, tid);
  }

  auto glContext = window->openglContext();
  QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
  EGLDisplay eglDisplay = (EGLDisplay)native->nativeResourceForContext("egldisplay", glContext);
  _display = eglDisplay;
  EGLDisplay eglConfig = (EGLConfig)native->nativeResourceForContext("eglconfig", glContext);

  EGLint attribs[] = {
      EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA, EGL_TEXTURE_TARGET, EGL_TEXTURE_2D, EGL_NONE,
  };
  auto surface = eglCreatePbufferFromClientBuffer(eglDisplay, EGL_D3D_TEXTURE_ANGLE,
                                                  (EGLClientBuffer)texture, eglConfig, attribs);
  _surface = surface;
  if (surface == EGL_NO_SURFACE) {
    qDebug("eglCreatePbufferFromClientBuffer failed");
    return nullptr;
  }

  if (!eglBindTexImage(eglDisplay, surface, EGL_BACK_BUFFER)) {
    qDebug("eglBindTexImage failed");
    return nullptr;
  }

  return window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture, &tid, 0,
                                               QSize(width, height));
}

void GameView::setSize(QSize size) {
  _size = size;
  NetReferenceContainer container{_handle};
  set_size(&container, _size.width(), _size.height());
}

void GameView::setUpdateCallback(const std::function<void()> &updateCallback) {
  _updateCallback = updateCallback;
}

const std::function<void()> &GameView::getUpdateCallback() const { return _updateCallback; }

decltype(GameViews::create_game_view) GameViews::create_game_view;
decltype(GameView::dispose) GameView::dispose;
decltype(GameView::get_texture) GameView::get_texture;
decltype(GameView::set_size) GameView::set_size;
