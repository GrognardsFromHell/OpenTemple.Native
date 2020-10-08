

#include <BinkVideoItem.h>
#include <QtQml/QQmlModuleRegistration>
#include <QmlNet/qml/JsNetObject.h>

#include "GameViewItem.h"
#include "GameViews.h"

static void qml_opentemple_register_types() {
  qmlRegisterTypesAndRevisions<GameViewItem>("OpenTemple", 1);
  qmlRegisterTypesAndRevisions<BinkVideoItem>("OpenTemple", 1);
  qmlRegisterSingletonType<JsNetObject>(
      "OpenTemple", 1, 0, "Net", [](auto engine, auto jsEngine) { return new JsNetObject; });
  qmlRegisterModule("OpenTemple", 1, 0);
}

[[maybe_unused]] static const QQmlModuleRegistration registration("OpenTemple", 1,
                                                                  qml_opentemple_register_types);
