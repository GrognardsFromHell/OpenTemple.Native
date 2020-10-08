
#pragma once

#include <QMetaType>
#include <QtGlobal>

struct ManagedObjectRef {
  ManagedObjectRef() : _handle(nullptr) {}
  explicit ManagedObjectRef(void *handle) noexcept : _handle(handle) {}

 private:
  const void *_handle;
};

class ManagedObject {
 public:
  ~ManagedObject();
  ManagedObject(ManagedObject &&o) noexcept;
  ManagedObject &operator=(ManagedObject &&o) noexcept;

  static void setFreeHandle(void (*)(void *));

  [[nodiscard]] operator ManagedObjectRef() const noexcept {
    return ManagedObjectRef(_handle);
  }

  // Cannot be constructed from the native side
  ManagedObject() = delete;

 private:
  static void (*_freeHandle)(void *);

  void *_handle;

  Q_DISABLE_COPY(ManagedObject);
};

// Must be usable in place of a void*
static_assert(sizeof(ManagedObjectRef) == sizeof(void*));
static_assert(sizeof(ManagedObject) == sizeof(void*));

Q_DECLARE_METATYPE(ManagedObjectRef);
