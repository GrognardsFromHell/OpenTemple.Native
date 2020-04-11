
#pragma once

#include <QQuickItem>

class RegisteredGameView;

class GameView : public QQuickItem
{
    Q_OBJECT
public:
    explicit GameView(QQuickItem *parent = 0);
    ~GameView();

protected:
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;

    std::unique_ptr<RegisteredGameView> _gameView;
};
