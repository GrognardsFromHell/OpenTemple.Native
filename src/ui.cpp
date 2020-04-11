
#include <QtPlugin>
#include <QThread>
#include <QDir>

Q_IMPORT_PLUGIN(QtQuick2Plugin)

Q_IMPORT_PLUGIN(QtQmlPlugin)

Q_IMPORT_PLUGIN(QtQmlModelsPlugin)

Q_IMPORT_PLUGIN(QtQmlWorkerScriptPlugin)

Q_IMPORT_PLUGIN(QtGraphicalEffectsPlugin)

Q_IMPORT_PLUGIN(QtQuick2WindowPlugin)

Q_IMPORT_PLUGIN(QtGraphicalEffectsPrivatePlugin)

#include "utils.h"
#include "ui.h"
#include "completion_source.h"

NATIVE_API Ui *ui_create(GameViewFunctions *functions) {
    auto ui = new Ui(functions);

    // Keep processing until the OpenGL context has been created
    while (!ui->openglContextCreated()) {
        QCoreApplication::processEvents();
        QThread::yieldCurrentThread();
    }

    return ui;
}

NATIVE_API void ui_destroy(Ui *ui) {
    delete ui;
}

NATIVE_API void ui_set_config(Ui &ui, const NativeWindowConfig &config) {
    ui.setConfig(config);
}

NATIVE_API void *ui_getd3d11device(Ui &ui) {
    return ui.d3d11Device();
}

NATIVE_API bool ui_render(Ui &ui) {
    ui.render();
    return ui.view->isVisible();
}

NATIVE_API void ui_hide_cursor(Ui &ui) {
    ui.hideCursor();
}

NATIVE_API void ui_set_cursor(Ui &ui, QCursor &cursor) {
    ui.setCursor(cursor);
}

NATIVE_API void ui_set_icon(Ui &ui, const char16_t *icon_path) {
    ui.setIcon(QIcon(QString::fromUtf16(icon_path)));
}

NATIVE_API void ui_set_title(Ui &ui, const char16_t *title) {
    ui.setTitle(QString::fromUtf16(title));
}

NATIVE_API void ui_add_search_path(const char16_t *prefix, const char16_t *path) {
    QDir::addSearchPath(QString::fromUtf16(prefix), QString::fromUtf16(path));
}

template<typename T>
using AsyncSuccessCallback = void(T result);
using AsyncErrorCallback = void(const char16_t *);

NATIVE_API void
ui_load_view(Ui &ui, const char16_t *path, QObjectCompletionSource *completionSource) {
    ui.loadView(
            QString::fromUtf16(path),
            std::unique_ptr<QObjectCompletionSource>(completionSource)
    );
}

using BeforeRenderingCallback = void();
NATIVE_API void ui_set_before_rendering_callback(Ui &ui, BeforeRenderingCallback *callback) {
    ui.setBeforeRenderingCallback(callback);
}
