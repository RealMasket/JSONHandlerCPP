#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef JSONHANDLERC_EXPORTS
#define JSON_API __declspec(dllexport)
#else
#define JSON_API __declspec(dllimport)
#endif
#else
#define JSON_API
#endif