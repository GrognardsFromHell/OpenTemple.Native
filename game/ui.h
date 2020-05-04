
#pragma once

#include "gl_headers.h"

#include <memory>

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickView>
#include <QSGRendererInterface>
#include <QScreen>
#include <utility>
#include "GameViewItem.h"
#include "GameViews.h"
#include "completion_source.h"
#include "events.h"

#include <qpa/qplatformnativeinterface.h>

#include <d3d11.h>

struct NativeWindowConfig {
  int minWidth;
  int minHeight;
  int width;
  int height;
  bool fullScreen;
};

class Ui : public QObject {
  Q_OBJECT
 public:
  std::unique_ptr<QGuiApplication> app;
  std::unique_ptr<QQuickView> view;
  std::unique_ptr<QCursor> transparentCursor;

  explicit Ui() {
    static char *qtArgs[]{""};
    int qtArgsSize = 1;
    app = std::make_unique<QGuiApplication>(qtArgsSize, qtArgs);

    view = std::make_unique<QQuickView>();

    view->installEventFilter(this);

    QObject::connect(view.get(), &QWindow::visibleChanged,
                     [this](bool visible) {
                       if (!visible && _closeCallback) {
                         _closeCallback();
                       }
                     });

    connect(view.get(), &QQuickWindow::sceneGraphInitialized, this,
            &Ui::handleSceneGraphInitialized);
    connect(view.get(), &QQuickWindow::sceneGraphInvalidated, this,
            &Ui::handleSceneGraphInvalidated);

    QObject::connect(view.get(), &QQuickView::statusChanged,
                     [this](auto status) {
                       if (status == QQuickView::Ready) {
                         _loadViewCompletionSource->succeed(view->rootObject());
                       } else if (status == QQuickView::Error) {
                         QString errorMessage;
                         for (auto &error : view->errors()) {
                           if (!errorMessage.isEmpty()) {
                             errorMessage.append("\n");
                           }
                           errorMessage.append(error.toString());
                         }
                         _loadViewCompletionSource->fail(errorMessage);
                       }
                     });

    QObject::connect(view.get(), &QQuickWindow::beforeRendering, [this]() {
      if (_beforeRenderingCallback) {
        _beforeRenderingCallback();
      }
    });
    QObject::connect(view.get(), &QQuickWindow::beforeRenderPassRecording,
                     [this]() {
                       if (_beforeRenderPassRecordingCallback) {
                         _beforeRenderPassRecordingCallback();
                       }
                     });
    QObject::connect(view.get(), &QQuickWindow::afterRenderPassRecording,
                     [this]() {
                       if (_afterRenderPassRecordingCallback) {
                         _afterRenderPassRecordingCallback();
                       }
                     });
    QObject::connect(view.get(), &QQuickWindow::afterRendering, [this]() {
      if (_afterRenderingCallback) {
        _afterRenderingCallback();
      }
    });
    view->setResizeMode(QQuickView::SizeRootObjectToView);

    transparentCursor = std::make_unique<QCursor>();
  }

  QUrl baseUrl() const { return view->engine()->baseUrl(); }

  void setBaseUrl(const QUrl &url) { view->engine()->setBaseUrl(url); }

  void setCursor(QCursor &cursor) { view->setCursor(cursor); }

  void hideCursor() { view->setCursor(*transparentCursor); }

  void setTitle(const QString &title) { view->setTitle(title); }

  void setIcon(const QIcon &icon) { view->setIcon(icon); }

  void setConfig(const NativeWindowConfig &config) {
    view->setMinimumWidth(config.minWidth);
    view->setMinimumHeight(config.minHeight);
    view->resize(config.width, config.height);
    if (config.fullScreen) {
      view->showFullScreen();
    } else {
      auto screen = view->screen();
      if (screen) {
        // Center on the view on screen
        auto screenSize = screen->availableSize();
        auto x = (screenSize.width() - view->width()) / 2;
        auto y = (screenSize.height() - view->height()) / 2;
        view->setPosition(x, y);
      }

      view->show();
    }
  }

  void loadView(const QString &path,
                std::unique_ptr<QObjectCompletionSource> completionSource) {
    if (_loadViewCompletionSource) {
      _loadViewCompletionSource->cancel();
    }
    _loadViewCompletionSource = std::move(completionSource);

    view->setSource(QUrl(path));
  }

  void setBeforeRenderingCallback(std::function<void()> callback) {
    _beforeRenderingCallback = std::move(callback);
  }

  void setBeforeRenderPassRecordingCallback(std::function<void()> callback) {
    _beforeRenderPassRecordingCallback = std::move(callback);
  }

  void setAfterRenderPassRecordingCallback(std::function<void()> callback) {
    _afterRenderPassRecordingCallback = std::move(callback);
  }

  void setAfterRenderingCallback(std::function<void()> callback) {
    _afterRenderingCallback = std::move(callback);
  }

  void setMouseEventFilter(
      const std::function<bool(const MouseEvent &)> &mouseEventFilter) {
    _mouseEventFilter = mouseEventFilter;
  }

  void setWheelEventFilter(
      const std::function<bool(const WheelEvent &)> &wheelEventFilter) {
    _wheelEventFilter = wheelEventFilter;
  }

  void setKeyEventFilter(
      const std::function<bool(const KeyEvent &)> &keyEventFilter) {
    _keyEventFilter = keyEventFilter;
  }

  void setCloseCallback(const std::function<void()> &closeCallback) {
    _closeCallback = closeCallback;
  }

  void setDeviceCreatedCallback(
      const std::function<void(ID3D11Device *)> &deviceCreatedCallback) {
    _deviceCreatedCallback = deviceCreatedCallback;
  }

  void setDeviceDestroyedCallback(
      const std::function<void(ID3D11Device *)> &deviceDestroyedCallback) {
    _deviceDestroyedCallback = deviceDestroyedCallback;
  }

 protected:
  Q_SLOT void handleSceneGraphInitialized();

  Q_SLOT void handleSceneGraphInvalidated();

  bool eventFilter(QObject *object, QEvent *event) override {
    if (object != view.get()) {
      return false;
    }

    auto type = event->type();
    if (_mouseEventFilter &&
        (type == QEvent::MouseButtonDblClick ||
         type == QEvent::MouseButtonPress ||
         type == QEvent::MouseButtonRelease || type == QEvent::MouseMove ||
         type == QEvent::MouseTrackingChange)) {
      MouseEvent mouseEvent(*static_cast<QMouseEvent *>(event));
      return _mouseEventFilter(mouseEvent);
    } else if (_wheelEventFilter && type == QEvent::Wheel) {
      WheelEvent wheelEvent(*static_cast<QWheelEvent *>(event));
      return _wheelEventFilter(wheelEvent);
    } else if (_keyEventFilter &&
               (type == QEvent::KeyPress || type == QEvent::KeyRelease)) {
      KeyEvent keyEvent(*static_cast<QKeyEvent *>(event));
      return _keyEventFilter(keyEvent);
    }
    return false;
  }

 private:
  EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
  EGLConfig _eglConfig = EGL_NO_CONFIG_KHR;
  ID3D11Device *_d3d11device = nullptr;
  bool _openglContextCreated = false;
  std::function<void()> _beforeRenderingCallback;
  std::function<void()> _beforeRenderPassRecordingCallback;
  std::function<void()> _afterRenderPassRecordingCallback;
  std::function<void()> _afterRenderingCallback;
  std::function<bool(const MouseEvent &e)> _mouseEventFilter;
  std::function<bool(const WheelEvent &e)> _wheelEventFilter;
  std::function<bool(const KeyEvent &e)> _keyEventFilter;
  std::function<void()> _closeCallback;
  std::function<void(ID3D11Device *)> _deviceCreatedCallback;
  std::function<void(ID3D11Device *)> _deviceDestroyedCallback;

  std::unique_ptr<QObjectCompletionSource> _loadViewCompletionSource;
};
