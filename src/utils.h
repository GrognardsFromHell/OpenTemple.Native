
#pragma once

#ifdef _MSC_VER
    #define NATIVE_API extern "C" __declspec(dllexport)
#else
    #define NATIVE_API extern "C" __attribute__ ((visibility ("default")))
#endif

typedef int ApiBool;
