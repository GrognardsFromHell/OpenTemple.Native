
#include <QDir>
#include <QThread>
#include <QTimer>

#include "completion_source.h"
#include "managed_delegate.h"
#include "ui.h"
#include "utils.h"
#include "string_interop.h"

struct ID3D11Device;

void Ui::handleSceneGraphInitialized() {
  Q_ASSERT(!_openglContextCreated);
  Q_ASSERT(!_d3d11device);
  _openglContextCreated = true;

  auto glContext =
      static_cast<QOpenGLContext *>(view->rendererInterface()->getResource(
          view.get(), QSGRendererInterface::OpenGLContextResource));

  QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
  _eglDisplay =
      (EGLDisplay)native->nativeResourceForContext("egldisplay", glContext);
  _eglConfig =
      (EGLConfig)native->nativeResourceForContext("eglconfig", glContext);

  // Query the underlying D3D11 device
  EGLDeviceEXT eglDevice;
  eglQueryDisplayAttribEXT(_eglDisplay, EGL_DEVICE_EXT,
                           (EGLAttrib *)&eglDevice);
  auto success = eglQueryDeviceAttribEXT(eglDevice, EGL_D3D11_DEVICE_ANGLE,
                                         (EGLAttrib *)&_d3d11device);
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

  // Just a sanity check that ANGLE is being used (which we can only check
  // indirectly via OpenGL)
  auto rendererInterface = ui->view->rendererInterface();
  Q_ASSERT(rendererInterface->graphicsApi() == QSGRendererInterface::OpenGL);

  ui->setBeforeRenderingCallback(callbacks.before_rendering);
  ui->setBeforeRenderPassRecordingCallback(
      callbacks.before_renderpass_recording);
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

NATIVE_API void ui_set_config(Ui &ui, const NativeWindowConfig &config) {
  ui.setConfig(config);
}

NATIVE_API void ui_update(Ui &ui) { ui.view->update(); }

NATIVE_API void ui_quit(Ui &ui) {
  QMetaObject::invokeMethod(ui.app.get(), &QCoreApplication::quit,
                            Qt::QueuedConnection);
}

NATIVE_API void ui_process_events() {
  QCoreApplication::sendPostedEvents();
  QCoreApplication::processEvents();
}

NATIVE_API void ui_set_baseurl(Ui &ui, const char16_t *baseUrlString) {
  QUrl baseUrl(QString::fromUtf16(baseUrlString));
  ui.setBaseUrl(baseUrl);
}

NATIVE_API char16_t* ui_get_baseurl(Ui &ui) {
  return copyString(ui.baseUrl().toString());
}

NATIVE_API void ui_hide_cursor(Ui &ui) { ui.hideCursor(); }

NATIVE_API void ui_set_cursor(Ui &ui, QCursor &cursor) { ui.setCursor(cursor); }

NATIVE_API void ui_set_icon(Ui &ui, const char16_t *icon_path) {
  ui.setIcon(QIcon(QString::fromUtf16(icon_path)));
}

NATIVE_API void ui_set_title(Ui &ui, const char16_t *title) {
  ui.setTitle(QString::fromUtf16(title));
}

NATIVE_API void ui_add_search_path(const char16_t *prefix,
                                   const char16_t *path) {
  QDir::addSearchPath(QString::fromUtf16(prefix), QString::fromUtf16(path));
}

template <typename T>
using AsyncSuccessCallback = void(T result);
using AsyncErrorCallback = void(const char16_t *);

NATIVE_API void ui_load_view(Ui &ui, const char16_t *path,
                             QObjectCompletionSource *completionSource) {
  ui.loadView(QString::fromUtf16(path),
              std::unique_ptr<QObjectCompletionSource>(completionSource));
}

NATIVE_API void ui_get_rendertarget_size(Ui &ui, int *width, int *height) {
  // If we're rendering to a native window surface, we'll assume that to be our
  // backbuffer size instead
  if (!ui.view->renderTarget()) {
    *width = ui.view->width();
    *height = ui.view->height();
    return;
  }

  auto size = ui.view->renderTargetSize();
  *width = size.width();
  *height = size.height();
}

NATIVE_API void ui_begin_external_commands(Ui &ui) {
  ui.view->beginExternalCommands();
}

NATIVE_API void ui_end_external_commands(Ui &ui) {
  ui.view->endExternalCommands();
}

NATIVE_API QQuickItem* ui_get_root_item(Ui &ui) {
  return ui.view->contentItem();
}

NATIVE_API void ui_show(Ui &ui) { ui.view->show(); }
