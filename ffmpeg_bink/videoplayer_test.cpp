
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickWindow>

#include <soloud/soloud.h>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  QByteArray localMsg = msg.toLocal8Bit();
  const char *file = context.file ? context.file : "";
  const char *function = context.function ? context.function : "";
  switch (type) {
    case QtDebugMsg:
      fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line,
              function);
      break;
    case QtInfoMsg:
      fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
      break;
    case QtWarningMsg:
      fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line,
              function);
      break;
    case QtCriticalMsg:
      fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line,
              function);
      break;
    case QtFatalMsg:
      fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line,
              function);
      break;
  }
}

#include "BinkVideoItem.h"

int main(int argc, char **argv) {
  qInstallMessageHandler(myMessageOutput);

  QGuiApplication app(argc, argv);

  SoLoud::Soloud soloud;
  soloud.init();

  QQuickWindow window;
  auto videoItem = new BinkVideoItem(window.contentItem());
  QObject::connect(videoItem, &BinkVideoItem::ended, []() { qDebug() << "ENDED!"; });
  videoItem->setSoloud(&soloud);
  videoItem->open("D:/ffmpeg/bin/AtariLogo.bik");
  window.resize(videoItem->width(), videoItem->height());
  QObject::connect(window.contentItem(), &QQuickItem::widthChanged,
                   [&]() { videoItem->setWidth(window.contentItem()->width()); });
  QObject::connect(window.contentItem(), &QQuickItem::heightChanged,
                   [&]() { videoItem->setHeight(window.contentItem()->height()); });

  window.show();

  return QGuiApplication::exec();
}
