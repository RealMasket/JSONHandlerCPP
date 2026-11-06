#pragma once

#if defined(MASKETJSON_STATIC)
	#define MASKETJSON_API
#elif defined(_WIN32) || defined(__CYGWIN__)
	#ifdef MASKETJSON_EXPORTS
		#define MASKETJSON_API __declspec(dllexport)
	#else
		#define MASKETJSON_API __declspec(dllimport)
	#endif
#else
	#define MASKETJSON_API
#endif
