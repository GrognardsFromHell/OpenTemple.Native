#pragma once

// Ensure that GL functions are not dllimported because we use
// a statically linked ANGLE
#define EGLAPI
#define EGLAPIENTRY
#define GL_APICALL
#define GL_APIENTRY

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
