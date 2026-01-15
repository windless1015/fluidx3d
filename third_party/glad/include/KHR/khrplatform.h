#pragma once

#if defined(_WIN32)
#define KHRONOS_APIENTRY __stdcall
#define KHRONOS_APICALL __declspec(dllimport)
#define KHRONOS_APIATTRIBUTES
#else
#define KHRONOS_APIENTRY
#define KHRONOS_APICALL
#define KHRONOS_APIATTRIBUTES
#endif
