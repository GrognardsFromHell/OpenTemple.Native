
#include <QSGNode>
#include <QSGSimpleMaterialShader>
#include <QSGImageNode>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include "GameView.h"
#include "GameViews.h"

GameView::GameView(QQuickItem *parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

QSGNode *GameView::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *data) {

    if (!_gameView) {
        auto context = QQmlEngine::contextForObject(this);
        if (!context) {
            return oldNode;
        }
        auto gameViews = context->contextProperty("gameViews").value<GameViews *>();
        if (!gameViews) {
            return oldNode;
        }

        _gameView = gameViews->create();
    }

    _gameView->setDimensions(width(), height());

    auto *node = static_cast<QSGImageNode *>(oldNode);
    if (!node) {
        node = window()->createImageNode();
        auto texture = _gameView->createTexture(window());
        node->setTexture(texture);        
        node->setSourceRect(0, 0, texture->textureSize().width(), texture->textureSize().height());
        node->setOwnsTexture(true);
    }

    node->setRect(0, 0, width(), height());

    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    return node;
}

GameView::~GameView() = default;
