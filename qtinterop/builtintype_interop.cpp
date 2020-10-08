
#include <QByteArray>
#include <QColor>
#include <QPoint>
#include <QQmlListProperty>
#include <QRect>
#include <QSize>
#include <QString>
#include <QUrl>

#include "../game/utils.h"
#include "string_interop.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// QQmlListProperty
/////////////////////////////////////////////////////////////////////////////////////////////

NATIVE_API int QQmlListProperty_size() { return sizeof(QQmlListProperty<QObject>); }

NATIVE_API void QmlListInterop_append(QQmlListProperty<QObject> *list, QObject *value) {
  return list->append(list, value);
}

NATIVE_API int QmlListInterop_count(QQmlListProperty<QObject> *list) { return list->count(list); }

NATIVE_API void *QmlListInterop_at(QQmlListProperty<QObject> *list, int index) {
  return list->at(list, index);
}

NATIVE_API void QmlListInterop_clear(QQmlListProperty<QObject> *list) { list->clear(list); }

NATIVE_API void QmlListInterop_replace(QQmlListProperty<QObject> *list, int index, QObject *value) {
  return list->replace(list, index, value);
}

NATIVE_API void QmlListInterop_removeLast(QQmlListProperty<QObject> *list) {
  return list->removeLast(list);
}

NATIVE_API int QmlListInterop_indexOf(QQmlListProperty<QObject> *list, QObject *value) {
  if (!list->count || !list->at) {
    return -1;
  }
  auto count = list->count(list);
  for (auto i = 0; i < count; i++) {
    if (list->at(list, i) == value) {
      return i;
    }
  }
  return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// QString
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(!std::is_trivially_default_constructible<QString>());
static_assert(!std::is_trivially_destructible<QString>());

NATIVE_API int QString_size() { return sizeof(QString); }

NATIVE_API char16_t *QString_read(const QString *str) { return copyString(*str); }

NATIVE_API QString *QString_create_null() { return new QString(); }

NATIVE_API QString *QString_create(const char16_t *charsPtr, int charCount) {
  if (!charsPtr) {
    return new QString();
  }
  return new QString(reinterpret_cast<const QChar *>(charsPtr), charCount);
}

NATIVE_API void QString_delete(QString *ptr) { delete ptr; }

/////////////////////////////////////////////////////////////////////////////////////////////
// QByteArray
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(!std::is_trivially_default_constructible<QByteArray>());
static_assert(!std::is_trivially_destructible<QByteArray>());

NATIVE_API int QByteArray_size() { return sizeof(QByteArray); }

NATIVE_API void QByteArray_ctor_default(QByteArray *byteArray) {
  new (byteArray) QByteArray();
}

NATIVE_API void QByteArray_ctor(QByteArray *byteArray, const char *data, int size) {
  new (byteArray) QByteArray(data, size);
}

NATIVE_API void QByteArray_read(const QByteArray *byteArray, const void **dataOut, int *sizeOut) {
  *dataOut = byteArray->constData();
  *sizeOut = byteArray->size();
}

NATIVE_API void QByteArray_dtor(QByteArray *byteArray) { byteArray->~QByteArray(); }

/////////////////////////////////////////////////////////////////////////////////////////////
// QPoint
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(std::is_trivially_destructible<QPoint>());
static_assert(std::is_trivially_copyable<QPoint>());

NATIVE_API int QPoint_size() { return sizeof(QPoint); }

NATIVE_API void QPoint_ctor(QPoint *point, int x, int y) { new (point) QPoint(x, y); }

NATIVE_API void QPoint_ctor_default(QPoint *point) { new (point) QPoint(); }

NATIVE_API void QPoint_read(QPoint *point, int *x, int *y) {
  *x = point->x();
  *y = point->y();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// QPointF
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(std::is_trivially_destructible<QPointF>());
static_assert(std::is_trivially_copyable<QPointF>());

NATIVE_API int QPointF_size() { return sizeof(QPointF); }

NATIVE_API void QPointF_ctor(QPointF *point, double x, double y) { new (point) QPointF(x, y); }

NATIVE_API void QPointF_ctor_default(QPointF *point) { new (point) QPointF(); }

NATIVE_API void QPointF_read(QPointF *point, double *x, double *y) {
  *x = point->x();
  *y = point->y();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// QSize
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(std::is_trivially_destructible<QSize>());
static_assert(std::is_trivially_copyable<QSize>());

NATIVE_API int QSize_size() { return sizeof(QSize); }

NATIVE_API void QSize_ctor(QSize *size, int width, int height) { new (size) QSize(width, height); }

NATIVE_API void QSize_ctor_default(QSize *size) { new (size) QSize(); }

NATIVE_API void QSize_read(QSize *size, int *width, int *height) {
  *width = size->width();
  *height = size->height();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// QSizeF
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(std::is_trivially_destructible<QSizeF>());
static_assert(std::is_trivially_copyable<QSizeF>());

NATIVE_API int QSizeF_size() { return sizeof(QSizeF); }

NATIVE_API void QSizeF_ctor(QSizeF *size, double width, double height) {
  *size = QSizeF(width, height);
}

NATIVE_API void QSizeF_ctor_default(QSizeF *size) { new (size) QSizeF(); }

NATIVE_API void QSizeF_read(QSizeF *size, double *width, double *height) {
  *width = size->width();
  *height = size->height();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// QRect
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(std::is_trivially_destructible<QRect>());
static_assert(std::is_trivially_copyable<QRect>());

NATIVE_API int QRect_size() { return sizeof(QRect); }

NATIVE_API void QRect_ctor(QRect *rect, int x, int y, int width, int height) {
  new (rect) QRect(x, y, width, height);
}

NATIVE_API void QRect_ctor_default(QRect *rect) { new (rect) QRect(); }

NATIVE_API void QRect_read(QRect *rect, int *x, int *y, int *width, int *height) {
  *x = rect->x();
  *y = rect->y();
  *width = rect->width();
  *height = rect->height();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// QRectF
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(std::is_trivially_destructible<QRectF>());
static_assert(std::is_trivially_copyable<QRectF>());

NATIVE_API int QRectF_size() { return sizeof(QRectF); }

NATIVE_API void QRectF_ctor(QRectF *rect, double x, double y, double width, double height) {
  new (rect) QRectF(x, y, width, height);
}

NATIVE_API void QRectF_ctor_default(QRectF *rect) { new (rect) QRectF(); }

NATIVE_API void QRectF_read(QRectF *rect, double *x, double *y, double *width, double *height) {
  *x = rect->x();
  *y = rect->y();
  *width = rect->width();
  *height = rect->height();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// QColor
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(std::is_trivially_destructible<QColor>());
static_assert(!std::is_trivially_copyable<QColor>());

NATIVE_API int QColor_size() { return sizeof(QColor); }

NATIVE_API void QColor_ctor(QColor *color, int argb) { new (color) QColor((QRgb)argb); }

NATIVE_API void QColor_ctor_default(QColor *color) { new (color) QColor(); }

NATIVE_API void QColor_read(QColor *color, int *argb) { *argb = color->rgba(); }

/////////////////////////////////////////////////////////////////////////////////////////////
// QUrl
/////////////////////////////////////////////////////////////////////////////////////////////

static_assert(!std::is_trivially_destructible<QUrl>());
static_assert(!std::is_trivially_copyable<QUrl>());

NATIVE_API int QUrl_size() { return sizeof(QUrl); }

NATIVE_API void QUrl_ctor(QUrl *url, const char16_t *str, int strLen) {
  new (url) QUrl(QString::fromRawData(reinterpret_cast<const QChar *>(str), strLen));
}

NATIVE_API void QUrl_ctor_default(QUrl *url) { new (url) QUrl(); }

NATIVE_API void QUrl_dtor(QUrl *memory) { memory->~QUrl(); }

NATIVE_API char16_t *QUrl_read(const QUrl *url) { return copyString(url->toString()); }
