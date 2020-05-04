
#pragma once

class QString;
class QByteArray;

char16_t *copyString(const QString&);
char *copyString(const QByteArray&);
char *copyString(const char*);
wchar_t *copyString(const wchar_t*);
