
#ifndef _LIBMATH_H
#define _LIBMATH_H

#include <math.h>

#define USE_FDLIBM

#ifdef __STDC__
#define __P(p)  p
#else
#define __P(p)  ()
#endif

#if defined _WIN32 || defined SUNOS4 

#define fd_acos acos
#define fd_asin asin
#define fd_atan atan
#define fd_cos cos
#define fd_sin sin
#define fd_tan tan
#define fd_cosh cosh
#define fd_sinh sinh
#define fd_tanh tanh
#define fd_exp exp
#define fd_frexp frexp
#define fd_ldexp ldexp
#define fd_log log
#define fd_log10 log10
#define fd_modf modf
#define fd_sqrt sqrt
#define fd_ceil ceil
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod
#define fd_erf erf
#define fd_erfc erfc
#define fd_gamma gamma
#define fd_hypot hypot
#define fd_isnan isnan
#define fd_finite finite
#define fd_j0 j0
#define fd_j1 j1
#define fd_jn jn
#define fd_lgamma lgamma
#define fd_y0 y0
#define fd_y1 y1
#define fd_yn yn
#define fd_acosh acosh
#define fd_asinh asinh
#define fd_atanh atanh
#define fd_cbrt cbrt
#define fd_logb logb
#define fd_nextafter nextafter
#define fd_remainder remainder
#define fd_scalb scalb
#define fd_matherr matherr
extern double fd_atan2 __P((double, double));
extern double fd_pow __P((double, double));

#elif defined IRIX

#define fd_acos acos
#define fd_asin asin
#define fd_atan atan
#define fd_cosh cosh
#define fd_sinh sinh
#define fd_tanh tanh
#define fd_exp exp
#define fd_frexp frexp
#define fd_ldexp ldexp
#define fd_log log
#define fd_log10 log10
#define fd_modf modf
#define fd_sqrt sqrt
#define fd_fabs fabs
#define fd_fmod fmod
#define fd_erf erf
#define fd_erfc erfc
#define fd_gamma gamma
#define fd_hypot hypot
#define fd_isnan isnan
#define fd_finite finite
#define fd_j0 j0
#define fd_j1 j1
#define fd_jn jn
#define fd_lgamma lgamma
#define fd_y0 y0
#define fd_y1 y1
#define fd_yn yn
#define fd_acosh acosh
#define fd_asinh asinh
#define fd_atanh atanh
#define fd_cbrt cbrt
#define fd_logb logb
#define fd_nextafter nextafter
#define fd_remainder remainder
#define fd_scalb scalb
#define fd_matherr matherr
extern double fd_cos __P((double));
extern double fd_sin __P((double));
extern double fd_tan __P((double));
extern double fd_atan2 __P((double, double));
extern double fd_pow __P((double, double));
extern double fd_ceil __P((double));
extern double fd_floor __P((double));

#elif defined SOLARIS

#define fd_atan atan
#define fd_cos cos
#define fd_sin sin
#define fd_tan tan
#define fd_cosh cosh
#define fd_sinh sinh
#define fd_tanh tanh
#define fd_exp exp
#define fd_frexp frexp
#define fd_ldexp ldexp
#define fd_log10 log10
#define fd_modf modf
#define fd_sqrt sqrt
#define fd_ceil ceil
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod
#define fd_erf erf
#define fd_erfc erfc
#define fd_gamma gamma
#define fd_hypot hypot
#define fd_isnan isnan
#define fd_finite finite
#define fd_j0 j0
#define fd_j1 j1
#define fd_jn jn
#define fd_lgamma lgamma
#define fd_y0 y0
#define fd_y1 y1
#define fd_yn yn
#define fd_acosh acosh
#define fd_asinh asinh
#define fd_atanh atanh
#define fd_cbrt cbrt
#define fd_logb logb
#define fd_nextafter nextafter
#define fd_remainder remainder
#define fd_scalb scalb
#define fd_matherr matherr
extern double fd_acos __P((double));
extern double fd_asin __P((double));
extern double fd_log __P((double));
extern double fd_atan2 __P((double, double));
extern double fd_pow __P((double, double));

#elif defined HPUX

#define fd_cos cos
#define fd_sin sin
#define fd_cosh cosh
#define fd_sinh sinh
#define fd_tanh tanh
#define fd_exp exp
#define fd_frexp frexp
#define fd_ldexp ldexp
#define fd_log10 log10
#define fd_modf modf
#define fd_sqrt sqrt
#define fd_ceil ceil
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod
#define fd_erf erf
#define fd_erfc erfc
#define fd_gamma gamma
#define fd_hypot hypot
#define fd_isnan isnan
#define fd_finite finite
#define fd_j0 j0
#define fd_j1 j1
#define fd_jn jn
#define fd_lgamma lgamma
#define fd_y0 y0
#define fd_y1 y1
#define fd_yn yn
#define fd_acosh acosh
#define fd_asinh asinh
#define fd_atanh atanh
#define fd_cbrt cbrt
#define fd_logb logb
#define fd_nextafter nextafter
#define fd_remainder remainder
#define fd_scalb scalb
#define fd_matherr matherr
#define fd_acos acos
#define fd_log log
extern double fd_atan2 __P((double, double));
extern double fd_tan __P((double));
extern double fd_pow __P((double, double));
extern double fd_asin __P((double));
extern double fd_atan __P((double));

#else /* other platform.. generic paranoid slow fdlibm */

extern double fd_acos __P((double));
extern double fd_asin __P((double));
extern double fd_atan __P((double));
extern double fd_cos __P((double));
extern double fd_sin __P((double));
extern double fd_tan __P((double));
 
extern double fd_cosh __P((double));
extern double fd_sinh __P((double));
extern double fd_tanh __P((double));

extern double fd_exp __P((double));
extern double fd_frexp __P((double, int *));
extern double fd_ldexp __P((double, int));
extern double fd_log __P((double));
extern double fd_log10 __P((double));
extern double fd_modf __P((double, double *));
extern double fd_sqrt __P((double));

extern double fd_ceil __P((double));
extern double fd_fabs __P((double));
extern double fd_floor __P((double));
extern double fd_fmod __P((double, double));

extern double fd_erf __P((double));
extern double fd_erfc __P((double));
extern double fd_gamma __P((double));
extern double fd_hypot __P((double, double));
extern int fd_isnan __P((double));
extern int fd_finite __P((double));
extern double fd_j0 __P((double));
extern double fd_j1 __P((double));
extern double fd_jn __P((int, double));
extern double fd_lgamma __P((double));
extern double fd_y0 __P((double));
extern double fd_y1 __P((double));
extern double fd_yn __P((int, double));

extern double fd_acosh __P((double));
extern double fd_asinh __P((double));
extern double fd_atanh __P((double));
extern double fd_cbrt __P((double));
extern double fd_logb __P((double));
extern double fd_nextafter __P((double, double));
extern double fd_remainder __P((double, double));
#ifdef _SCALB_INT
extern double fd_scalb __P((double, int));
#else
extern double fd_scalb __P((double, double));
#endif

extern int fd_matherr __P((struct exception *));

extern double fd_atan2 __P((double, double));
extern double fd_pow __P((double, double));

#endif

extern double fd_copysign(double, double);

#endif /* _LIBMATH_H */

