
#pragma once

#include "ManagedObject.h"
#include "../game/utils.h"

ManagedObject::ManagedObject(ManagedObject&& o) noexcept {
  _handle = o._handle;
  o._handle = nullptr;
}

ManagedObject& ManagedObject::operator=(ManagedObject&& o) noexcept {
  if (_handle) {
    _freeHandle(_handle);
  }
  _handle = o._handle;
  o._handle = nullptr;
  return *this;
}

ManagedObject::~ManagedObject() {
  Q_ASSERT(_freeHandle);
  if (_handle) {
    _freeHandle(_handle);
  }
}

void (*ManagedObject::_freeHandle)(void*) = nullptr;

void ManagedObject::setFreeHandle(void (*freeHandle)(void*)) { _freeHandle = freeHandle; }

NATIVE_API void ManagedObject_setFreeHandle(void (*freeHandle)(void*)) {
  ManagedObject::setFreeHandle(freeHandle);
}
