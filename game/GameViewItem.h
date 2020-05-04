
#pragma once

#include <QQuickItem>

class GameView;

class GameViewItemTextureProvider;

class GameViewItem : public QQuickItem {
Q_OBJECT
QML_ELEMENT
public:
    explicit GameViewItem(QQuickItem *parent = nullptr);

    ~GameViewItem() override;

    bool isTextureProvider() const override;

    QSGTextureProvider *textureProvider() const override;

protected:
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;

    std::unique_ptr<GameViewItemTextureProvider> _textureProvider;

    std::unique_ptr<GameView> _gameView;
};
