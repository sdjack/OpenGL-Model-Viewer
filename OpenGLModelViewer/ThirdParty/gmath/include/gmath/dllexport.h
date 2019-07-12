
#ifndef GPH_MATH_API_H
#define GPH_MATH_API_H

#ifdef GPH_MATH_STATIC
#  define GPH_MATH_API
#  define GPH_MATH_NO_EXPORT
#else
#  ifndef GPH_MATH_API
#    ifdef gmath_EXPORTS
        /* We are building this library */
#      define GPH_MATH_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define GPH_MATH_API __declspec(dllimport)
#    endif
#  endif

#  ifndef GPH_MATH_NO_EXPORT
#    define GPH_MATH_NO_EXPORT 
#  endif
#endif

#ifndef GPH_MATH_DEPRECATED
#  define GPH_MATH_DEPRECATED __declspec(deprecated)
#endif

#ifndef GPH_MATH_DEPRECATED_EXPORT
#  define GPH_MATH_DEPRECATED_EXPORT GPH_MATH_API GPH_MATH_DEPRECATED
#endif

#ifndef GPH_MATH_DEPRECATED_NO_EXPORT
#  define GPH_MATH_DEPRECATED_NO_EXPORT GPH_MATH_NO_EXPORT GPH_MATH_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef GPH_MATH_NO_DEPRECATED
#    define GPH_MATH_NO_DEPRECATED
#  endif
#endif

#endif /* GPH_MATH_API_H */
