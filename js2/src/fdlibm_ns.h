
#include <math.h>

#if defined(_WIN32) && !defined(__MWERKS__)
#define JS_USE_FDLIBM_MATH 1
#define __STDC__

#elif defined(linux)
#define JS_USE_FDLIBM_MATH 1

#else
#define JS_USE_FDLIBM_MATH 0
#endif

#if !JS_USE_FDLIBM_MATH

/*
 * Use system provided math routines.
 */

#define fd_acos acos
#define fd_asin asin
#define fd_atan atan
#define fd_atan2 atan2
#define fd_ceil ceil
#define fd_copysign copysign
#define fd_cos cos
#define fd_exp exp
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod
#define fd_log log
#define fd_pow pow
#define fd_sin sin
#define fd_sqrt sqrt
#define fd_tan tan

#else

/*
 * Use math routines in fdlibm.
 */

#undef __P
#ifdef __STDC__
#define __P(p)  p
#else
#define __P(p)  ()
#endif

#if defined _WIN32 || defined SUNOS4 

// these are functions we trust the local implementation
// to provide, so we just inline them into calls to the
// standard library.
namespace fd {
    inline double floor(double x)           { return ::floor(x); }
    inline double acos(double x)            { return ::acos(x); }
    inline double asin(double x)            { return ::asin(x); }
    inline double atan(double x)            { return ::atan(x); }
    inline double cos(double x)             { return ::cos(x); }
    inline double sin(double x)             { return ::sin(x); }
    inline double tan(double x)             { return ::tan(x); }
    inline double exp(double x)             { return ::exp(x); }
    inline double log(double x)             { return ::log(x); }
    inline double sqrt(double x)            { return ::sqrt(x); }
    inline double ceil(double x)            { return ::ceil(x); }
    inline double fabs(double x)            { return ::fabs(x); }
    inline double fmod(double x, double y)  { return ::fmod(x, y); }
}

// these one we get from the fdlibm library
namespace fd {
    extern "C" {
        double fd_atan2 __P((double, double));
        double fd_copysign __P((double, double));
        double fd_pow __P((double, double));
    }
    inline double atan2(double x, double y)      { return fd_atan2(x, y); }
    inline double copysign(double x, double y)   { return fd_copysign(x, y); }
    inline double pow(double x, double y)        { return fd_pow(x, y); }
}


#elif defined(linux)

namespace fd {
    inline double atan(double x)               { return ::atan(x); }
    inline double atan2(double x, double y)    { return ::atan2(x, y); }
    inline double ceil(double x)               { return ::ceil(x); }
    inline double cos(double x)                { return ::cos(x); }
    inline double fabs(double x)               { return ::fabs(x); }
    inline double floor(double x)              { return ::floor(x); }
    inline double fmod(double x, double y)     { return ::fmod(x, y); }
    inline double sin(double x)                { return ::sin(x); }
    inline double sqrt(double x)               { return ::sqrt(x); }
    inline double tan(double x)                { return ::tan(x); }
    inline double copysign(double x, double y) { return ::copysign(x, y); }
}

namespace fd {
    extern "C" {
        double fd_asin __P((double));
        double fd_acos __P((double));
        double fd_exp __P((double));
        double fd_log __P((double));
        double fd_pow __P((double, double));
    }
    inline double asin(double x)                 { return fd_asin(x); }
    inline double acos(double x)                 { return fd_acos(x); }
    inline double exp(double x)                  { return fd_exp(x); }
    inline double log(double x)                  { return fd_log(x); }
    inline double pow(double x, double y)        { return fd_pow(x, y); }
}

#endif


#endif

