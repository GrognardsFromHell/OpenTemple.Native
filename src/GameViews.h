
#pragma once

#include <QtANGLE/GLES2/gl2.h>
#include <QObject>

struct GameViewFunctions {
    void *(*create)();

    void (*destroy)(void *handle);

    void (*set_size)(void *handle, int width, int height);

    bool (*get_texture)(void *handle, void **texture_ptr, int *width, int *height);
};

class QSGTexture;

class QQuickWindow;

class GameView {
public:
    GameView(const GameViewFunctions &functions, void *handle);

    ~GameView();

    QSGTexture *createTexture(QQuickWindow *);

    QSize size() const {
        return _size;
    }
    void setSize(QSize size);
private:
    const GameViewFunctions &_functions;
    void *_handle;
    QSize _size;
};

class GameViews : public QObject {
Q_OBJECT
public:
    GameViews(const GameViewFunctions &functions);

    std::unique_ptr<GameView> create();

private:
    const GameViewFunctions _functions;
};
