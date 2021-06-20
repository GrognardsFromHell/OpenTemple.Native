
#pragma once

#include <atomic>

#define COM_NO_WINDOWS_H
#include <Unknwnbase.h>

/**
 * This is used as a custom drawing effect on text-runs to specify how they should be
 * rendered.
 * The default in Direct2D is that it only supports IBrush for this.
 */
class RunRenderSettings : public IUnknown {
 public:
  RunRenderSettings() {}

  HRESULT QueryInterface(const IID &riid, void **ppvObject) override;
  ULONG AddRef() override;
  ULONG Release() override;

 private:
  std::atomic<int> _refCount;
};
