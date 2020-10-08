
#pragma once

#include <QmlNet/qml/NetValue.h>
#include <QQuickItem>
#include "ManagedObject.h"

class GameView;

class GameViewItemTextureProvider;

class GameViewItem : public QQuickItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(GameView)
  Q_PROPERTY(NetValue * gameViewHandle READ gameViewHandle NOTIFY gameViewHandleChanged);
 public:
  explicit GameViewItem(QQuickItem *parent = nullptr);

  ~GameViewItem() override;

  bool isTextureProvider() const override;

  QSGTextureProvider *textureProvider() const override;

  NetValue *gameViewHandle() const;

 Q_SIGNALS:
  void gameViewHandleChanged(void*);

 protected:
  QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;

  void releaseResources() override;

  std::unique_ptr<GameViewItemTextureProvider> _textureProvider;

  std::unique_ptr<GameView> _gameView;
};
