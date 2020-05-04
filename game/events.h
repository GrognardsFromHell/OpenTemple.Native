
#pragma once

#include "utils.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

/**
 * Interop type to transport QMouseEvent fields to C#.
 */
struct MouseEvent {
    QEvent::Type type;
    Qt::KeyboardModifiers modifiers;
    Qt::MouseButton button;
    Qt::MouseButtons buttons;
    Qt::MouseEventFlags flags;
    int globalX;
    int globalY;
    float x;
    float y;
    float screenX;
    float screenY;
    Qt::MouseEventSource source;
    float windowX;
    float windowY;

    explicit MouseEvent(const QMouseEvent &e) {
        type = e.type();
        modifiers = e.modifiers();
        button = e.button();
        buttons = e.buttons();
        flags = e.flags();
        globalX = e.globalX();
        globalY = e.globalY();
        x = e.localPos().x();
        y = e.localPos().x();
        screenX = e.screenPos().x();
        screenY = e.screenPos().y();
        source = e.source();
        windowX = e.windowPos().x();
        windowY = e.windowPos().y();
    }
};

/**
 * Interop type to transport QWheelEvent fields to C#.
 */
struct WheelEvent {
    QEvent::Type type;
    Qt::KeyboardModifiers modifiers;
    int angleDeltaX;
    int angleDeltaY;
    Qt::MouseButtons buttons;
    float globalX;
    float globalY;
    bool inverted;
    Qt::ScrollPhase phase;
    int pixelDeltaX;
    int pixelDeltaY;
    float x;
    float y;
    Qt::MouseEventSource source;

    explicit WheelEvent(const QWheelEvent &e) {
        type = e.type();
        modifiers = e.modifiers();
        angleDeltaX = e.angleDelta().x();
        angleDeltaY = e.angleDelta().y();
        buttons = e.buttons();
        globalX = e.globalX();
        globalY = e.globalY();
        inverted = e.inverted();
        pixelDeltaX = e.pixelDelta().x();
        pixelDeltaY = e.pixelDelta().y();
        x = e.position().x();
        y = e.position().y();
        source = e.source();
    }
};

/**
 * Interop type to transport QKeyEvent fields to C#.
 */
struct KeyEvent {
    QEvent::Type type;
    Qt::KeyboardModifiers modifiers;
    int count;
    bool isAutoRepeat;
    int key;
    uint32_t nativeModifiers;
    uint32_t nativeScanCode;
    uint32_t nativeVirtualKey;
    const char16_t *text;

    explicit KeyEvent(const QKeyEvent &e) {
        type = e.type();
        modifiers = e.modifiers();
        count = e.count();
        isAutoRepeat = e.isAutoRepeat();
        key = e.key();
        nativeModifiers = e.nativeModifiers();
        nativeScanCode = e.nativeScanCode();
        nativeVirtualKey = e.nativeVirtualKey();
        text = reinterpret_cast<const char16_t *>(e.text().utf16());
    }
};
