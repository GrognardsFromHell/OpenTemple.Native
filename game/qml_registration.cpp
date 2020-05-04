

#include <BinkVideoItem.h>
#include <QtQml/QQmlModuleRegistration>

#include "GameViewItem.h"

static void qml_opentemple_register_types() {


  qmlRegisterTypesAndRevisions<GameViewItem>("OpenTemple", 1);
  qmlRegisterTypesAndRevisions<BinkVideoItem>("OpenTemple", 1);

  qmlRegisterModule("OpenTemple", 1, 0);
}

[[maybe_unused]] static const QQmlModuleRegistration registration("OpenTemple", 1,
                                                                  qml_opentemple_register_types);
