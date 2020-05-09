
#include "gl_headers.h"

#include <QQuickView>
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

#include "GameViews.h"
#include "utils.h"
#include "GameViewItem.h"
#include "ui.h"

std::unique_ptr<GameView> GameViews::create() {
    auto handle = create_game_view(_handle);
    return std::make_unique<GameView>(handle);
}

GameViews::GameViews(GameViewsHandle handle, QObject *parent) : QObject(parent), _handle(handle) {}

GameViews::~GameViews() {
    destroy_game_views(_handle);
}

struct GameViewsCallbacks {
    NativeDelegate<void(GameViewsHandle)> destroy_game_views;
    NativeDelegate<GameViewHandle(GameViewsHandle)> create_game_view;
    NativeDelegate<void(GameViewHandle)> destroy_game_view;
    NativeDelegate<bool(GameViewHandle, ID3D11Texture2D **texture, int *width, int *height)> get_texture;
    NativeDelegate<void(GameViewHandle, int width, int height)> set_size;

    void install() {
        GameViews::destroy_game_views = destroy_game_views;
        GameViews::create_game_view = create_game_view;

        GameView::destroy_game_view = destroy_game_view;
        GameView::get_texture = get_texture;
        GameView::set_size = set_size;
    }
};

NATIVE_API void gameviews_set_callbacks(GameViewsCallbacks callbacks) {
    callbacks.install();
}

static const QString gameViewsObjectName{"gameViews"};

NATIVE_API bool gameviews_install(Ui &ui, GameViewsHandle handle) {
    if (ui.window()->findChild<GameViews *>(gameViewsObjectName)) {
        return false;
    }

    auto views = new GameViews(handle, ui.window());
    views->setObjectName(gameViewsObjectName);
    return true;
}

NATIVE_API bool gameviews_uninstall(Ui &ui) {
    auto views = ui.window()->findChild<GameViews *>(gameViewsObjectName);
    if (views) {
        delete views;
        return true;
    }
    return false;
}

GameView::~GameView() {
    if (_textureId) {
        GLuint tid = _textureId.value();
        glDeleteTextures(1, &tid);
    }

    destroy_game_view(_handle);
}

QSGTexture *GameView::createTexture(QQuickWindow *window) {
    ID3D11Texture2D *texture;
    int width, height;
    if (!get_texture(_handle, &texture, &width, &height)) {
        return nullptr;
    }

    GLuint tid;
    if (_textureId) {
        tid = *_textureId;
    } else {
        glGenTextures(1, &tid);
        _textureId = tid;
    }
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
    set_size(_handle, _size.width(), _size.height());
}

void GameView::setUpdateCallback(const std::function<void()> &updateCallback) {
    _updateCallback = updateCallback;
}

const std::function<void()> &GameView::getUpdateCallback() const {
    return _updateCallback;
}

std::function<void(GameViewsHandle)> GameViews::destroy_game_views;
std::function<GameViewHandle(GameViewsHandle)> GameViews::create_game_view;
std::function<void(GameViewHandle)> GameView::destroy_game_view;
std::function<bool(GameViewHandle, ID3D11Texture2D **texture, int *width, int *height)> GameView::get_texture;
std::function<void(GameViewHandle, int width, int height)> GameView::set_size;
