#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "mini-printf.h"
#include "pbl-math.h"
#include "sunmoon.h"
#include "config.h"

//#define USEDST 1 //Attempt to use DST, formula is for (most of) USA, need a better way...

#define MY_UUID { 0x02, 0xFC, 0xB5, 0x2F, 0xD4, 0x05, 0x4F, 0xD7, 0xB1, 0x13, 0x11, 0xE9, 0x76, 0x2B, 0xCA, 0x0C }
PBL_APP_INFO(MY_UUID,
             "Almanac", "Chad Harp",
             2, 0, /* App version */
             RESOURCE_ID_ALMANAC_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;
TextLayer timeLayer; // The clock
TextLayer dateLayer; // The date
TextLayer riseLayer; // sunrise
TextLayer setLayer; // sunset
TextLayer moonLayer; // moon phase
TextLayer moonLeft; // moonrise
TextLayer moonRight; // moonset
TextLayer moonPercent;//
TextLayer location;

//Make fonts global so we can deinit later
GFont font_roboto;
GFont font_moon;

//is it daylight time?
//need a better way
/*
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
*/

//return julian day number for time
int tm2jd(PblTm* time)
{
    int y,m;
    y = time->tm_year + 1900;
    m = time->tm_mon + 1;
    return time->tm_mday-32075+1461*(y+4800+(m-14)/12)/4+367*(m-2-(m-14)/12*12)/12-3*((y+4900+(m-14)/12)/100)/4;
}

float moon_phase(int jdn)
{
    double jd;
    jd = jdn-2451550.1;
    jd /= 29.530588853;
    jd -= (int)jd;
    return jd;
}

/*Convert decimal hours, into hours and minutes with rounding*/
int hours(float time)
{
    return (int)(time+1/60.0*0.5);
}

int mins(float time)
{
    int m = (int)((time-(int)time)*60.0+0.5);
	return (m==60)?0:m;
}

//If 12 hour time, subtract 12 from hr if hr > 12
char* thr(float time, char ap)
{
    static char fmttime[] = "00:00A";
    int h = hours(time);
    int m = mins(time);
    if (clock_is_24h_style()) {
        mini_snprintf(fmttime, sizeof(fmttime), "%d:%02d",h,m);
    } else {
        if (h > 12) {
            h -= 12;
            mini_snprintf(fmttime, sizeof(fmttime), (ap==1)?"%d:%02dP":"%d:%02d",h,m);
        } else {
            mini_snprintf(fmttime, sizeof(fmttime), (ap==1)?"%d:%02dA":"%d:%02d",h,m);
        }
    }
    return fmttime;
}
char* mthr(float time1, float time2, char* inject)
{
    static char fmttime[] = "00:00A> 00:00A";
    int h1 = hours(time1);
    int m1 = mins(time1);
    int h2 = hours(time2);
    int m2 = mins(time2);
    if (clock_is_24h_style()) {
        mini_snprintf(fmttime, sizeof(fmttime), "%d:%02d%s %d:%02d",h1,m1,inject,h2,m2);
    } else {
        if (h1 > 12 && h2 > 12) {
            h1 -= 12;
            h2 -= 12;
            mini_snprintf(fmttime, sizeof(fmttime), "%d:%02dP%s %d:%02dP",h1,m1,inject,h2,m2);
        } else if (h1 > 12) {
            h1 -= 12;
            mini_snprintf(fmttime, sizeof(fmttime), "%d:%02dP%s %d:%02dA",h1,m1,inject,h2,m2);
        } else if (h2 > 12) {
            h2 -= 12;
            mini_snprintf(fmttime, sizeof(fmttime), "%d:%02dA%s %d:%02dP",h1,m1,inject,h2,m2);
        } else {
            mini_snprintf(fmttime, sizeof(fmttime), "%d:%02dA%s %d:%02dA",h1,m1,inject,h2,m2);
        }
    }
    return fmttime;
}

// Called once per day
void handle_day(AppContextRef ctx, PebbleTickEvent* t)
{

    (void)t;
    (void)ctx;

    static char riseText[] = "00:00a 00:00a";
    static char setText[] = "00:00a 00:00a";
    static char moon1Text[] = "<00:00a\n<00:00a";
    static char moon2Text[] = "00:00a>\n00:00a>";
    static char date[] = "00/00/0000";
    static char moon[] = "m";
    static char moonp[] = "-----";
    char riseTemp[] = "00:00a";
    char setTemp[] = "00:00a";
    float moonphase_number = 0.0;
    int moonphase_letter = 0;
    float sunrise, sunset, dawn, dusk, moonrise[3], moonset[3];
    PblTm* time = t->tick_time;
    if (!t)
        get_time(time);

    // date
    string_format_time(date, sizeof(date), DATEFMT, time);
    text_layer_set_text(&dateLayer, date);


    moonphase_number = moon_phase(tm2jd(time));
    moonphase_letter = (int)(moonphase_number*27 + 0.5);
    // correct for southern hemisphere
    if ((moonphase_letter > 0) && (LAT < 0))
        moonphase_letter = 28 - moonphase_letter;
    // select correct font char
    if (moonphase_letter == 14) {
        moon[0] = (unsigned char)(48);
    } else if (moonphase_letter == 0) {
        moon[0] = (unsigned char)(49);
    } else if (moonphase_letter < 14) {
        moon[0] = (unsigned char)(moonphase_letter+96);
    } else {
        moon[0] = (unsigned char)(moonphase_letter+95);
    }
    text_layer_set_text(&moonLayer, moon);
    if (moonphase_number >= 0.5) {
        mini_snprintf(moonp,sizeof(moonp)," %d-",(int)((1-(1+pbl_cos(moonphase_number*M_PI*2))/2)*100));
    } else {
        mini_snprintf(moonp,sizeof(moonp)," %d+",(int)((1-(1+pbl_cos(moonphase_number*M_PI*2))/2)*100));
    }
    text_layer_set_text(&moonPercent, moonp);

    //sun rise set
    sunmooncalc(tm2jd(time), TZ, LAT, -LON, 1, &sunrise, &sunset);
    sunmooncalc(tm2jd(time), TZ, LAT, -LON, 2, &dawn, &dusk);

    (dawn == 99.0) ? mini_snprintf(riseTemp,sizeof(riseTemp),"--:--") : mini_snprintf(riseTemp,sizeof(riseTemp),"%s",thr(dawn,0));
    (sunrise == 99.0) ?  mini_snprintf(riseText,sizeof(riseText),"%s --:--",riseTemp) : mini_snprintf(riseText,sizeof(riseText),"%s  %s",riseTemp,thr(sunrise,0));
    (sunset == 99.0) ? mini_snprintf(setTemp,sizeof(setTemp),"--:--") : mini_snprintf(setTemp,sizeof(setTemp),"%s",thr(sunset,0));
    (dusk == 99.0) ? mini_snprintf(setText,sizeof(setText),"%s --:--",setTemp) : mini_snprintf(setText,sizeof(setText),"%s  %s",setTemp,thr(dusk,0));

    text_layer_set_text(&riseLayer, riseText);
    text_layer_set_text(&setLayer, setText);

    //moon times
    sunmooncalc(tm2jd(time)-1, TZ, LAT, -LON, 0, &moonrise[0], &moonset[0]); // yesterday
    sunmooncalc(tm2jd(time), TZ, LAT, -LON, 0, &moonrise[1], &moonset[1]); // today
    sunmooncalc(tm2jd(time)+1, TZ, LAT, -LON, 0, &moonrise[2], &moonset[2]); // tomorrow

    if (moonrise[1] == 99.0) { // moon didn't rise today
        mini_snprintf(moon1Text,sizeof(moon1Text),"<%s",mthr(moonrise[0],moonset[1],"\0"));
        mini_snprintf(moon2Text,sizeof(moon2Text),"%s>",mthr(moonrise[2],moonset[2],">"));
    } else if (moonset[1] == 99.0) { // moon didn't set today
        mini_snprintf(moon1Text,sizeof(moon1Text),"%s>",mthr(moonrise[1],moonset[2],"\0"));
        mini_snprintf(moon2Text,sizeof(moon2Text),"%s>\n--:--",thr(moonrise[2],1));
    } else if (moonrise[1] > moonset[1]) { // moon rose before midnight, rises again today
        mini_snprintf(moon1Text,sizeof(moon1Text),"<%s",mthr(moonrise[0],moonset[1],"\0"));
        mini_snprintf(moon2Text,sizeof(moon2Text),"%s>",mthr(moonrise[1],moonset[2],"\0"));
    } else { // moon was down at midnight, rose today
        mini_snprintf(moon1Text,sizeof(moon1Text),"%s",mthr(moonrise[1],moonset[1],"\0"));
        mini_snprintf(moon2Text,sizeof(moon2Text),"%s>",mthr(moonrise[2],moonset[2],">"));
   }

    text_layer_set_text(&moonLeft, moon1Text);
    text_layer_set_text(&moonRight, moon2Text);
}

// Called once per minute
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent* t)
{

    (void)t;
    (void)ctx;

    static char timeText[] = "00:00"; // Needs to be static because it's used by the system later.
    char* time_format;

	static const uint32_t const segments[] = {70};
	VibePattern pat = {
	    .durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};
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
            vibes_enqueue_custom_pattern(pat);
#endif
        //perform daily tasks is hour is 0
        if (t->tick_time->tm_hour == 0)
            handle_day(ctx, t);
    }
}

void handle_init(AppContextRef ctx)
{
    (void)ctx;

    window_init(&window, "Almanac");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);

    resource_init_current_app(&APP_RESOURCES);
    font_moon = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MOON_PHASES_SUBSET_30));
    font_roboto = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));

    text_layer_init(&timeLayer, GRect(0, 40, 144 /* width */, 168-40 /* height */));
    text_layer_set_text_color(&timeLayer, GColorWhite);
    text_layer_set_background_color(&timeLayer, GColorClear);
    text_layer_set_font(&timeLayer, font_roboto);
    text_layer_set_text_alignment(&timeLayer, GTextAlignmentCenter);

    text_layer_init(&dateLayer, GRect(0, 0, 144 /* width */, 168 /* height */));
    text_layer_set_text_color(&dateLayer, GColorWhite);
    text_layer_set_background_color(&dateLayer, GColorClear);
    text_layer_set_font(&dateLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(&dateLayer, GTextAlignmentCenter);

    text_layer_init(&location, GRect(0, 30, 144 /* width */, 168-30 /* height */));
    text_layer_set_text_color(&location, GColorWhite);
    text_layer_set_background_color(&location, GColorClear);
    text_layer_set_font(&location, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(&location, GTextAlignmentCenter);

    text_layer_init(&riseLayer, GRect(0, 148, 144 /* width */, 168-148 /* height */));
    text_layer_set_text_color(&riseLayer, GColorWhite);
    text_layer_set_background_color(&riseLayer, GColorClear);
    text_layer_set_font(&riseLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(&riseLayer, GTextAlignmentLeft);

    text_layer_init(&setLayer, GRect(0, 148, 144 /* width */, 168-148 /* height */));
    text_layer_set_text_color(&setLayer, GColorWhite);
    text_layer_set_background_color(&setLayer, GColorClear);
    text_layer_set_font(&setLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(&setLayer, GTextAlignmentRight);

    text_layer_init(&moonLayer, GRect(0, 108, 144 /* width */, 168-108 /* height */));
    text_layer_set_text_color(&moonLayer, GColorWhite);
    text_layer_set_background_color(&moonLayer, GColorClear);
    text_layer_set_font(&moonLayer, font_moon);
    text_layer_set_text_alignment(&moonLayer, GTextAlignmentCenter);

    text_layer_init(&moonPercent, GRect(0, 135, 144 /* width */, 168-135 /* height */));
    text_layer_set_text_color(&moonPercent, GColorWhite);
    text_layer_set_background_color(&moonPercent, GColorClear);
    text_layer_set_font(&moonPercent, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(&moonPercent, GTextAlignmentCenter);

    text_layer_init(&moonLeft, GRect(0, 102, 50, 168-102 /* height */));
    text_layer_set_text_color(&moonLeft, GColorWhite);
    text_layer_set_background_color(&moonLeft, GColorClear);
    text_layer_set_font(&moonLeft, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(&moonLeft, GTextAlignmentRight);

    text_layer_init(&moonRight, GRect(94, 102, 144-94, 168-102 /* height */));
    text_layer_set_text_color(&moonRight, GColorWhite);
    text_layer_set_background_color(&moonRight, GColorClear);
    text_layer_set_font(&moonRight, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(&moonRight, GTextAlignmentLeft);

    handle_day(ctx, NULL);
    handle_minute_tick(ctx, NULL);
	text_layer_set_text(&location, LOC);
	
    layer_add_child(&window.layer, &timeLayer.layer);
    layer_add_child(&window.layer, &dateLayer.layer);
    layer_add_child(&window.layer, &riseLayer.layer);
    layer_add_child(&window.layer, &setLayer.layer);
    layer_add_child(&window.layer, &moonLayer.layer);
    layer_add_child(&window.layer, &moonLeft.layer);
    layer_add_child(&window.layer, &moonRight.layer);
    layer_add_child(&window.layer, &moonPercent.layer);
	layer_add_child(&window.layer, &location.layer);
}

void handle_deinit(AppContextRef ctx)
{
    (void)ctx;
    fonts_unload_custom_font(font_moon);
    fonts_unload_custom_font(font_roboto);
}

void pbl_main(void* params)
{
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
