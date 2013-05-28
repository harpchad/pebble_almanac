/*
 * most functions from Michael Ehrmann
 * loosely based on
 * - http://stackoverflow.com/questions/11261170/c-and-maths-fast-approximation-of-a-trigonometric-function
 * - http://www.codeproject.com/Articles/69941/Best-Square-Root-Method-Algorithm-Function-Precisi
 */
#include "pbl-math.h"

float pbl_sqrt(float n)
{
    int i;
    n -= 1;
    float x = 2;
    for (; n > 1; n -= 2 * x++ - 1);
    n += (x - 1) * (x - 1);
    for (i = 0; i < 32; i++) {
        x += (x * x < n) ? (float) 1 / (1 << i) : (float) -1 / (1 << i);
    }
    return x;
}

float pbl_floor(float x)
{
    return ((int)x);
}

float pbl_fabs(float x)
{
    if (x<0) return -x;
    return x;
}

float pbl_round(float x)
{
    if (x>=0) {
        return (int)(x+0.5);
    } else {
        return (int)(x-0.5);
    }
}

float pbl_atan(float x)
{
    if (x>0) {
        //return (M_PI/2)*(0.596227*x + x*x)/(1 + 2*0.596227*x + x*x);
		return (M_PI/2)*( 0.640388203*x + x*x + x*x*x)/( 1 + 1.640388203 * x + 1.640388203 * x*x + x*x*x);
    } else {
        return -(pbl_atan(-x));
    }
}
/*
float pbl_atan2(float y, float x)
{
  double z;
  if (x == 0 && y == 0) {
    return 0; //atan undefined
  }
  z = pbl_atan(y/x);
  if (x > 0)
    return z;
  if (y < 0) {
    if (x < 0 ) {
      return z - M_PI;
    } else {
      return -M_PI/2;
    }
  } else {
    if (x < 0) {
      return z + M_PI;
    } else {
      return M_PI/2;
    }
  }
}
*/
/* not quite rint(), i.e. results not properly rounded to nearest-or-even */
float pbl_rint(float x)
{
    float t = pbl_floor(pbl_fabs(x) + 0.5);
    return (x < 0.0) ? -t : t;
}

/* minimax approximation to cos on [-pi/4, pi/4] with rel. err. ~= 7.5e-13 */
float cos_core(float x)
{
    float x8, x4, x2;
    x2 = x * x;
    x4 = x2 * x2;
    x8 = x4 * x4;
    /* evaluate polynomial using Estrin's scheme */
    return (-2.7236370439787708e-7 * x2 + 2.4799852696610628e-5) * x8 +
           (-1.3888885054799695e-3 * x2 + 4.1666666636943683e-2) * x4 +
           (-4.9999999999963024e-1 * x2 + 1.0000000000000000e+0);
}

/* minimax approximation to sin on [-pi/4, pi/4] with rel. err. ~= 5.5e-12 */
float sin_core(float x)
{
    float x4, x2;
    x2 = x * x;
    x4 = x2 * x2;
    /* evaluate polynomial using a mix of Estrin's and Horner's scheme */
    return ((2.7181216275479732e-6 * x2 - 1.9839312269456257e-4) * x4 +
            (8.3333293048425631e-3 * x2 - 1.6666666640797048e-1)) * x2 * x + x;
}

/* minimax approximation to arcsin on [0, 0.5625] with rel. err. ~= 1.5e-11 */
float asin_core(float x)
{
    float x8, x4, x2;
    x2 = x * x;
    x4 = x2 * x2;
    x8 = x4 * x4;
    /* evaluate polynomial using a mix of Estrin's and Horner's scheme */
    return (((4.5334220547132049e-2 * x2 - 1.1226216762576600e-2) * x4 +
             (2.6334281471361822e-2 * x2 + 2.0596336163223834e-2)) * x8 +
            (3.0582043602875735e-2 * x2 + 4.4630538556294605e-2) * x4 +
            (7.5000364034134126e-2 * x2 + 1.6666666300567365e-1)) * x2 * x + x;
}

/* relative error < 7e-12 on [-50000, 50000] */
float pbl_sin(float x)
{
    float q, t;
    int quadrant;
    /* Cody-Waite style argument reduction */
    q = pbl_rint(x * 6.3661977236758138e-1);
    quadrant = (int)q;
    t = x - q * 1.5707963267923333e+00;
    t = t - q * 2.5633441515945189e-12;
    if (quadrant & 1) {
        t = cos_core(t);
    } else {
        t = sin_core(t);
    }
    return (quadrant & 2) ? -t : t;
}

float pbl_cos(float x)
{
    return pbl_sin(x + (M_PI/2));
}

/* relative error < 2e-11 on [-1, 1] */
float pbl_acos(float x)
{
    float xa, t;
    xa = pbl_fabs(x);
    /* arcsin(x) = pi/2 - 2 * arcsin (sqrt ((1-x) / 2))
     * arccos(x) = pi/2 - arcsin(x)
     * arccos(x) = 2 * arcsin (sqrt ((1-x) / 2))
     */
    if (xa > 0.5625) {
        t = 2.0 * asin_core(pbl_sqrt(0.5 * (1.0 - xa)));
    } else {
        t = 1.5707963267948966 - asin_core(xa);
    }
    /* arccos (-x) = pi - arccos(x) */
    return (x < 0.0) ? (3.1415926535897932 - t) : t;
}

float pbl_asin(float x)
{
    return (M_PI/2) - pbl_acos(x);
}

float pbl_tan(float x)
{
    return pbl_sin(x) / pbl_cos(x);
}
