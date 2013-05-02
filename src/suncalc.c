/*
 * Based on suncalc.js by Vladimir Agafonkin
 */
#include "pbl-math.h"
#include "suncalc.h"
#define J1970 2440588.0
#define J2000 2451545.0
#define deg2rad M_PI / 180.0
#define M0 357.5291 * deg2rad
#define M1 0.98560028 * deg2rad
#define J0 0.0009
#define J1 0.0053
#define J2 -0.0069
#define C1 1.9148 * deg2rad
#define C2 0.0200 * deg2rad
#define C3 0.0003 * deg2rad
#define P 102.9372 * deg2rad
#define e 23.45 * deg2rad
#define th0 280.1600 * deg2rad
#define th1 360.9856235 * deg2rad
#define h0 -0.83 * deg2rad //sunset angle
#define d0 0.53 * deg2rad //sun diameter
#define h1 -6 * deg2rad //civil twilight angle
#define h2 -12 * deg2rad //nautical twilight angle
#define h3 -18 * deg2rad //astronomical twilight angle

double getJulianCycle( double J, float lw ) {
    return pbl_round(J - J2000 - J0 - lw/(2 * M_PI));
}

double getApproxSolarTransit( double Ht, float lw, double n ) {
    return J2000 + J0 + (Ht + lw)/(2 * M_PI) + n;
}

double getSolarMeanAnomaly( double Js ) {
    return M0 + M1 * (Js - J2000);
}

double getEquationOfCenter( double M ) {
    return C1 * pbl_sin(M) + C2 * pbl_sin(2 * M) + C3 * pbl_sin(3 * M);
}

double getEclipticLongitude( double M, double C ) {
    return M + P + C + M_PI;
}

double getSolarTransit( double Js, double M, double Lsun ) {
    return Js + (J1 * pbl_sin(M)) + (J2 * pbl_sin(2 * Lsun));
}

double getSunDeclination( double Lsun ) {
    return pbl_asin(pbl_sin(Lsun) * pbl_sin(e));
}

double getRightAscension( double Lsun ) {
    return pbl_atan(pbl_cos(Lsun)/(pbl_sin(Lsun) * pbl_cos(e)));
}

double getSiderealTime( double J, float lw ) {
    return th0 + th1 * (J - J2000) - lw;
}

double getAzimuth( double th, double a, float phi, double d ) {
    double H = th - a;
    return pbl_atan((pbl_cos(H) * pbl_sin(phi) -
                     pbl_tan(d) * pbl_cos(phi))/pbl_sin(H));
}

double getAltitude( double th, double a, float phi, double d ) {
    double H = th - a;
    return pbl_asin(pbl_sin(phi) * pbl_sin(d) +
                    pbl_cos(phi) * pbl_cos(d) * pbl_cos(H));
}

double getHourAngle( double h, float phi, double d ) {
    return pbl_acos((pbl_sin(h) - pbl_sin(phi) * pbl_sin(d)) /
                    (pbl_cos(phi) * pbl_cos(d)));
}

double getSunsetJulianDate( double w0, double M, double Lsun, float lw, double n ) {
    return getSolarTransit(getApproxSolarTransit(w0, lw, n), M, Lsun);
}

double getSunriseJulianDate( double Jtransit, double Jset ) {
    return Jtransit - (Jset - Jtransit);
}

void getDayInfo( double date, float lat, float lng, double* dawn, double* sunrise, double* sunset, double* dusk )
{
    float lw = -lng * deg2rad,
          phi = lat * deg2rad;

    double n = getJulianCycle(date, lw),
           Js = getApproxSolarTransit(0, lw, n),
           M = getSolarMeanAnomaly(Js),
           C = getEquationOfCenter(M),
           Lsun = getEclipticLongitude(M, C),
           d = getSunDeclination(Lsun),
           Jtransit = getSolarTransit(Js, M, Lsun),
           w0 = getHourAngle(h0, phi, d),
           Jset = getSunsetJulianDate(w0, M, Lsun, lw, n),
           Jrise = getSunriseJulianDate(Jtransit, Jset),
           w2 = getHourAngle(h1, phi, d),
           Jnau = getSunsetJulianDate(w2, M, Lsun, lw, n),
           Jciv = getSunriseJulianDate(Jtransit, Jnau);

    *dawn = Jciv;
    *sunrise = Jrise;
    *sunset = Jset;
    *dusk = Jnau; //begin of nautical twilight
}
