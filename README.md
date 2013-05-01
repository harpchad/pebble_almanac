Almanac
==============
Almanac watchface for the Pebble smart watch.

![ScreenShot](http://www.mypebblefaces.com/files/7613/6743/1396/almanac.png)

Includes
--------
* Time
* Date
* Moon Phase
* Dawn time
* Sunrise time
* Sunset time
* Dusk time

Issues
------
 * Invert moon phase for southern hemisphere.
 * Handle case where sun doesn't set in designated location.
 * Daylight Saving Time (really need an SDK enhancement that populates tm_isdst)
 * Use location from device (wait on SDK)
 * Allow alternate date formats.

Notes
-----
All times honor the Pebble OS 12/24 hr setting.

Made possible by:
 * Vladimir Agafonkin : sun calculations
 * Michael Ehrmann : math library

Build Instructions
------------------
Until the SDK allows watchface preferences, or exposes location information from the phone, you must custom build this watchface for your location and time zone.
