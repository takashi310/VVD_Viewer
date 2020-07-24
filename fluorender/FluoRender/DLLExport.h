#ifndef _DLLEXPORT_H
#define _DLLEXPORT_H

#if defined(WIN32)
#ifdef VVD_DLL_EXPORTS
#define EXPORT_API __declspec(dllexport)
#define EXTERN_EXPORT_API extern "C" __declspec(dllexport)
#else
#define EXPORT_API __declspec(dllimport)
#define EXTERN_EXPORT_API
#endif
#else
#define EXPORT_API
#define EXTERN_EXPORT_API extern "C"
#endif

#endif // _DECLARATIONS_H
