
extern "C" {
#include "fdlibm.h"
};

double fmod(double x, double y) { return __ieee754_fmod(x, y); }
double floor(double x) { return fd_floor(x); }
double fabs(double x) { return fd_fabs(x); }
