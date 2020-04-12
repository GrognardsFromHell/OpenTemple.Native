
#pragma once

#include <memory>

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#include <QGuiApplication>
#include <QQuickRenderControl>
#include <QQuickView>
#include <QQmlContext>
#include <QOpenGLContext>
#include <QSGRendererInterface>
#include <QQuickItem>
#include <QScreen>
#include "GameViewItem.h"
#include "GameViews.h"
#include "completion_source.h"
#include "qobject_interop.h"

#include <qpa/qplatformnativeinterface.h>

struct NativeWindowConfig {
    int minWidth;
    int minHeight;
    int width;
    int height;
    bool fullScreen;
};

class Ui {
public:
    std::unique_ptr<QGuiApplication> app;
    std::unique_ptr<GameViews> gameViews;
    std::unique_ptr<QQuickView> view;
    std::unique_ptr<QCursor> transparentCursor;

    explicit Ui(struct GameViewFunctions *gameViewFunctions) {
        qmlRegisterType<GameViewItem>("OpenTemple", 1, 0, "GameView");

        char *qtArgs[]{
                ""
        };
        int qtArgsSize = 1;
        app = std::make_unique<QGuiApplication>(qtArgsSize, qtArgs);

        gameViews = std::make_unique<GameViews>(*gameViewFunctions);

        view = std::make_unique<QQuickView>();

        QObject::connect(view.get(), &QQuickWindow::openglContextCreated, [this](auto glContext) {
            Q_ASSERT(!_openglContextCreated);
            _openglContextCreated = true;
            QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
            _eglDisplay = (EGLDisplay) native->nativeResourceForContext("egldisplay", glContext);
            _eglConfig = (EGLConfig) native->nativeResourceForContext("eglconfig", glContext);

            // Query the underlying D3D11 device
            EGLDeviceEXT eglDevice;
            eglQueryDisplayAttribEXT(_eglDisplay, EGL_DEVICE_EXT, (EGLAttrib *) &eglDevice);
            auto success = eglQueryDeviceAttribEXT(eglDevice, EGL_D3D11_DEVICE_ANGLE, (EGLAttrib *) &_d3d11Device);
            Q_ASSERT(success);
        });

        QObject::connect(view.get(), &QQuickView::statusChanged, [this](auto status) {
            if (status == QQuickView::Ready) {
                _loadViewCompletionSource->succeed(wrap_qobject(view->rootObject()));
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
            beforeRendering();
        });

        view->rootContext()->setContextProperty("gameViews", gameViews.get());
        view->setResizeMode(QQuickView::SizeRootObjectToView);
        view->show();

        transparentCursor = std::make_unique<QCursor>();
    }

    void setSource(const QUrl &url) {
        view->setSource(url);
    }

    void render() {
        view->update();
        QGuiApplication::processEvents();
    }

    [[nodiscard]] bool openglContextCreated() const {
        return _openglContextCreated;
    }

    [[nodiscard]] void *d3d11Device() const {
        return _d3d11Device;
    }

    void setCursor(QCursor &cursor) {
        view->setCursor(cursor);
    }

    void hideCursor() {
        view->setCursor(*transparentCursor);
    }

    void setTitle(const QString &title) {
        view->setTitle(title);
    }

    void setIcon(const QIcon &icon) {
        view->setIcon(icon);
    }

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

    void loadView(const QString &path, std::unique_ptr<QObjectCompletionSource> completionSource) {
        if (_loadViewCompletionSource) {
            _loadViewCompletionSource->cancel();
        }
        _loadViewCompletionSource = std::move(completionSource);

        view->setSource(QUrl(path));
    }

    void setBeforeRenderingCallback(std::function<void()> callback) {
        _beforeRenderingCallback = callback;
    }

private:
    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
    EGLConfig _eglConfig = EGL_NO_CONFIG_KHR;
    void *_d3d11Device = nullptr;
    bool _openglContextCreated = false;
    std::function<void()> _beforeRenderingCallback;

    std::unique_ptr<QObjectCompletionSource> _loadViewCompletionSource;

    void beforeRendering() {
        if (_beforeRenderingCallback) {
            _beforeRenderingCallback();
        }
    }
};
