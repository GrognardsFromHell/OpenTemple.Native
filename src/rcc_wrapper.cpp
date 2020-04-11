
#include <QtCore/qfile.h>
#include "utils.h"
#include "rcc.h"

NATIVE_API RCCResourceLibrary *rcc_library_create() {
    auto library = new RCCResourceLibrary(3);
    library->setFormat(RCCResourceLibrary::Binary);
    return library;
}

NATIVE_API void rcc_library_add(RCCResourceLibrary &library, const char16_t *filename) {
    auto inputFiles = library.inputFiles();
    inputFiles.append(QString::fromUtf16(filename));
    library.setInputFiles(inputFiles);
}

NATIVE_API bool rcc_library_write(RCCResourceLibrary &library, const char16_t *filename) {
    QFile file(QString::fromUtf16(filename));
    if (!file.open(QFile::WriteOnly)) {
        return false;
    }

    QFile tmpFile, errorFile;
    return library.output(file, tmpFile, errorFile);
}

NATIVE_API void rcc_library_destroy(RCCResourceLibrary *library) {
    delete library;
}
