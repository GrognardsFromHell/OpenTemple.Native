//
// Created by Sebastian on 24.05.2021.
//

#include "RunRenderSettings.h"

HRESULT RunRenderSettings::QueryInterface(const IID &riid, void **ppvObject) {
  return 0;
}

ULONG RunRenderSettings::AddRef() {
  return ++_refCount;
}

ULONG RunRenderSettings::Release() {
  return 0;
}
