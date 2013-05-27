#include "pbl-math.h"
#define trunc(x)  ((int)(x))
#define rad M_PI/180
#define true 1
#define false 0
/*-----------------------------------------------------------------------*/
/*                                 SUNSET                                */
/*                 solar and lunar rising and setting times              */
/*                            version 93/07/01                           */
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* LMST: local mean sidedouble time                                        */
/*-----------------------------------------------------------------------*/
double lmst(double mjd,double lambda);

static double frac(double x)
{
    double frac_result;
    x=x-trunc(x);
    if (x<0)  x=x+1;
    frac_result=x;
    return frac_result;
}

double lmst(double mjd,double lambda)
{
    double mjd0,t,ut,gmst;
    double lmst_result;
    mjd0=trunc(mjd);
    ut=(mjd-mjd0)*24;
    t=(mjd0-51544.5)/36525.0;
    gmst=6.697374558 + 1.0027379093*ut
         +(8640184.812866+(0.093104-6.2E-6*t)*t)*t/3600.0;
    lmst_result=24.0*frac((gmst-lambda/15.0) / 24.0);
    return lmst_result;
}

/* ABS function*/
double dabs(double x)
{
    return  x < 0 ? -x : x;
}

/*-----------------------------------------------------------------------*/
/* SN: sine function (degrees)                                           */
/*-----------------------------------------------------------------------*/
double sn(double x)
{
    double sn_result;
    sn_result=pbl_sin(x*rad);
    return sn_result;
}
/*-----------------------------------------------------------------------*/
/* CS: cosine function (degrees)                                         */
/*-----------------------------------------------------------------------*/
double cs(double x)
{
    double cs_result;
    cs_result=pbl_cos(x*rad);
    return cs_result;
}
/*-----------------------------------------------------------------------*/
/* QUAD: finds a parabola through 3 points                               */
/*       (-1,Y_MINUS), (0,Y_0) und (1,Y_PLUS),                           */
/*       that do not lie on a straight line.                             */
/*                                                                       */
/*      Y_MINUS,Y_0,Y_PLUS: three y-values                               */
/*      XE,YE   : x and y of the extreme value of the parabola           */
/*      ZERO1   : first root within [-1,+1] (for NZ=1,2)                 */
/*      ZERO2   : second root within [-1,+1] (only for NZ=2)             */
/*      NZ      : number of roots within the interval [-1,+1]            */
/*-----------------------------------------------------------------------*/
void quad(double y_minus,double y_0,double y_plus,
          double* xe,double* ye,double* zero1,double* zero2, int* nz)
{
    double a,b,c,dis,dx;
    *nz = 0;
    a  = 0.5*(y_minus+y_plus)-y_0;
    b = 0.5*(y_plus-y_minus);
    c = y_0;
    *xe = -b/(2.0*a);
    *ye = (a* *xe + b) * *xe + c;
    dis = b*b - 4.0*a*c;  /* discriminant of y = axx+bx+c */
    if (dis >= 0) {       /* parabola intersects x-axis   */
        dx = 0.5*pbl_sqrt(dis)/dabs(a);
        *zero1 = *xe-dx;
        *zero2 = *xe+dx;
        if (dabs(*zero1) <= 1.0)  *nz += 1;
        if (dabs(*zero2) <= 1.0)  *nz += 1;
        if (*zero1<-1.0)  *zero1=*zero2;
    }
}

/*-----------------------------------------------------------------------*/
/* MINI_MOON: low precision lunar coordinates (approx. 5'/1')            */
/*            T  : time in Julian centuries since J2000                  */
/*                 ( T=(JD-2451545)/36525 )                              */
/*            RA : right ascension (in h; equinox of date)               */
/*            DEC: declination (in deg; equinox of date)                 */
/*-----------------------------------------------------------------------*/
static double frac1(double x)
/* with some compilers it may be necessary to replace */
/* TRUNC by LONG_TRUNC oder INT if T<-24!             */
{
    double frac1_result;
    x=x-trunc(x);
    if (x<0)  x=x+1;
    frac1_result=x;
    return frac1_result;
}

void mini_moon(double t, double* ra,double* dec)
{
    const double p2 = 6.283185307;
    const double arc = 206264.8062;
    const double coseps = 0.91748;
    const double sineps = 0.39778;  /* cos/pbl_sin(obliquity ecliptic)  */
    double l0,l,ls,f,d,h,s,n,dl,cb;
    double l_moon,b_moon,v,w,x,y,z,rho;
    /* mean elements of lunar orbit */
    l0=   frac1(0.606433+1336.855225*t); /* mean longitude Moon (in rev) */
    l =p2*frac1(0.374897+1325.552410*t); /* mean anomaly of the Moon     */
    ls=p2*frac1(0.993133+  99.997361*t); /* mean anomaly of the Sun      */
    d =p2*frac1(0.827361+1236.853086*t); /* diff. longitude Moon-Sun     */
    f =p2*frac1(0.259086+1342.227825*t); /* mean argument of latitude    */
    dl = +22640*pbl_sin(l) - 4586*pbl_sin(l-2*d) + 2370*pbl_sin(2*d) +  769*pbl_sin(2*l)
         -668*pbl_sin(ls)- 412*pbl_sin(2*f) - 212*pbl_sin(2*l-2*d) - 206*pbl_sin(l+ls-2*d)
         +192*pbl_sin(l+2*d) - 165*pbl_sin(ls-2*d) - 125*pbl_sin(d) - 110*pbl_sin(l+ls)
         +148*pbl_sin(l-ls) - 55*pbl_sin(2*f-2*d);
    s = f + (dl+412*pbl_sin(2*f)+541*pbl_sin(ls)) / arc;
    h = f-2*d;
    n = -526*pbl_sin(h) + 44*pbl_sin(l+h) - 31*pbl_sin(-l+h) - 23*pbl_sin(ls+h)
        + 11*pbl_sin(-ls+h) -25*pbl_sin(-2*l+f) + 21*pbl_sin(-l+f);
    l_moon = p2 * frac1(l0 + dl/1296E3);    /* in rad */
    b_moon = (18520.0*pbl_sin(s) + n) / arc;    /* in rad */
    /* equatorial coordinates */
    cb=pbl_cos(b_moon);
    x=cb*pbl_cos(l_moon);
    v=cb*pbl_sin(l_moon);
    w=pbl_sin(b_moon);
    y=coseps*v-sineps*w;
    z=sineps*v+coseps*w;
    rho=pbl_sqrt(1.0-z*z);
    *dec = (360.0/p2)*pbl_atan(z/rho);
    *ra  = (48.0/p2)*pbl_atan(y/(x+rho));
    if (*ra<0)  *ra+=24.0;
}

/*-----------------------------------------------------------------------*/
/* MINI_SUN: low precision solar coordinates (approx. 1')                */
/*           T  : time in Julian centuries since J2000                   */
/*                ( T=(JD-2451545)/36525 )                               */
/*           RA : right ascension (in h; equinox of date)                */
/*           DEC: declination (in deg; equinox of date)                  */
/*-----------------------------------------------------------------------*/
static double frac2(double x)
{
    double frac2_result;
    x=x-trunc(x);
    if (x<0)  x=x+1;
    frac2_result=x;
    return frac2_result;
}

void mini_sun(double t, double* ra,double* dec)
{
    const double p2 = 6.283185307;
    const double coseps = 0.91748;
    const double sineps = 0.39778;
    double l,m,dl,sl,x,y,z,rho;
    m  = p2*frac2(0.993133+99.997361*t);
    dl = 6893.0*pbl_sin(m)+72.0*pbl_sin(2*m);
    l  = p2*frac2(0.7859453 + m/p2 + (6191.2*t+dl)/1296E3);
    sl = pbl_sin(l);
    x=pbl_cos(l);
    y=coseps*sl;
    z=sineps*sl;
    rho=pbl_sqrt(1.0-z*z);
    *dec = (360.0/p2)*pbl_atan(z/rho);
    *ra  = (48.0/p2)*pbl_atan(y/(x+rho));
    if (*ra<0)  *ra+=24.0;
}

/*-----------------------------------------------------------------------*/
/* SIN_ALT: sin(altitude)                                                */
/*         IOBJ:  0=moon, >0=sun                                          */
/*-----------------------------------------------------------------------*/
double sin_alt(int iobj,double mjd0,double hour,double lambda,double cphi,double sphi)
{
    double mjd,t,ra,dec,tau;
    mjd = mjd0 + hour/24.0;
    t   = (mjd-51544.5)/36525.0;
    if (iobj==0)
        mini_moon(t,&ra,&dec);
    else  mini_sun(t,&ra,&dec);
    tau = 15.0 * (lmst(mjd,lambda) - ra);
    return sphi*sn(dec) + cphi*cs(dec)*cs(tau);
}

/*
 * Calculate rise and set time for sun and moon
 * 0 = moon rise / set
 * 1 = sun rise / set
 * 2 = sun dawn / dusk
 */
void sunmooncalc(double jd, float tz, float lat, float lon, int iobj, float* utrise, float* utset)
{
    unsigned char rise,sett;
    int nz;
    double lambda,zone,phi,sphi,cphi;
    double date,hour;
    double y_minus,y_0,y_plus,zero1,zero2,xe,ye;
    double sinh0[] = {
        sn(+8.0/60.0),  /* moonrise          at h= +8'        */
        sn(-50.0/60.0),  /* sunrise           at h=-50'        */
        sn(-6.0)
    };  /* civil twilight at h=-6 degrees */
    zone = tz / 24.0;
    lambda = lon;
    phi = lat;
    sphi = sn(phi);
    cphi = cs(phi);
    date = (long)(jd-2400000.5)-zone;
    hour = 1.0;
    y_minus = sin_alt(iobj,date,hour-1.0,lambda,cphi,sphi)
              - sinh0[iobj];
    rise = false;
    sett = false;
    /* loop over search intervals from [0h-2h] to [22h-24h]  */
    do {
        y_0    = sin_alt(iobj,date,hour,lambda,cphi,sphi)
                 -  sinh0[iobj];
        y_plus = sin_alt(iobj,date,hour+1.0,lambda,cphi,sphi)
                 -  sinh0[iobj];
        /* find parabola through three values Y_MINUS,Y_0,Y_PLUS */
        quad(y_minus,y_0,y_plus, &xe,&ye, &zero1,&zero2, &nz);
        switch (nz) {
        case 0:
            ;
            break;
        case 1:
            if (y_minus<0.0) {
                *utrise=hour+zero1;
                rise=true;
            } else {
                *utset =hour+zero1;
                sett=true;
            }
            break;
        case 2: {
            if (ye<0.0) {
                *utrise=hour+zero2;
                *utset=hour+zero1;
            } else {
                *utrise=hour+zero1;
                *utset=hour+zero2;
            }
            rise=true;
            sett=true;
        }
        break;
        }
        y_minus = y_plus;      /* prepare for next interval */
        hour += 2.0;
    } while (!((hour>=25.0) || (rise==true && sett==true)));
    if (rise!=true) *utrise=99.0;
    if (sett!=true) *utset=99.0;
}
