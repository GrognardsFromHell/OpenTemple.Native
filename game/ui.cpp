
#include <QDir>
#include <QThread>
#include <QTimer>

#include "completion_source.h"
#include "managed_delegate.h"
#include "string_interop.h"
#include "ui.h"
#include "utils.h"

struct ID3D11Device;

static struct TypeRegistrar {
  TypeRegistrar() noexcept {
    qRegisterMetaType<Ui*>();
  }
} registrar;

Ui::Ui() {
  static char emptyString = 0;
  static char *qtArgs[]{&emptyString};
  int qtArgsSize = 1;
  _app = std::make_unique<QGuiApplication>(qtArgsSize, qtArgs);

  _engine = std::make_unique<QQmlEngine>();

  _window = std::make_unique<QQuickWindow>();
  _window->installEventFilter(this);

  QObject::connect(_window.get(), &QWindow::visibleChanged, [this](bool visible) {
    if (!visible && _closeCallback) {
      _closeCallback();
    }
  });

  connect(_window.get(), &QQuickWindow::sceneGraphInitialized, this,
          &Ui::handleSceneGraphInitialized);
  connect(_window.get(), &QQuickWindow::sceneGraphInvalidated, this,
          &Ui::handleSceneGraphInvalidated);
  QObject::connect(_window.get(), &QQuickWindow::beforeRendering, [this]() {
    if (_beforeRenderingCallback) {
      _beforeRenderingCallback();
    }
  });
  QObject::connect(_window.get(), &QQuickWindow::beforeRenderPassRecording, [this]() {
    if (_beforeRenderPassRecordingCallback) {
      _beforeRenderPassRecordingCallback();
    }
  });
  QObject::connect(_window.get(), &QQuickWindow::afterRenderPassRecording, [this]() {
    if (_afterRenderPassRecordingCallback) {
      _afterRenderPassRecordingCallback();
    }
  });
  QObject::connect(_window.get(), &QQuickWindow::afterRendering, [this]() {
    if (_afterRenderingCallback) {
      _afterRenderingCallback();
    }
  });

  _transparentCursor = std::make_unique<QCursor>();

  // Just a sanity check that ANGLE is being used (which we can only check
  // indirectly via OpenGL)
  auto rendererInterface = _window->rendererInterface();
  Q_ASSERT(rendererInterface->graphicsApi() == QSGRendererInterface::OpenGL);

  //
  //    QObject::connect(window.get(), &QQuickView::statusChanged,
  //                     [this](auto status) {
  //                       if (status == QQuickView::Ready) {
  //                         _loadViewCompletionSource->succeed(window->rootObject());
  //                       } else if (status == QQuickView::Error) {
  //                         QString errorMessage;
  //                         for (auto &error : window->errors()) {
  //                           if (!errorMessage.isEmpty()) {
  //                             errorMessage.append("\n");
  //                           }
  //                           errorMessage.append(error.toString());
  //                         }
  //                         _loadViewCompletionSource->fail(errorMessage);
  //                       }
  //                     });
}

void Ui::handleSceneGraphInitialized() {
  Q_ASSERT(!_openglContextCreated);
  Q_ASSERT(!_d3d11device);
  _openglContextCreated = true;

  auto glContext = static_cast<QOpenGLContext *>(_window->rendererInterface()->getResource(
      _window.get(), QSGRendererInterface::OpenGLContextResource));

  QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
  _eglDisplay = (EGLDisplay)native->nativeResourceForContext("egldisplay", glContext);
  _eglConfig = (EGLConfig)native->nativeResourceForContext("eglconfig", glContext);

  // Query the underlying D3D11 device
  EGLDeviceEXT eglDevice;
  eglQueryDisplayAttribEXT(_eglDisplay, EGL_DEVICE_EXT, (EGLAttrib *)&eglDevice);
  auto success =
      eglQueryDeviceAttribEXT(eglDevice, EGL_D3D11_DEVICE_ANGLE, (EGLAttrib *)&_d3d11device);
  Q_ASSERT(success);
  _d3d11device->AddRef();
  if (_deviceCreatedCallback) {
    _deviceCreatedCallback(_d3d11device);
  }
}

void Ui::handleSceneGraphInvalidated() {
  Q_ASSERT(_openglContextCreated);
  _openglContextCreated = false;

  _eglDisplay = EGL_NO_DISPLAY;
  _eglConfig = EGL_NO_CONFIG_KHR;

  if (_deviceDestroyedCallback) {
    _deviceDestroyedCallback(_d3d11device);
  }
  if (_d3d11device) {
    _d3d11device->Release();
    _d3d11device = nullptr;
  }
}

void Ui::setConfig(const NativeWindowConfig &config) {
  _window->setMinimumWidth(config.minWidth);
  _window->setMinimumHeight(config.minHeight);
  _window->resize(config.width, config.height);
  if (config.fullScreen) {
    _window->showFullScreen();
  } else {
    auto screen = _window->screen();
    if (screen) {
      // Center on the view on screen
      auto screenSize = screen->availableSize();
      auto x = (screenSize.width() - _window->width()) / 2;
      auto y = (screenSize.height() - _window->height()) / 2;
      _window->setPosition(x, y);
    }

    _window->show();
  }
}
void Ui::queueUpdate() { _window->update(); }

QSize Ui::renderTargetSize() const {
  // If we're rendering to a native window surface, we'll assume that to be our
  // backbuffer size instead
  if (!_window->renderTarget()) {
    return _window->size();
  }

  return _window->renderTargetSize();
}

struct UiCallbacks {
  NativeDelegate<void()> before_rendering;
  NativeDelegate<void()> before_renderpass_recording;
  NativeDelegate<void()> after_renderpass_recording;
  NativeDelegate<void()> after_rendering;
  NativeDelegate<bool(const MouseEvent &)> mouse_event_filter;
  NativeDelegate<bool(const WheelEvent &)> wheel_event_filter;
  NativeDelegate<bool(const KeyEvent &)> key_event_filter;
  NativeDelegate<void()> on_close;
  NativeDelegate<void(ID3D11Device *)> device_created;
  NativeDelegate<void(ID3D11Device *)> device_destroyed;
};

NATIVE_API Ui *ui_create(UiCallbacks callbacks) {
  auto ui = std::make_unique<Ui>();

  ui->setBeforeRenderingCallback(callbacks.before_rendering);
  ui->setBeforeRenderPassRecordingCallback(callbacks.before_renderpass_recording);
  ui->setAfterRenderPassRecordingCallback(callbacks.after_renderpass_recording);
  ui->setAfterRenderingCallback(callbacks.after_rendering);
  ui->setMouseEventFilter(callbacks.mouse_event_filter);
  ui->setWheelEventFilter(callbacks.wheel_event_filter);
  ui->setKeyEventFilter(callbacks.key_event_filter);
  ui->setCloseCallback(callbacks.on_close);
  ui->setDeviceCreatedCallback(callbacks.device_created);
  ui->setDeviceDestroyedCallback(callbacks.device_destroyed);

  return ui.release();
}

NATIVE_API void ui_destroy(Ui *ui) { delete ui; }

NATIVE_API void ui_set_config(Ui &ui, const NativeWindowConfig &config) { ui.setConfig(config); }

NATIVE_API void ui_quit(Ui &ui) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), &QCoreApplication::quit,
                            Qt::QueuedConnection);
}

NATIVE_API void ui_process_events() {
  QCoreApplication::sendPostedEvents();
  QCoreApplication::processEvents();
}

NATIVE_API void ui_hide_cursor(Ui &ui) { ui.hideCursor(); }

NATIVE_API void ui_set_cursor(Ui &ui, QCursor &cursor) { ui.setCursor(cursor); }

NATIVE_API void ui_set_icon(Ui &ui, const char16_t *icon_path) {
  ui.setIcon(QIcon(QString::fromUtf16(icon_path)));
}

NATIVE_API void ui_set_title(Ui &ui, const char16_t *title) {
  ui.setTitle(QString::fromUtf16(title));
}

NATIVE_API void ui_add_search_path(const char16_t *prefix, const char16_t *path) {
  QDir::addSearchPath(QString::fromUtf16(prefix), QString::fromUtf16(path));
}

template <typename T>
using AsyncSuccessCallback = void(T result);
using AsyncErrorCallback = void(const char16_t *);

NATIVE_API void ui_load_view(Ui &ui, const char16_t *path,
                             QObjectCompletionSource *completionSource) {
  //  ui.loadView(QString::fromUtf16(path),
  //              std::unique_ptr<QObjectCompletionSource>(completionSource));
}

NATIVE_API void ui_get_rendertarget_size(Ui &ui, int *width, int *height) {}

NATIVE_API void ui_begin_external_commands(Ui &ui) { ui.window()->beginExternalCommands(); }

NATIVE_API void ui_end_external_commands(Ui &ui) { ui.window()->endExternalCommands(); }

NATIVE_API QQuickItem *ui_get_root_item(Ui &ui) { return ui.window()->contentItem(); }

NATIVE_API void ui_show(Ui &ui) { ui.window()->show(); }
