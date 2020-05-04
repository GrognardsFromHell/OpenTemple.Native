
#include <QImage>
#include <QPixmap>
#include <QCursor>

#include "utils.h"

NATIVE_API QCursor *cursor_create(uint8_t *pixelData, int width, int height, int hotspotX, int hotspotY) {
    QImage image(pixelData, width, height, QImage::Format_ARGB32);
    QPixmap pixmap = QPixmap::fromImage(image);
    return new QCursor(pixmap, hotspotX, hotspotY);
}

NATIVE_API void cursor_delete(QCursor *cursor) {
    delete cursor;
}
