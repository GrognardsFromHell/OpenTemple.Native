
#include <QSGNode>
#include <QSGSimpleMaterialShader>
#include <QSGImageNode>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGTextureProvider>

#include "GameViewItem.h"
#include "GameViews.h"

class GameViewItemTextureProvider : public QSGTextureProvider {
Q_OBJECT
public:
    void setTexture(QSGTexture *texture) {
        _texture = texture;
    }

    QSGTexture *texture() const override {
        return _texture;
    }

    void fireTextureChanged() {
        emit textureChanged();
    }

private:
    QSGTexture *_texture = nullptr;
};

GameViewItem::GameViewItem(QQuickItem *parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
    _textureProvider = std::make_unique<GameViewItemTextureProvider>();
}

QSGNode *GameViewItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *data) {

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
        _gameView->setUpdateCallback([this] {
            update();
            // We assume that the texture changes every frame
            _textureProvider->fireTextureChanged();
        });
    }

    QSize desiredSize((int) width(), (int) height());
    _gameView->setSize(desiredSize);

    auto *node = static_cast<QSGImageNode *>(oldNode);
    if (!node) {
        auto texture = _gameView->createTexture(window());
        _textureProvider->setTexture(texture);
        if (!texture) {
            return nullptr;
        }
        node = window()->createImageNode();
        node->setTexture(texture);
        node->setSourceRect(0, 0, texture->textureSize().width(), texture->textureSize().height());
        node->setOwnsTexture(true);
    } else {
        if (node->texture()->textureSize() != desiredSize) {
            // Update size and texture
            auto texture = _gameView->createTexture(window());
            _textureProvider->setTexture(texture);
            if (!texture) {
                delete node;
                return nullptr;
            }
            node->setTexture(texture);
            node->setSourceRect(0, 0, texture->textureSize().width(), texture->textureSize().height());
        }
    }

    node->setRect(0, 0, width(), height());

    return node;
}

bool GameViewItem::isTextureProvider() const {
    return true;
}

QSGTextureProvider *GameViewItem::textureProvider() const {
    return _textureProvider.get();
}

GameViewItem::~GameViewItem() = default;

#include "GameViewItem.moc"
