
#pragma once

#include <QQuickItem>

class GameView;

class GameViewItemTextureProvider;

class GameViewItem : public QQuickItem {
Q_OBJECT
public:
    explicit GameViewItem(QQuickItem *parent = nullptr);

    ~GameViewItem();

    bool isTextureProvider() const override;

    QSGTextureProvider *textureProvider() const override;

protected:
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;

    std::unique_ptr<GameViewItemTextureProvider> _textureProvider;

    std::unique_ptr<GameView> _gameView;
};
