
#pragma once

class QObject;

void *wrap_qobject(QObject *qobject, bool ownsObject = false);
