#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "xprintf.h"
#include "pbl-math.h"
#include "sunmoon.h"
#include "config.h"

//#define USEDST 1 //Attempt to use DST, formula is for (most of) USA, need a better way...

#define MY_UUID { 0x02, 0xFC, 0xB5, 0x2F, 0xD4, 0x05, 0x4F, 0xD7, 0xB1, 0x13, 0x11, 0xE9, 0x76, 0x2B, 0xCA, 0x0C }
PBL_APP_INFO(MY_UUID,
             "Almanac", "Chad Harp",
             0, 2, /* App version */
             RESOURCE_ID_ALMANAC_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;
TextLayer timeLayer; // The clock
TextLayer dateLayer; // The date
TextLayer riseLayer; // sunrise
TextLayer setLayer; // sunset
TextLayer moonLayer; // moon phase
TextLayer moonRise; // moonrise
TextLayer moonSet; // moonset

//Make fonts global so we can deinit later
GFont font_roboto;
GFont font_moon;

//is it daylight time?
//need a better way
bool isDST(int day, int month, int dow)
{
    //January, february, and december are out.
    if (month < 3 || month > 11) {
        return false;
    }
    //April to October are in
    if (month > 3 && month < 11) {
        return true;
    }
    int previousSunday = day - dow;
    //In march, we are DST if our previous sunday was on or after the 8th.
    if (month == 3) {
        return previousSunday >= 8;
    }
    //In november we must be before the first sunday to be dst.
    //That means the previous sunday must be before the 1st.
    return previousSunday <= 0;
}

//If 12 hour time, subtract 12 from hr if hr > 12
int thr(int hr)
{
    return !clock_is_24h_style() && (hr > 12) ? hr - 12 : hr;
}

//return julian day number for time
int tm2jd(PblTm *time)
{
    int y,m;
    y = time->tm_year + 1900;
    m = time->tm_mon + 1;
    return time->tm_mday-32075+1461*(y+4800+(m-14)/12)/4+367*(m-2-(m-14)/12*12)/12-3*((y+4900+(m-14)/12)/100)/4;
}

int moon_phase(int jdn)
{
    double jd;
    jd = jdn-2451550.1;
    jd /= 29.530588853;
    jd -= (int)jd;
    return (int)(jd*27 + 0.5); /* scale fraction from 0-27 and round by adding 0.5 */
}                              /* 0 = new, 14 = full */

// Called once per day
void handle_day(AppContextRef ctx, PebbleTickEvent *t) {

    (void)t;
    (void)ctx;

    static char riseText[] = "00:00  00:00";
    static char setText[] = "00:00  00:00";
    static char moonriseText[] = "00:00";
    static char moonsetText[] = "00:00";
    static char date[] = "00/00/0000";
    static char moon[] = "m";
    int moonphase_number = 0;
    float sunrise, sunset, dawn, dusk, moonrise, moonset;
    PblTm *time = t->tick_time;
    if(!t)
        get_time(time);

    // date
    string_format_time(date, sizeof(date), DATEFMT, time);
    text_layer_set_text(&dateLayer, date);

    // moon
    moonphase_number = moon_phase(tm2jd(time));
    // correct for southern hemisphere
    if ((moonphase_number > 0) && (LAT < 0))
        moonphase_number = 28 - moonphase_number;
    // select correct font char
    if (moonphase_number == 14)
    {
        moon[0] = (unsigned char)(48);
    } else if (moonphase_number == 0) {
        moon[0] = (unsigned char)(49);
    } else if (moonphase_number < 14) {
        moon[0] = (unsigned char)(moonphase_number+96);
    } else {
        moon[0] = (unsigned char)(moonphase_number+95);
    }
    text_layer_set_text(&moonLayer, moon);

    //sun rise set
    sunmooncalc(time->tm_mday,time->tm_mon+1,time->tm_year+1900, TZ, LAT, -LON, &sunrise, &sunset, &dawn, &dusk, &moonrise, &moonset);
    //if (isDST(time->tm_mday,time->tm_mon,time->tm_wday))
    //  ++time->tm_hour;

    if (dawn == 99.0 || sunrise == 99.0) {
      if (dawn == 99.0) {
            xsprintf(riseText,"--:--  %d:%02d",
             thr((int)sunrise),(int)((sunrise-(int)sunrise)*60.0+0.5)
            );
      } else if (sunrise == 99.0) {
            xsprintf(riseText,"%d:%02d  --:--",
             thr((int)dawn),(int)((dawn-(int)dawn)*60.0+0.5)
            );
      } else {
            xsprintf(riseText,"--:--  --:--");
      }
    } else {
            xsprintf(riseText,"%d:%02d  %d:%02d",
             thr((int)dawn),(int)((dawn-(int)dawn)*60.0+0.5),
             thr((int)sunrise),(int)((sunrise-(int)sunrise)*60.0+0.5)
            );
    }
    if (sunset == 99.0 || dusk == 99.0) {
      if (sunset == 99.0) {
            xsprintf(setText,"--:--  %d:%02d",
             thr((int)dusk),(int)((dusk-(int)dusk)*60.0+0.5)
            );
      } else if (dusk == 99.0) {
            xsprintf(setText,"%d:%02d  --:--",
             thr((int)sunset),(int)((sunset-(int)sunset)*60.0+0.5)
            );
      } else {
            xsprintf(setText,"--:--  --:--");
      }
    } else {
            xsprintf(setText,"%d:%02d  %d:%02d",
             thr((int)sunset),(int)((sunset-(int)sunset)*60.0+0.5),
             thr((int)dusk),(int)((dusk-(int)dusk)*60.0+0.5)
            );
    }
    if (moonrise==99.0) {
      xsprintf(moonriseText,"--:--");
    } else {
      xsprintf(moonriseText,"%d:%02d",(int)moonrise,(int)((moonrise-(int)moonrise)*60.0+0.5));
    }
    if (moonset==99.0) {
      xsprintf(moonsetText,"--:--");
    } else {
      xsprintf(moonsetText,"%d:%02d",(int)moonset,(int)((moonset-(int)moonset)*60.0+0.5));
    }
    text_layer_set_text(&riseLayer, riseText);
    text_layer_set_text(&setLayer, setText);
    text_layer_set_text(&moonSet, moonsetText);
    text_layer_set_text(&moonRise, moonriseText);
}

// Called once per minute
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {

    (void)t;
    (void)ctx;

    static char timeText[] = "00:00"; // Needs to be static because it's used by the system later.
    char *time_format;

    if (clock_is_24h_style()) {
        time_format = "%R";
    } else {
        time_format = "%I:%M";
    }

    string_format_time(timeText, sizeof(timeText), time_format, t->tick_time);
    if (!clock_is_24h_style() && (timeText[0] == '0')) {
        memmove(timeText, &timeText[1], sizeof(timeText) - 1);
    }

    text_layer_set_text(&timeLayer, timeText);

    // on the top of the hour
    if (t->tick_time->tm_min == 0) {
#ifdef VIBHOUR
        // vibrate once if between 6am and 10pm
        if (t->tick_time->tm_hour >= 6 && t->tick_time->tm_hour <= 22)
            vibes_short_pulse();
#endif
        //perform daily tasks is hour is 0
        if (t->tick_time->tm_hour == 0)
            handle_day(ctx, t);
    }
}

void handle_init(AppContextRef ctx) {
    (void)ctx;

    window_init(&window, "Almanac");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);

    resource_init_current_app(&APP_RESOURCES);
    font_moon = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MOON_PHASES_SUBSET_30));
    font_roboto = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));

    text_layer_init(&timeLayer, GRect(0, 55, 144 /* width */, 168-60 /* height */));
    text_layer_set_text_color(&timeLayer, GColorWhite);
    text_layer_set_background_color(&timeLayer, GColorClear);
    text_layer_set_font(&timeLayer, font_roboto);
    text_layer_set_text_alignment(&timeLayer, GTextAlignmentCenter);

    text_layer_init(&dateLayer, GRect(0, 0, 144 /* width */, 168 /* height */));
    text_layer_set_text_color(&dateLayer, GColorWhite);
    text_layer_set_background_color(&dateLayer, GColorClear);
    text_layer_set_font(&dateLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(&dateLayer, GTextAlignmentCenter);

    text_layer_init(&riseLayer, GRect(0, 150, 144 /* width */, 168-130 /* height */));
    text_layer_set_text_color(&riseLayer, GColorWhite);
    text_layer_set_background_color(&riseLayer, GColorClear);
    text_layer_set_font(&riseLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(&riseLayer, GTextAlignmentLeft);

    text_layer_init(&setLayer, GRect(0, 150, 144 /* width */, 168-130 /* height */));
    text_layer_set_text_color(&setLayer, GColorWhite);
    text_layer_set_background_color(&setLayer, GColorClear);
    text_layer_set_font(&setLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(&setLayer, GTextAlignmentRight);

    text_layer_init(&moonLayer, GRect(0, 115, 144 /* width */, 168-115 /* height */));
    text_layer_set_text_color(&moonLayer, GColorWhite);
    text_layer_set_background_color(&moonLayer, GColorClear);
    text_layer_set_font(&moonLayer, font_moon);
    text_layer_set_text_alignment(&moonLayer, GTextAlignmentCenter);

    text_layer_init(&moonRise, GRect(0, 120, 62, 168-120 /* height */));
    text_layer_set_text_color(&moonRise, GColorWhite);
    text_layer_set_background_color(&moonRise, GColorClear);
    text_layer_set_font(&moonRise, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(&moonRise, GTextAlignmentRight);

    text_layer_init(&moonSet, GRect(82, 120, 144-82, 168-120 /* height */));
    text_layer_set_text_color(&moonSet, GColorWhite);
    text_layer_set_background_color(&moonSet, GColorClear);
    text_layer_set_font(&moonSet, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(&moonSet, GTextAlignmentLeft);

    handle_day(ctx, NULL);
    handle_minute_tick(ctx, NULL);
    layer_add_child(&window.layer, &timeLayer.layer);
    layer_add_child(&window.layer, &dateLayer.layer);
    layer_add_child(&window.layer, &riseLayer.layer);
    layer_add_child(&window.layer, &setLayer.layer);
    layer_add_child(&window.layer, &moonLayer.layer);
    layer_add_child(&window.layer, &moonRise.layer);
    layer_add_child(&window.layer, &moonSet.layer);
}

void handle_deinit(AppContextRef ctx) {
    (void)ctx;
    fonts_unload_custom_font(font_moon);
    fonts_unload_custom_font(font_roboto);
}

void pbl_main(void *params) {
    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .deinit_handler = &handle_deinit,
        .tick_info = {
            .tick_handler = &handle_minute_tick,
            .tick_units = MINUTE_UNIT
        }
    };
    app_event_loop(params, &handlers);
}
