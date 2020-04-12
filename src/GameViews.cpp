
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#include <QQuickView>
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

#include "GameViews.h"

std::unique_ptr<GameView> GameViews::create() {
    return std::make_unique<GameView>(_functions, _functions.create());
}

GameViews::GameViews(const GameViewFunctions &functions) : QObject(nullptr),
                                                           _functions(functions) {}

GameView::GameView(const GameViewFunctions &functions, void *handle) :
        _functions(functions), _handle(handle) {}

GameView::~GameView() {
    _functions.destroy(_handle);
}

QSGTexture *GameView::createTexture(QQuickWindow *window) {
    void *texture;
    int width, height;
    if (!_functions.get_texture(_handle, &texture, &width, &height)) {
        return nullptr;
    }

    GLuint tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);

    auto glContext = window->openglContext();
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    EGLDisplay eglDisplay = (EGLDisplay)
            native->nativeResourceForContext("egldisplay", glContext);
    EGLDisplay eglConfig = (EGLConfig)
            native->nativeResourceForContext("eglconfig", glContext);

    EGLint attribs[] = {
        EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
        EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,   
        EGL_NONE,
    };
    auto surface = eglCreatePbufferFromClientBuffer(eglDisplay,
                                                    EGL_D3D_TEXTURE_ANGLE,
                                                    (EGLClientBuffer) texture,
                                                    eglConfig,
                                                    attribs);
    if (surface == EGL_NO_SURFACE) {
          qDebug("eglCreatePbufferFromClientBuffer failed");
    }

    if (!eglBindTexImage(eglDisplay, surface, EGL_BACK_BUFFER)) {
      qDebug("eglBindTexImage failed");
      return nullptr;
    }

    return window->createTextureFromNativeObject(
            QQuickWindow::NativeObjectTexture,
            &tid,
            0,
            QSize(width, height)
    );
}

void GameView::setSize(QSize size) {
    _size = size;
    _functions.set_size(_handle, _size.width(), _size.height());
}
