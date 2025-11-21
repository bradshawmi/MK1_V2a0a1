// NOTE: INDEX_HTML has been extracted to web_pages.h (see branch refactor/extract-webpages)

#define DEBUG_LOGS 0
#include <stdint.h>
#include <stddef.h>

static volatile uint32_t stateEpoch = 0;
// === INSERTION POINT: TYPES_GUARD_ABOVE ===

struct MsgInputs;
enum Effect : uint8_t;
struct Zone;
enum DFPhase : uint8_t;
struct DFState;
struct LightningZoneState;
// === INSERTION POINT: OPTIONAL_PROTOTYPES ===
struct CRGB;

static inline void auroraUpdate(uint8_t z, uint16_t speed);
static inline CRGB auroraSample(uint8_t z, uint16_t iGlobal, uint8_t intensity);
static inline uint8_t auroraHolesMask(uint8_t z, uint16_t iGlobal);

static constexpr char BUILD_TAG[] = "v2a0c1";

enum DFPhase : uint8_t;
struct DFState;

struct AuroraLayer {
  uint16_t phaseA;
  uint16_t phaseB;
  uint16_t width;
};

struct AuroraState {
  uint8_t  hueBase;
  uint16_t pulsPhase;
  uint16_t zoneOffset;
  AuroraLayer layers[3];
  uint16_t seed;
  uint32_t lastMs;
};

// Enhanced Plasma Effect Parameters (v2a0b3+)
// Implements dynamic wave sources with orbital motion and multi-octave noise
// for realistic electromagnetic plasma appearance inspired by FastLED Plasma Waves
struct PlasmaParams {
  float   time_scale;          // Overall animation speed multiplier
  float   noise_intensity;     // Noise field density/scale
  float   noise_amplitude;     // Noise contribution to wave sum
  uint8_t time_bitshift;       // Time scaling for noise evolution
  uint8_t hue_offset;          // Base hue for color palette
  float   brightness;          // Overall brightness multiplier
  float   wave_turbulence;     // Wave interference intensity (0.4-1.2)
  float   source_orbit_speed;  // Orbital motion speed of wave sources (0.12-0.40)
  float   color_cycle_speed;   // Color palette cycling speed (0.08-0.30)
  float   noise_damping;       // Low-speed noise damping factor (0.0-1.0, lower = more damping)
};

struct RingCoord {
  float x;
  float y;
};

static AuroraState gAurora[3] = {0};
#include <Arduino.h>
#include <pgmspace.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include <math.h>
#include <stdarg.h>
#include <esp_system.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "prefs_keys.h"

// === INSERTION POINT: TYPES_GUARD_ABOVE ===
#ifndef PP_ENABLED
#define PP_ENABLED 1
#endif

#if PP_ENABLED

#ifndef Z1_START
  #define Z1_START 0
  #define Z1_END   15

#ifndef NUM_LEDS
#define NUM_LEDS 25
#endif

  #define Z2_START 16
  #define Z2_END   23
  #define Z3_START 24
  #define Z3_END   24
#endif

#ifndef Z3_RADIUS_MM
  #define Z3_RADIUS_MM 0
  #define Z2_RADIUS_MM 12
  #define Z1_RADIUS_MM 32
#endif

static const uint8_t PP_INTENSITY_MAX = 255;
static const uint8_t  PP_INTENSITY_MIN      = 24;
static const uint8_t  PP_MIN_BRIGHTEN_WH    = 18;
static const uint8_t  PP_WHITE_LUMA_THRESH  = 180;
static const uint16_t PP_ZONE_WIDTH_MS      = 140;
static const uint16_t PP_BASE_PERIOD_MS     = 600;
static const float    PP_RADIAL_SPEED_MM_MS = 0.343f; 
static const uint16_t PP_DWELL_Z3_MS = 70;

static const uint8_t PP_ZONE_GAIN_Z3 = 255;
static const uint8_t PP_ZONE_GAIN_Z2 = 220;
static const uint8_t PP_ZONE_GAIN_Z1 = 250;

struct PP_State {
  uint32_t startMs;
  uint32_t nextMs;
  uint16_t periodMs;
  bool     active;
  uint16_t widthMs;
  uint16_t dwellZ3Ms;
  uint16_t peakMax;
  uint16_t gapMs;
};
static PP_State PP = {0, 0, PP_BASE_PERIOD_MS, false, PP_ZONE_WIDTH_MS, PP_DWELL_Z3_MS, PP_INTENSITY_MAX, 16};

static inline uint8_t PP_lumaMax(const CRGB& c){
  uint8_t m = c.r; if (c.g > m) m = c.g; if (c.b > m) m = c.b; return m;
}

static inline CRGB PP_lightenWhiteKiss(const CRGB& base, uint8_t amt){
  CRGB out = base;
  out.r = qadd8(out.r, scale8(255 - out.r, amt));
  out.g = qadd8(out.g, scale8(255 - out.g, amt));
  out.b = qadd8(out.b, scale8(255 - out.b, amt));
{ 
  uint8_t r = out.r, g = out.g, b = out.b;
  uint8_t hiCnt = (r > 230) + (g > 230) + (b > 230);
  if (hiCnt >= 2) {
    uint8_t which = 0; uint8_t minc = r;
    if (g < minc) { minc = g; which = 1; }
    if (b < minc) { minc = b; which = 2; }
    uint8_t lift = (uint8_t)constrain((int)amt/2 + 48, 16, 96);
    if (PP.periodMs < 80) lift = scale8(lift, 128);
    if (PP.periodMs < 60) lift = scale8(lift,  64);
    uint8_t* p = (which == 0 ? &out.r : (which == 1 ? &out.g : &out.b));
    uint8_t headroom = 255 - *p;
    if (lift > headroom) lift = headroom;
    *p = *p + lift;
  }
}
  return out;
}

// Note: PP_applyZoneWindow_from is the active implementation used by PP_draw.
// Legacy PP_applyZoneWindow (in-place variant) has been removed as it was unused.
static inline void PP_applyZoneWindow_from(const CRGB* __src, CRGB* __dst, uint16_t i0, uint16_t i1, uint32_t nowMs,
                                           uint32_t onsetMs, uint8_t zoneGain){
  if (nowMs < onsetMs) return;
  uint32_t dt = nowMs - onsetMs;
  uint16_t __pp_winWidth = PP.widthMs;
  /* Z2 micro-dwell removed in v4c9w7 */
{
    const uint16_t __pp_visibleMinMs = 14;
    if (PP.periodMs < 120 && __pp_winWidth < __pp_visibleMinMs) {
      uint32_t __pp_w = __pp_visibleMinMs;
      if (__pp_w > PP.periodMs) __pp_w = PP.periodMs;
      __pp_winWidth = (uint16_t)__pp_w;
    }
  }
  if (dt >= __pp_winWidth) return;

  uint16_t width = PP.widthMs;
  uint16_t dwell = (zoneGain == PP_ZONE_GAIN_Z3) ? PP.dwellZ3Ms : 0;
  if (dwell > width) dwell = width;
  uint16_t halfSpan = (uint16_t)((width > dwell) ? ((width - dwell) >> 1) : 0);
  uint16_t riseMs = halfSpan;
  uint16_t fallMs = halfSpan;

  float envF = 0.0f;
  if (riseMs == 0 && fallMs == 0){
    envF = 1.0f;
  } else if (dt < riseMs){
    float x = (riseMs ? (float)dt / (float)riseMs : 1.0f);
    envF = x*x*(3.0f - 2.0f*x);
  } else if (dt < (uint32_t)riseMs + dwell){
    envF = 1.0f;
  } else if (dt < (uint32_t)riseMs + dwell + fallMs){
    uint32_t d2 = dt - riseMs - dwell;
    float x = (fallMs ? (float)d2 / (float)fallMs : 1.0f);
    float s = x*x*(3.0f - 2.0f*x);
    envF = 1.0f - s;
  } else {
    envF = 0.0f;
  }
  {
    const uint16_t __pp_visibleMinMs = 14;
    uint16_t __pp_effWidth = PP.widthMs;
    if (PP.periodMs < 120 && __pp_effWidth < __pp_visibleMinMs) __pp_effWidth = __pp_visibleMinMs;
    if (dt >= PP.widthMs && dt < __pp_effWidth) { envF = 1.0f; }
  }
  uint16_t peak = PP.peakMax;
  uint16_t env = (uint16_t)((envF <= 0.0f) ? 0 : (envF >= 1.0f ? peak : (uint16_t)(envF * peak)));
  uint8_t amt = (uint8_t)constrain((int)((env * zoneGain) >> 8), PP_INTENSITY_MIN, 255);

  uint16_t feather = (uint16_t)max<uint16_t>(1, (uint16_t)(PP.widthMs / 10));
  float fscale = 0.0f;
  if (dt < feather) fscale = (float)dt / (float)feather;
  else if (dt > PP.widthMs - feather) fscale = (float)(PP.widthMs - dt) / (float)feather;
  if (fscale < 0.0f) fscale = 0.0f;
  if (fscale > 1.0f) fscale = 1.0f;

  uint8_t floorAmt = (uint8_t)((float)PP_MIN_BRIGHTEN_WH * fscale + 0.5f);
if (PP.periodMs < 60) floorAmt = 0;

  for (uint16_t i = i0; i <= i1; ++i){
    CRGB base = __src[i];
    CRGB out  = PP_lightenWhiteKiss(base, amt);
    if (PP_lumaMax(base) >= PP_WHITE_LUMA_THRESH && floorAmt){
      out.r = qadd8(out.r, floorAmt);
      out.g = qadd8(out.g, floorAmt);
      out.b = qadd8(out.b, floorAmt);
    }
    __dst[i] = out;
  }
}

static inline void PP_draw(const CRGB* __src, CRGB* __dst, uint32_t nowMs){

float sp = PP_RADIAL_SPEED_MM_MS;
if (sp < 0.001f) sp = 0.343f;
uint32_t dZ3Z2 = (uint32_t)((float)(Z2_RADIUS_MM - Z3_RADIUS_MM) / sp + 0.5f);
uint32_t dZ3Z1 = (uint32_t)((float)(Z1_RADIUS_MM - Z3_RADIUS_MM) / sp + 0.5f);
if ((int32_t)dZ3Z2 < 8) dZ3Z2 = 8;
if ((int32_t)dZ3Z1 < 16) dZ3Z1 = 16;

const uint32_t tZ3 = PP.startMs;
  const uint32_t tZ2 = PP.startMs + PP.gapMs;
  const uint32_t tZ1 = PP.startMs + (PP.gapMs * 2);

  #if (Z3_END >= Z3_START)
    PP_applyZoneWindow_from(__src, __dst, Z3_START, Z3_END, nowMs, tZ3, PP_ZONE_GAIN_Z3);
  #endif
  #if (Z2_END >= Z2_START)
    { 
      float __k2 = (3000.0f - (float)PP.periodMs) / 2960.0f; if (__k2 < 0.0f) __k2 = 0.0f; if (__k2 > 1.0f) __k2 = 1.0f;
      float __x2 = (__k2 - 0.90f) / 0.10f; if (__x2 < 0.0f) __x2 = 0.0f; if (__x2 > 1.0f) __x2 = 1.0f;
      float __hs2 = __x2*__x2*(3.0f - 2.0f*__x2);
      float __z2WidthFrac = 0.24f + (0.20f - 0.24f) * __hs2;
      uint16_t __widthZ2 = (uint16_t)((float)PP.periodMs * __z2WidthFrac + 0.5f); if (__widthZ2 < 1) __widthZ2 = 1;
      if ((uint32_t)__widthZ2 + 6u > PP.periodMs) __widthZ2 = (uint16_t)max(1, (int)PP.periodMs - 6);
      uint16_t __saveW2 = PP.widthMs; PP.widthMs = __widthZ2;
      PP_applyZoneWindow_from(__src, __dst, Z2_START, Z2_END, nowMs, tZ2, PP_ZONE_GAIN_Z2);
      PP.widthMs = __saveW2;
    }
    
  #endif
  #if (Z1_END >= Z1_START)
    
    float __k = (3000.0f - (float)PP.periodMs) / 2960.0f;
    if (__k < 0.0f) __k = 0.0f; if (__k > 1.0f) __k = 1.0f;
    float __x = (__k - 0.90f) / 0.10f; if (__x < 0.0f) __x = 0.0f; if (__x > 1.0f) __x = 1.0f;
    float __hs = __x*__x*(3.0f - 2.0f*__x);
    float __z1WidthFrac = 0.24f + (0.16f - 0.24f) * __hs;
    uint16_t __widthZ1 = (uint16_t)((float)PP.periodMs * __z1WidthFrac + 0.5f);
    if (__widthZ1 < 1) __widthZ1 = 1;
    if ((uint32_t)__widthZ1 + 10u > PP.periodMs) __widthZ1 = (uint16_t)max(1, (int)PP.periodMs - 10);
    
    uint8_t __gainZ1_eff = (uint8_t)( (int)PP_ZONE_GAIN_Z1 + (int)lroundf( (220 - (int)PP_ZONE_GAIN_Z1) * __hs ) );
    uint16_t __saveW = PP.widthMs;
    PP.widthMs = __widthZ1;
    PP_applyZoneWindow_from(__src, __dst, Z1_START, Z1_END, nowMs, tZ1, __gainZ1_eff);
    PP.widthMs = __saveW;
    
  #endif

}

static inline void PP_applyPowerPulseOverlay(CRGB* leds, uint32_t nowMs){
  CRGB __pp_src[NUM_LEDS]; for (int i=0;i<NUM_LEDS;i++) __pp_src[i] = leds[i];
  CRGB __pp_dst[NUM_LEDS]; for (int i=0;i<NUM_LEDS;i++) __pp_dst[i] = __pp_src[i];

  if (nowMs >= PP.nextMs){
    uint16_t sp = 10; 
    uint8_t inten = 0;
    if (!PP_pickTempoOwner(sp, inten)) { sp = 10; inten = 0; }
    float s = (float)(sp - 10) / 990.0f; if (s < 0) s = 0; if (s > 1) s = 1;
    const float lnHi = logf(3000.0f), lnLo = logf( 40.0f );
    float lnP = lnHi + (lnLo - lnHi) * s;
    uint16_t period = (uint16_t)roundf(expf(lnP));
    if (period < 40) period = 40;

    if (period < 50) { FastLED.setDither(false); }

    float k = (3000.0f - (float)period) / (3000.0f - 40.0f);
    if (k < 0.0f) k = 0.0f; if (k > 1.0f) k = 1.0f;

    uint16_t width = (uint16_t)max(6, (int)roundf(period * 0.22f));
    uint16_t dwell = (uint16_t)max(60, (int)(width * 0.35f));
    uint16_t gap = (uint16_t)max(8, (int)(width * (0.70f + 0.35f * k))); 
    uint16_t maxGap = (period > width) ? (uint16_t)((period - width) / 2) : 0;
    if (gap > maxGap) gap = maxGap;

    uint16_t peak = (uint16_t)min( (int)PP_INTENSITY_MAX, (int)map(inten, 0, 255, PP_INTENSITY_MIN, PP_INTENSITY_MAX) );

    PP.startMs  = nowMs;
    PP.periodMs = period;
    PP.widthMs  = width;
    PP.dwellZ3Ms= dwell;
    PP.peakMax  = peak;
    PP.gapMs    = gap;
    PP.nextMs   = PP.startMs + PP.periodMs;
    PP.active   = true;
  }

  const uint32_t lastWindowEnd = PP.startMs + (PP.gapMs * 2) + PP.widthMs;
  if (PP.active){
    PP_draw(__pp_src, __pp_dst, nowMs);
    if (nowMs >= lastWindowEnd) PP.active = false;
  }

  for (int i=0;i<NUM_LEDS;i++) leds[i] = __pp_dst[i];
}

#endif

static void auroraUpdateAndOverlay();
static void lightningUpdateAndOverlay();
static void overloadUpdateAndOverlay();
static const char s0[] PROGMEM = "";
static const char s1[] PROGMEM = "text/plain";
static const char s2[] PROGMEM = "range";
static const char s3[] PROGMEM = "arcReactor";
static const char s4[] PROGMEM = "colorA";
static const char s5[] PROGMEM = "autoDF";
static const char s6[] PROGMEM = "master";
static const char s7[] PROGMEM = "wifiIdleAutoOff";
static const char s8[] PROGMEM = "zones";
static const char s9[] PROGMEM = "colorB";
static const char s10[] PROGMEM = "speedA";
static const char s11[] PROGMEM = "speedB";
static const char s12[] PROGMEM = "plain";
static const char s13[] PROGMEM = "dfThresholdV";
static const char s14[] PROGMEM = "effectA";
static const char s15[] PROGMEM = "effectB";
static const char s16[] PROGMEM = "intensityA";
static const char s17[] PROGMEM = "intensityB";
static const char s18[] PROGMEM = "preset";
static const char s19[] PROGMEM = "simVbat";
static const char s20[] PROGMEM = "10";
static const char s21[] PROGMEM = "1000";
static const char s22[] PROGMEM = "activePreset";
static const char s23[] PROGMEM = "lblEffect";
static const char s24[] PROGMEM = "lblIntensity";
static const char* const STRS[] PROGMEM = {
  s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18, s19, s20, s21, s22, s23, s24
};

/* === Gamma 2.2 LUT (v4c7b6) === */
static const uint8_t GAMMA22_LUT[256] PROGMEM = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,8,8,8,9,9,9,10,10,11,11,11,12,12,13,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,22
  // (full table retained in original repository)
};

static inline uint8_t gamma22_lut(uint8_t x){
  return pgm_read_byte(&GAMMA22_LUT[x]);
}

static inline uint16_t clampSpeed(uint16_t v){ return constrain(v, 10, 1000); }

#define DATA_PIN    6
#define BUTTON_PIN  3
#define VBAT_PIN    A2

#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

static const char* AP_SSID = "ArcReactorMK1";
static const char* AP_PASS = "arcreactor";

static const float FLICKER_START_V = 3.60f;
static const float FULL_OFF_V      = 3.51f;
static const unsigned long AUTO_SLEEP_GRACE_MS = 10UL * 60UL * 1000UL;

#ifndef LBF_HYSTERESIS_MV
#define LBF_HYSTERESIS_MV 0
#endif

static const uint16_t BAT_SAMPLES_PER_WINDOW = 16;
static const uint16_t BAT_SAMPLE_INTERVAL_MS = 50;
struct BatterySampler {
  uint32_t acc_mV = 0;
  uint16_t count  = 0;
  uint32_t lastMs = 0;
} batt;


enum Effect : uint8_t {
  E_Off = 0,
  E_On,
  E_Candle,
  E_ArcFlicker,
  E_PowerPulse,
  E_OverloadSurge,
  E_WavePulse,
  E_Lightning,
  E_Aurora
};

static const char _E0[] PROGMEM = "Off";
static const char _E1[] PROGMEM = "On";
static const char _E2[] PROGMEM = "Plasma";
static const char _E3[] PROGMEM = "ArcFlicker";
static const char _E4[] PROGMEM = "PowerPulse";
static const char _E5[] PROGMEM = "HaloBreath";
static const char _E6[] PROGMEM = "Breathe";
static const char _E7[] PROGMEM = "Lightning";
static const char _E8[] PROGMEM = "Aurora";
static const char* const EFFECT_NAMES[] PROGMEM = {
  _E0,_E1,_E2,_E3,_E4,_E5,_E6,_E7,_E8
};

static Effect effectFromCString(const char* s){
  if (!s) return E_Off;
  char buf[16];
  for (uint8_t i=0;i<9;i++){
    strncpy_P(buf, (PGM_P)pgm_read_ptr(&EFFECT_NAMES[i]), sizeof(buf)-1);
    buf[sizeof(buf)-1]='\0';
    if (strcmp(s, buf)==0) return (Effect)i;
  }
  return E_Off;
}
static void effectName(Effect e, char* out, size_t outlen){
  if (e >= 9) e = E_Off;
  strncpy_P(out, (PGM_P)pgm_read_ptr(&EFFECT_NAMES[e]), outlen-1);
  out[outlen-1]='\0';
}

static CRGB leds[NUM_LEDS];
static uint8_t osGlow[NUM_LEDS] = {0};
static RingCoord gRingLUT[NUM_LEDS];
static bool      gRingLUTInited = false;
static WebServer server(80);
static DNSServer dnsServer;
static Preferences prefs;

struct Zone {
  uint8_t startLed;
  uint8_t endLed;
  uint8_t effectA;
  uint8_t effectB;
  CRGB   colorA;
  CRGB   colorB;
  uint16_t speedA;
  uint16_t speedB;
  uint8_t  intensityA;
  uint8_t  intensityB;
} zones[3];

static inline bool PP_pickTempoOwner(uint16_t &outSp, uint8_t &outInt){
  if (zones[0].effectA == E_PowerPulse){ outSp = zones[0].speedA; outInt = zones[0].intensityA; return true; }
  if (zones[0].effectB == E_PowerPulse){ outSp = zones[0].speedB; outInt = zones[0].intensityB; return true; }
  if (zones[1].effectA == E_PowerPulse){ outSp = zones[1].speedA; outInt = zones[1].intensityA; return true; }
  if (zones[1].effectB == E_PowerPulse){ outSp = zones[1].speedB; outInt = zones[1].intensityB; return true; }
  if (zones[2].effectA == E_PowerPulse){ outSp = zones[2].speedA; outInt = zones[2].intensityA; return true; }
  if (zones[2].effectB == E_PowerPulse){ outSp = zones[2].speedB; outInt = zones[2].intensityB; return true; }
  return false;
}

static inline bool powerPulseAnyZoneSelected(){
  for (int z=0; z<3; ++z){
    if (zones[z].effectA == E_PowerPulse || zones[z].effectB == E_PowerPulse) return true;
  }
  return false;
}

static uint8_t masterBrightness = 128;
static bool    autoDFEnabled    = false;

static bool  wifiIdleAutoOff = true;
static float batteryVoltage = 0.0f;

static float dfThresholdV = 3.60f;
static bool  simVbatEnabled = false;
static float simVbat = 3.60f;

static unsigned long wakeMillis = 0;
static unsigned long simLowSinceMs = 0;
static bool allowAutoSleep = false;

static unsigned long buttonDownAt = 0;
static const unsigned long BTN_DEBOUNCE_MS = 25;
static const unsigned long BTN_DBL_MS = 600;
static int lastBtnState = HIGH;
static unsigned long lastBtnChange = 0;
static unsigned long shortPressStarted = 0;
static unsigned long lastReleaseMs = 0;
static uint8_t clickCount = 0;
static const unsigned long BUTTON_HOLD_MS = 1100;

static String favHex[9] = {
  "#FF6A00","#00C8FF","#FFFFFF","#00FFAA","#FF00FF",
  "#FFA500","#00FF00","#0000FF","#FFFF00"
};

static int8_t activePreset = -1;

static const int DEBUG_LINES = 12;
static char debugLog[DEBUG_LINES][64];
static int debugIdx = 0;
static void addDebug(const char* s){
  uint32_t sec = millis()/1000UL;
  int i = debugIdx % DEBUG_LINES;
  snprintf(debugLog[i], sizeof(debugLog[i]), "%lus: %s", (unsigned long)sec, s ? s : "");
  debugIdx++;
}
static void addDebugf(const char* fmt, ...){
  char tmp[48];
  va_list ap; va_start(ap, fmt);
  vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  addDebug(tmp);
}

static String colorToHex(const CRGB &c);
static uint16_t sliderToPeriod(uint16_t s);
static void sampleBattery();
static void applyZoneEffects(const Zone &z);
static void applyAllEffects();
static void loadPrefs();

static void savePrefsFromJSON(const ArduinoJson::JsonObject &root);
static void enterDeepSleepNow(const char* reason);
static void sendStatusJSON();
static void loadFavorites();
static void saveFavorite(uint8_t idx, const String &hex);

static String colorToHex(const CRGB &c) {
  char b[8];
  snprintf(b, sizeof(b), "#%02X%02X%02X", c.r, c.g, c.b);
  return String(b);
}
static uint16_t sliderToPeriod(uint16_t s){
  s = constrain(s,10,1000);
  return (uint16_t)map((int)s,10,1000,4000,200);
}
static void sampleBattery() {
  uint32_t now = millis();
  if (now - batt.lastMs < BAT_SAMPLE_INTERVAL_MS) return;
  batt.lastMs = now;

  uint32_t mv = analogReadMilliVolts(VBAT_PIN);
  batt.acc_mV += mv;
  batt.count++;

  if (batt.count >= BAT_SAMPLES_PER_WINDOW) {
    float avg_mV = (float)batt.acc_mV / (float)batt.count;
    float v = 2.0f * avg_mV / 1000.0f;
    batteryVoltage = constrain(v, 0.0f, 5.5f);
    batt.acc_mV = 0;
    batt.count  = 0;
  }
}

static inline CRGB withIntensity(CRGB c, uint8_t intensity){
  if (intensity < 255) c.nscale8_video(intensity);
  return c;
}
struct FrameSeeds { uint16_t s1, s2; } gSeeds;
static inline void refreshSeeds(){ gSeeds.s1 = (uint16_t)random16(); gSeeds.s2 = (uint16_t)random16(); }

static void ensureRingLUTInited(){
  if (gRingLUTInited) return;
  const float cx  = 0.5f;
  const float cy  = 0.5f;
  const float rad = 0.45f;
  const float tau = 6.28318530718f;
  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    float a = tau * (float)i / (float)NUM_LEDS;
    gRingLUT[i].x = cx + rad * cosf(a);
    gRingLUT[i].y = cy + rad * sinf(a);
  }
  gRingLUTInited = true;
}

static PlasmaParams makePlasmaParams(uint16_t period, uint8_t intensity, uint16_t seedMix, uint8_t masterBright){
  PlasmaParams p;

  float pMs = (float)period;
  if (pMs < 200.0f)   pMs = 200.0f;
  if (pMs > 4000.0f)  pMs = 4000.0f;

  float tNorm = (4000.0f - pMs) / (4000.0f - 200.0f);
  if (tNorm < 0.0f) tNorm = 0.0f;
  if (tNorm > 1.0f) tNorm = 1.0f;

  float intenNorm = (float)intensity / 255.0f;
  if (intenNorm < 0.0f) intenNorm = 0.0f;
  if (intenNorm > 1.0f) intenNorm = 1.0f;

  const float TIME_SCALE_SLOW = 0.08f;
  const float TIME_SCALE_FAST = 3.20f;
  p.time_scale = TIME_SCALE_SLOW + (TIME_SCALE_FAST - TIME_SCALE_SLOW) * tNorm;

  // Enhanced noise parameters for more detailed plasma texture
  p.noise_amplitude = 0.15f + 1.20f * intenNorm;
  p.noise_intensity = 0.30f + 1.20f * intenNorm;

  p.time_bitshift = 5;
  p.hue_offset    = (uint8_t)(seedMix & 0xFF);

  float brightNorm = (float)masterBright / 255.0f;
  p.brightness = 1.6f * brightNorm;
  
  // New enhanced parameters
  p.wave_turbulence = 0.4f + 0.8f * intenNorm;  // Higher intensity = more turbulent waves
  p.source_orbit_speed = 0.12f + 0.28f * tNorm; // Faster orbital motion at higher speeds
  p.color_cycle_speed = 0.08f + 0.22f * tNorm;  // Color palette cycling speed
  
  // Low-speed noise damping for smooth, molten motion without jitter
  // At slow speeds (tNorm near 0), apply strong damping (0.2-0.3)
  // At fast speeds (tNorm near 1), no damping (1.0)
  // Use exponential curve for smooth transition
  p.noise_damping = 0.25f + 0.75f * (tNorm * tNorm);  // Quadratic: more damping at low speeds
  
  return p;
}

// =============================================================================
// FBM (Fractional Brownian Motion) Noise Helper
// =============================================================================
// Multi-octave noise generator using FastLED's inoise16 for plasma displacement
// Parameters:
//   - octaves: number of noise layers (2-3 recommended for ESP32 performance)
//   - gain: amplitude falloff per octave (0.55 typical)
//   - lacunarity: frequency multiplier per octave (2.0 typical)
// Performance: ~150-250µs per call on ESP32 @ 240MHz with 3 octaves
static inline float fbm_noise(uint32_t x, uint32_t y, uint32_t z, uint8_t octaves = 3, float gain = 0.55f, float lacunarity = 2.0f) {
  float sum = 0.0f;
  float amplitude = 1.0f;
  float frequency = 1.0f;
  float max_value = 0.0f;  // Used for normalizing result to [-1, 1]
  
  for (uint8_t i = 0; i < octaves; i++) {
    uint32_t nx = (uint32_t)(x * frequency);
    uint32_t ny = (uint32_t)(y * frequency);
    uint32_t nz = (uint32_t)(z + i * 5000);  // Offset Z for octave variety
    
    uint16_t n16 = inoise16(nx, ny, nz);
    float n = ((float)n16 - 32768.0f) / 32768.0f;  // Normalize to [-1, 1]
    
    sum += n * amplitude;
    max_value += amplitude;
    
    amplitude *= gain;
    frequency *= lacunarity;
  }
  
  // Normalize to [-1, 1] range
  return sum / max_value;
}

// =============================================================================
// Plasma Wave to Color Mapping with Nonlinear Contrast & Specular Highlights
// =============================================================================
// Maps wave interference value to HSV color with advanced color dynamics
// Features:
//   - Nonlinear contrast stretch using powf for enhanced peak visibility
//   - Specular highlight path for bright peaks (threshold-based)
//   - Saturation boost at peaks for more vivid plasma appearance
//   - Hue cycling based on wave patterns and intensity
// Performance: ~50-80µs per call on ESP32 @ 240MHz
static CRGB plasmaMapWaveToColor(float wave_value, float intensityNorm, const PlasmaParams &params){
  float normalized = (wave_value + 4.0f) / 8.0f;
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;

  // Clamp intensityNorm
  if (intensityNorm < 0.0f) intensityNorm = 0.0f;
  if (intensityNorm > 1.0f) intensityNorm = 1.0f;

  // === Nonlinear Contrast Stretch ===
  // Apply power function to make peaks stand out more dramatically
  // Power range: 0.7-0.9 based on intensity (higher intensity = more linear)
  float contrastPower = 0.70f + (0.20f * intensityNorm);
  float contrastStretched = powf(normalized, contrastPower);
  
  uint8_t baseHue = params.hue_offset;

  // Enhanced hue modulation for electromagnetic spectrum feel
  // Map wave peaks to different colors in the electromagnetic spectrum
  float waveNorm = wave_value;
  if (waveNorm < -4.0f) waveNorm = -4.0f;
  if (waveNorm > 4.0f)  waveNorm =  4.0f;
  waveNorm *= 0.25f; // normalize to -1..+1
  
  // Map intensity slider to hue jitter amplitude.
  // Intensity 0 -> ±5 degrees, Intensity 255 -> ±20 degrees.
  // FastLED hue units: 0..255 == 0..360 degrees.
  const float MIN_SHIFT_DEG = 5.0f;   // degrees at intensity = 0
  const float MAX_SHIFT_DEG = 20.0f;  // degrees at intensity = 1 (255)
  const float HUE_UNITS_PER_DEG = 255.0f / 360.0f; // convert degrees -> 0..255 hue units

  // Linear interpolation of shift amplitude by intensityNorm (0..1)
  float shiftDeg = MIN_SHIFT_DEG + (MAX_SHIFT_DEG - MIN_SHIFT_DEG) * intensityNorm;
  float shiftHueUnits = shiftDeg * HUE_UNITS_PER_DEG;

  // waveNorm is -1..+1, so multiply to get signed spectral shift in hue units
  float spectralShift = waveNorm * shiftHueUnits;
  
  // Add wave-based color pulsing for more dynamic appearance
  float colorPulse = sinf(contrastStretched * 3.14159f * 2.0f) * 18.0f;
  
  // Intensity affects color range - higher intensity = broader spectrum
  float intensityColorSpread = 8.0f + (28.0f - 8.0f) * intensityNorm;
  float intensityJitter = waveNorm * intensityColorSpread;
  
  // Combine all color modulations
  float totalHueOffset = spectralShift + colorPulse + intensityJitter;
  int16_t hueOffset16 = (int16_t)(totalHueOffset + (totalHueOffset >= 0.0f ? 0.5f : -0.5f));
  
  uint8_t hue = baseHue + hueOffset16;

  // === Enhanced Saturation with Peak Boost ===
  float waveIntensity = fabsf(wave_value);
  float satBase = 220.0f + waveIntensity * 35.0f; // Higher base saturation
  
  // Boost saturation at peaks for more vibrant plasma
  if (contrastStretched > 0.75f) {
    float peakSat = (contrastStretched - 0.75f) / 0.25f;
    satBase += peakSat * 25.0f * intensityNorm;
  }
  
  // Add intensity-based saturation boost for more vibrant colors
  satBase += intensityNorm * 20.0f;
  if (satBase > 255.0f) satBase = 255.0f;
  uint8_t sat = (uint8_t)(satBase + 0.5f);

  // === Enhanced Brightness with Specular Highlights ===
  float valF = contrastStretched * 255.0f * params.brightness;
  
  // Add brightness peaks at wave maxima for "plasma cell" effect
  float peakBoost = 1.0f;
  if (contrastStretched > 0.7f) {
    float peakness = (contrastStretched - 0.7f) / 0.3f;
    peakBoost = 1.0f + (peakness * peakness * 0.35f * intensityNorm);
  }
  valF *= peakBoost;
  
  // === Specular Highlight Path ===
  // For very bright peaks (>0.85), add a specular highlight
  // This creates sharp, bright spots that make the plasma "pop"
  const float SPECULAR_THRESHOLD = 0.85f;
  if (contrastStretched > SPECULAR_THRESHOLD) {
    float specularStrength = (contrastStretched - SPECULAR_THRESHOLD) / (1.0f - SPECULAR_THRESHOLD);
    specularStrength = powf(specularStrength, 2.0f);  // Quadratic for sharp falloff
    
    // Desaturate and brighten at specular peaks
    float specularAmount = specularStrength * intensityNorm * 0.6f;
    sat = (uint8_t)((float)sat * (1.0f - specularAmount) + 0.5f);
    valF += specularAmount * 80.0f;  // Specular boost
  }
  
  if (valF < 0.0f) valF = 0.0f;
  if (valF > 255.0f) valF = 255.0f;
  uint8_t val = (uint8_t)(valF + 0.5f);

  return CHSV(hue, sat, val);
}

// =============================================================================
// Plasma Sample with FBM Displacement, Bloom, and Center LED Enhancement
// =============================================================================
// Generates plasma effect color for a specific LED position
// Features:
//   - 4-source wave interference with orbital motion
//   - FBM noise (3 octaves) for coordinate displacement and amplitude modulation
//   - Smooth spatial calculations using sinf/sqrtf
//   - Small neighbor-based bloom overlay for highlights
//   - Center LED (zone 3) special contribution for distinctive glow
// Performance: ~400-600µs per LED on ESP32 @ 240MHz
// Tunable parameters (for performance):
//   - Reduce FBM octaves from 3 to 2 for ~25% speedup
//   - Reduce bloom radius for faster frame rates
static CRGB plasmaSample(uint16_t iGlobal,
                         uint32_t nowMs,
                         uint16_t period,
                         uint8_t intensity,
                         uint16_t seedMix,
                         uint8_t masterBright)
{
  ensureRingLUTInited();

  if (iGlobal >= NUM_LEDS) {
    iGlobal = NUM_LEDS - 1;
  }
  const RingCoord &coord = gRingLUT[iGlobal];

  PlasmaParams params = makePlasmaParams(period, intensity, seedMix, masterBright);

  float time_scaled = (float)nowMs * params.time_scale * 0.001f;
  float orbit_time = (float)nowMs * params.source_orbit_speed * 0.001f;

  // === FBM Noise for Coordinate Displacement ===
  // Use FBM to displace sampling coordinates for organic plasma flow
  // This creates swirling, turbulent patterns
  uint32_t noise_time = nowMs << params.time_bitshift;
  uint32_t nx = (uint32_t)(coord.x * 65535.0f * params.noise_intensity);
  uint32_t ny = (uint32_t)(coord.y * 65535.0f * params.noise_intensity);
  
  // FBM with 3 octaves (can reduce to 2 for better performance)
  float fbm_x = fbm_noise(nx, ny, noise_time, 3, 0.55f, 2.0f);
  float fbm_y = fbm_noise(nx + 10000, ny + 20000, noise_time + 5000, 3, 0.55f, 2.0f);
  
  // Apply low-speed damping to reduce jitter at slow speeds
  fbm_x *= params.noise_damping;
  fbm_y *= params.noise_damping;
  
  // Apply displacement to coordinates
  float displaced_x = coord.x + fbm_x * params.noise_amplitude * 0.15f;
  float displaced_y = coord.y + fbm_y * params.noise_amplitude * 0.15f;
  
  struct WaveSource {
    float x;
    float y;
    float frequency;
    float amplitude;
    float phase_speed;
    float orbit_radius;   // Radius of orbital motion
    float orbit_phase;    // Phase offset for orbital motion
  };

  // Enhanced wave sources with orbital motion for dynamic plasma
  WaveSource sources[4] = {
    {0.5f, 0.5f, 1.2f, 1.0f, 0.9f, 0.0f,   0.0f},        // Center source (stationary)
    {0.0f, 0.0f, 1.8f, 0.9f, 1.3f, 0.35f,  0.0f},        // Orbiting source 1
    {1.0f, 1.0f, 1.4f, 1.1f, 0.7f, 0.30f,  2.094f},      // Orbiting source 2 (120° offset)
    {0.5f, 0.0f, 1.6f, 0.95f, 1.1f, 0.28f, 4.189f}       // Orbiting source 3 (240° offset)
  };

  // Apply orbital motion to non-center sources
  for (uint8_t i = 1; i < 4; ++i) {
    float orbit_angle = orbit_time + sources[i].orbit_phase;
    float orbit_x = cosf(orbit_angle) * sources[i].orbit_radius;
    float orbit_y = sinf(orbit_angle) * sources[i].orbit_radius;
    sources[i].x = 0.5f + orbit_x;
    sources[i].y = 0.5f + orbit_y;
  }

  // === Wave Interference Calculation ===
  float wave_sum = 0.0f;
  float wave_product = 1.0f; // For interference patterns
  
  for (uint8_t i = 0; i < 4; ++i) {
    float dx = displaced_x - sources[i].x;
    float dy = displaced_y - sources[i].y;
    float distance = sqrtf(dx * dx + dy * dy);

    float wave_phase = distance * sources[i].frequency * 6.28318f // 2*PI for full wave
                     + time_scaled * sources[i].phase_speed;
    float wave = sinf(wave_phase) * sources[i].amplitude;
    wave_sum += wave;
    
    // Accumulate for interference pattern (creates visible plasma cells)
    wave_product *= (1.0f + wave * 0.3f);
  }
  
  // Add interference pattern to create more visible structure
  wave_sum += (wave_product - 1.0f) * params.wave_turbulence;

  // === FBM Amplitude Modulation ===
  // Additional FBM layer for amplitude/intensity variation
  float fbm_amplitude = fbm_noise(nx * 2, ny * 2, noise_time, 2, 0.6f, 2.0f);
  // Apply low-speed damping to amplitude modulation as well
  fbm_amplitude *= params.noise_damping;
  wave_sum += fbm_amplitude * params.noise_amplitude;

  // === Center LED Enhancement ===
  // Add special contribution for center LED (zone 3) so it glows distinctively
  // This creates a focal point in the center of the arc reactor
  if (iGlobal == Z3_START) {
    // Center LED gets a pulsing, radial energy contribution
    float center_pulse = sinf(time_scaled * 1.5f) * 0.3f;
    float center_noise = fbm_noise(nx, ny, noise_time * 2, 2, 0.7f, 2.0f) * 0.2f;
    // Apply damping to center noise as well for smooth low-speed motion
    center_noise *= params.noise_damping;
    wave_sum += (center_pulse + center_noise + 0.5f) * 0.8f;
  }

  // Intensity controls the hue-jitter band width and overall turbulence
  float intensityNorm = (float)intensity / 255.0f;
  if (intensityNorm < 0.0f) intensityNorm = 0.0f;
  if (intensityNorm > 1.0f) intensityNorm = 1.0f;

  // === Base Color Calculation ===
  CRGB base_color = plasmaMapWaveToColor(wave_sum, intensityNorm, params);
  
  // === Bloom Overlay ===
  // Add small neighbor-based bloom for specular highlights to "pop"
  // This simulates light bleeding from very bright spots
  // Bloom radius: 1 LED (can increase for stronger effect but at performance cost)
  // Only apply bloom if this LED is bright enough to warrant it
  const uint8_t BLOOM_THRESHOLD = 180;
  if (base_color.r > BLOOM_THRESHOLD || base_color.g > BLOOM_THRESHOLD || base_color.b > BLOOM_THRESHOLD) {
    // This LED is bright - it will contribute bloom to neighbors
    // (Actual bloom is applied in the rendering loop, this just marks it)
    // For now, we enhance the current LED slightly to prepare for bloom
    float bloom_boost = 1.08f;
    base_color.r = min(255, (int)(base_color.r * bloom_boost));
    base_color.g = min(255, (int)(base_color.g * bloom_boost));
    base_color.b = min(255, (int)(base_color.b * bloom_boost));
  }
  
  return base_color;
}

static inline CRGB effectSample(uint8_t eff, const CRGB &base,
                                uint16_t period, uint8_t intensity,
                                uint16_t i, uint16_t seedMix)
{
  switch(eff){
    case E_Off: return CRGB::Black;
    case E_On: return withIntensity(base, intensity);

    case E_Candle: {
  CHSV hsv = rgb2hsv_approximate(base);
  uint16_t seedHue = hsv.h;
  uint32_t now = millis();
  return plasmaSample(i, now, period, intensity, seedHue, masterBrightness);
}
case E_ArcFlicker:{

      uint32_t now = millis();

      uint16_t p = period;
      if (p < 200)   p = 200;
      if (p > 4000) p = 4000;
      const uint16_t reqMin = 7;
      const uint16_t reqMax = 1500;
      uint32_t msReq = reqMin + ((uint32_t)(p - 200) * (reqMax - reqMin)) / 3800U;

      uint8_t phase = (uint8_t)(((now % msReq) * 256UL) / (msReq ? msReq : 1));
      phase = (uint8_t)(phase + (uint8_t)((i * 7u) ^ (seedMix * 13u)));

      int16_t s1 = (int16_t)sin8(phase) - 128;
      int16_t s3 = (int16_t)sin8((uint8_t)(phase * 3u + (uint8_t)(17u * seedMix))) - 128;
      int16_t s5 = (int16_t)sin8((uint8_t)(phase * 5u + (uint8_t)(31u * (i & 0xFF)))) - 128;
      int16_t env = s1 + ((s3 * 90) >> 7) + ((s5 * 50) >> 7);

      int16_t grain = (int16_t)random8() - 128;
      uint8_t hfPhase = (uint8_t)(((now % 7UL) * 256UL) / 7UL);
      hfPhase = (uint8_t)(hfPhase + (uint8_t)(i * 11u + seedMix * 23u));
      int16_t hf = (int16_t)sin8(hfPhase) - 128;
      grain += (hf * 24) >> 7;

      uint16_t alpha = (uint16_t)(4000 - p);
      uint8_t mixQ8 = (uint8_t)(26 + (alpha * (218 - 26)) / 3800U);
      int16_t s = (int16_t)(((int32_t)(256 - mixQ8) * env + (int32_t)mixQ8 * grain) >> 8);

      const uint8_t A_min = 53;
      const uint8_t A_max = 240;
      uint8_t A = (uint8_t)(A_min + ((uint16_t)intensity * (uint16_t)(A_max - A_min)) / 255U);

      const uint8_t mid = 192;
      int16_t bC = (int16_t)mid + ((s * (int16_t)A) / 128);
      if (bC < 0) bC = 0; else if (bC > 255) bC = 255;

      CRGB out = base; out.nscale8_video((uint8_t)bC);
      return withIntensity(out, 255);
    }
case E_PowerPulse:{
      return base;
    }
case E_OverloadSurge:{
      return base;
    }
    break;

case E_WavePulse:{
      uint16_t breathP = (uint16_t)constrain(map((int)period, 200, 4000, 2000, 12000), 2000, 12000);
      uint32_t now = millis();
      uint32_t phaseMs = now % breathP;
      const uint8_t FLOOR_PWM = 7;

      const uint16_t inhaleMs = (uint16_t)(((uint32_t)breathP * 35) / 100);
      const uint16_t topDwellMs = (uint16_t)max((uint32_t)50, ((uint32_t)breathP * 25UL) / 1000UL);
      const uint16_t bottomDwellMs = (uint16_t)max((uint32_t)80, ((uint32_t)breathP * 50UL) / 1000UL);
      uint16_t exhaleMs = (breathP > inhaleMs + topDwellMs + bottomDwellMs) 
                          ? (breathP - inhaleMs - topDwellMs - bottomDwellMs) : 1;

      float y;
      if (phaseMs < bottomDwellMs) {
        y = 0.0f;
      } else if (phaseMs < bottomDwellMs + inhaleMs) {
        uint32_t pm = phaseMs - bottomDwellMs;
        y = (float)pm / (float)inhaleMs;
      } else if (phaseMs < (uint32_t)bottomDwellMs + inhaleMs + topDwellMs) {
        y = 1.0f;
      } else {
        uint32_t pm = phaseMs - bottomDwellMs - inhaleMs - topDwellMs;
        y = 1.0f - (float)pm / (float)exhaleMs;
      }

      float a = 0.5f + ((float)intensity / 255.0f);
      float vF = a * y;
      int v = (int)(vF * 255.0f + 0.5f);
      if (v < 0) v = 0; else if (v > 255) v = 255;

            if (v < FLOOR_PWM) v = FLOOR_PWM;
      float mixF = 0.25f * y * y * a;
      if (mixF < 0.0f) mixF = 0.0f; if (mixF > 0.40f) mixF = 0.40f;
      uint8_t mix = (uint8_t)(mixF * 255.0f + 0.5f);

      CRGB c = base;
      c = blend(c, CRGB::White, mix);
      c.nscale8((uint8_t)v);
      return withIntensity(c, 255);
    }
    break;
case E_Aurora:{
      uint8_t z = 0; if (i >= zones[1].startLed) z = (i <= zones[1].endLed ? 1 : (i >= zones[2].startLed ? 2 : 0));
      uint16_t zoneSpeed = (zones[z].effectA == E_Aurora) ? zones[z].speedA : zones[z].speedB;
      auroraUpdate(z, zoneSpeed);
      CRGB c = auroraSample(z, i, intensity);
      return withIntensity(c, intensity);
    }

    case E_Lightning:{
      return withIntensity(base, intensity);
    }
  }
  return withIntensity(base, intensity);
}

static void applyZoneEffects(const Zone &z){
  const uint16_t pA = sliderToPeriod(z.speedA);
  const uint16_t pB = sliderToPeriod(z.speedB);

  // If both effects are OFF, zone is dark
  if (z.effectA == E_Off && z.effectB == E_Off) {
    for (int i=z.startLed; i<=z.endLed; i++) {
      leds[i] = CRGB::Black;
    }
    return;
  }

  // Apply effect A as base
  for (int i=z.startLed; i<=z.endLed; i++) {
    leds[i] = effectSample(z.effectA, z.colorA, pA, z.intensityA, i, 7);
  }
  
  // Apply effect B as overlay if it's not OFF
  if (z.effectB != E_Off) {
    for (int i=z.startLed; i<=z.endLed; i++){
      CRGB over = effectSample(z.effectB, z.colorB, pB, z.intensityB, i, 11);
      // B replaces A only where B produces a non-black color
      if (over.r || over.g || over.b) {
        leds[i] = over;
      }
    }
  }
}

static inline float mapSpeedToHueRate(uint16_t speed) {
  speed = constrain(speed, 10, 1000);
  return 0.02f + 0.68f * ((float)(speed - 10) / 990.0f);
}
static inline float mapSpeedToPhaseRateA(uint16_t speed) {
  speed = constrain(speed, 10, 1000);
  return 0.005f + 0.055f * ((float)(speed - 10) / 990.0f);
}
static inline float mapSpeedToPhaseRateB(uint16_t speed) {
  speed = constrain(speed, 10, 1000);
  return 0.0075f + 0.060f * ((float)(speed - 10) / 990.0f);
}

static inline uint16_t advanceU16Phase(uint16_t phase, float rateHz, uint32_t dtMs){
  float cycles = rateHz * (float)dtMs * 0.001f;
  uint32_t add = (uint32_t)(cycles * 65536.0f + 0.5f);
  return (uint16_t)(phase + add);
}

static inline int8_t tinyShimmer(uint16_t &seed){
  seed ^= seed << 7; seed ^= seed >> 9; seed ^= seed << 8;
  return (int8_t)((seed >> 13) % 3) - 1;
}

static inline float mapAuroraRate(float minv, float maxv, uint16_t speed){
  speed = constrain(speed, 10, 1000);
  const float t = (float)(speed - 10) / 990.0f;
  return minv + (maxv - minv) * t;
}

static inline uint8_t curtainProfile(uint16_t phase){
  int16_t c = cos16(phase);
  uint16_t u = (uint16_t)((c + 32768) >> 8);
  return (uint8_t)((u * u) >> 8);
}

static inline uint16_t auroraRand(uint16_t &seed){
  seed ^= seed << 7; seed ^= seed >> 9; seed ^= seed << 8;
  return seed;
}

static inline void auroraEnsureInit(uint8_t z){
  AuroraState &A = gAurora[z];
  if (A.lastMs) return;
  A.lastMs = millis();
  A.hueBase = 96;
  A.zoneOffset = (uint16_t)(z * (65536u / 3u));
  A.seed = (uint16_t)(0xC0DEu + 97u*z);
  A.layers[0] = { (uint16_t)auroraRand(A.seed), (uint16_t)auroraRand(A.seed),  900 };
  A.layers[1] = { (uint16_t)auroraRand(A.seed), (uint16_t)auroraRand(A.seed), 1150 };
  A.layers[2] = { (uint16_t)auroraRand(A.seed), (uint16_t)auroraRand(A.seed),  700 };
}

static inline void auroraUpdate(uint8_t z, uint16_t speed){
  AuroraState &A = gAurora[z];
  auroraEnsureInit(z);
  uint32_t now = millis();
  uint32_t dt = now - A.lastMs;
  A.lastMs = now;

  float hueRate = mapAuroraRate(0.03f, 0.60f, speed);
  float pulsHz  = mapAuroraRate(0.10f, 0.40f, speed);

  float hueAdd = hueRate * (float)dt * 0.001f;
  uint8_t add8 = (uint8_t)(hueAdd + 0.5f);
  A.hueBase += add8;

  uint32_t addP = (uint32_t)(pulsHz * (float)dt * 0.001f * 65536.0f + 0.5f);
  A.pulsPhase += (uint16_t)addP;

  auto adv = [](uint16_t ph, float hz, uint32_t dtMs)->uint16_t{
    uint32_t add = (uint32_t)(hz * (float)dtMs * 0.001f * 65536.0f + 0.5f);
    return (uint16_t)(ph + add);
  };
  float a0 = mapAuroraRate(0.020f, 0.120f, speed);
  float b0 = mapAuroraRate(0.015f, 0.090f, speed);
  float a1 = mapAuroraRate(0.018f, 0.105f, speed*0.95f);
  float b1 = mapAuroraRate(0.013f, 0.085f, speed*0.90f);
  float a2 = mapAuroraRate(0.024f, 0.140f, speed*1.05f);
  float b2 = mapAuroraRate(0.020f, 0.110f, speed*1.02f);

  A.layers[0].phaseA = adv(A.layers[0].phaseA, a0, dt);
  A.layers[0].phaseB = adv(A.layers[0].phaseB, b0, dt);
  A.layers[1].phaseA = adv(A.layers[1].phaseA, a1, dt);
  A.layers[1].phaseB = adv(A.layers[1].phaseB, b1, dt);
  A.layers[2].phaseA = adv(A.layers[2].phaseA, a2, dt);
  A.layers[2].phaseB = adv(A.layers[2].phaseB, b2, dt);

  if ((auroraRand(A.seed) & 0x7FF) == 0){
    uint8_t k = (uint8_t)(A.seed % 3);
    int d = (A.seed & 1) ? -90 : +90;
    int newW = (int)A.layers[k].width + d;
    if (newW < 560) newW = 560;
    if (newW > 1400) newW = 1400;
    A.layers[k].width = (uint16_t)newW;
  }
}

static inline CRGB auroraSample(uint8_t z, uint16_t iGlobal, uint8_t intensity){
  const Zone &Z = zones[z];
  const uint16_t first = (uint16_t)Z.startLed;
  const uint16_t iLocal = (uint16_t)(iGlobal - first);
  AuroraState &A = gAurora[z];

  uint16_t p0 = (uint16_t)(A.layers[0].phaseA + (uint16_t)((uint32_t)A.layers[0].phaseB * 159u/256u) + A.zoneOffset + (uint16_t)(iLocal * A.layers[0].width));
  uint16_t p1 = (uint16_t)(A.layers[1].phaseA + (uint16_t)((uint32_t)A.layers[1].phaseB * 171u/256u) + A.zoneOffset + (uint16_t)(iLocal * A.layers[1].width));
  uint16_t p2 = (uint16_t)(A.layers[2].phaseA + (uint16_t)((uint32_t)A.layers[2].phaseB * 143u/256u) + A.zoneOffset + (uint16_t)(iLocal * A.layers[2].width));

  uint8_t a0 = curtainProfile(p0);
  uint8_t a1 = curtainProfile(p1);
  uint8_t a2 = curtainProfile(p2);

  uint8_t mx = max(a0, max(a1, a2));

  const float holeStrength = 0.70f;
  uint16_t hole = (uint16_t)((255 - a0) * (255 - a1) / 255);
  hole = (uint16_t)(hole * (255 - a2) / 255);
  uint8_t holes = (uint8_t)(255 - (uint16_t)(hole * holeStrength / 1.0f));

  uint8_t puls = (uint8_t)((sin16(A.pulsPhase) + 32768) >> 8);

  float modDepth = 0.25f + 0.55f * (float)intensity / 255.0f;
  uint16_t baseV = (uint16_t)( (uint16_t)(mx * modDepth) + (uint16_t)(puls * (255 - modDepth)) / 255 );
  uint8_t v = (uint8_t)constrain( (int)( (uint16_t)baseV * holes / 255 ), 0, 255 );
  uint8_t overlap = (uint8_t)(( (uint16_t)min(a0,a1) + (uint16_t)min(a1,a2) + (uint16_t)min(a0,a2) ) / 3);
  int16_t hue = (int16_t)A.hueBase;
  hue += (int16_t)(((int)overlap - 128) / 4);
  if (intensity > 160) hue += (int16_t)(((int)overlap - 128) / 3);

  uint8_t sat = (uint8_t)constrain(190 + (int)overlap/3, 160, 255);

  CRGB c; 
  {
    uint8_t h8 = (uint8_t)hue;
    uint8_t amtI = scale8(intensity, 200);
    uint8_t amtO = scale8(overlap,   220);
    uint8_t amt  = scale8(qadd8(amtI, 40), amtO);
    h8 = lerp8by8(h8, 210, amt);
    hue = h8;
  }
 hsv2rgb_rainbow(CHSV((uint8_t)hue, sat, v), c);
  return c;
}

static inline int firstZoneUsingEffect(uint8_t eff);

static inline uint8_t auroraHolesMask(uint8_t z, uint16_t iGlobal){
  const Zone &Z = zones[z];
  const uint16_t first = (uint16_t)Z.startLed;
  const uint16_t iLocal = (uint16_t)(iGlobal - first);
  AuroraState &A = gAurora[z];

  uint16_t p0 = (uint16_t)(A.layers[0].phaseA + (uint16_t)((uint32_t)A.layers[0].phaseB * 159u/256u) + A.zoneOffset + (uint16_t)(iLocal * A.layers[0].width));
  uint16_t p1 = (uint16_t)(A.layers[1].phaseA + (uint16_t)((uint32_t)A.layers[1].phaseB * 171u/256u) + A.zoneOffset + (uint16_t)(iLocal * A.layers[1].width));
  uint16_t p2 = (uint16_t)(A.layers[2].phaseA + (uint16_t)((uint32_t)A.layers[2].phaseB * 143u/256u) + A.zoneOffset + (uint16_t)(iLocal * A.layers[2].width));

  auto curtainProfile = [](uint16_t phase)->uint8_t{
    int16_t c = cos16(phase);
    uint16_t u = (uint16_t)((c + 32768) >> 8);
    return (uint8_t)((u * u) >> 8);
  };

  uint8_t a0 = curtainProfile(p0);
  uint8_t a1 = curtainProfile(p1);
  uint8_t a2 = curtainProfile(p2);

  const float holeStrength = 0.70f;
  uint16_t hole = (uint16_t)((255 - a0) * (255 - a1) / 255);
  hole = (uint16_t)(hole * (255 - a2) / 255);
  uint16_t scaled = 255 - (uint16_t)(hole * holeStrength);
  if (scaled > 255) scaled = 255;
  return (uint8_t)scaled;
}

static void auroraUpdateAndOverlay() {
  bool useA = true;
  const int zMain = pickOwnerForEffect(E_Aurora, useA);
  if (zMain < 0) return;

  const Zone &Z = zones[zMain];
  /* useA provided by pickOwnerForEffect() */
  const uint16_t speed = useA ? Z.speedA : Z.speedB;
  const uint8_t  inten = useA ? Z.intensityA : Z.intensityB;

  for (uint8_t z = 0; z < 3; ++z) auroraUpdate(z, speed);

  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    uint8_t z = 0;
    if (i >= zones[1].startLed) z = (i <= zones[1].endLed ? 1 : (i >= zones[2].startLed ? 2 : 0));

    const uint8_t holes = auroraHolesMask(z, i);
    leds[i].nscale8_video(holes);

    CRGB a = auroraSample(z, i, inten);
    leds[i].r = qadd8(leds[i].r, a.r);
    leds[i].g = qadd8(leds[i].g, a.g);
    leds[i].b = qadd8(leds[i].b, a.b);
  }
}

static void overloadUpdateAndOverlay() {
  bool useA = true;
  const int zMain = pickOwnerForEffect(E_OverloadSurge, useA);
  if (zMain < 0) return;

  const Zone &Z   = zones[zMain];
  const uint16_t speed = useA ? Z.speedA     : Z.speedB;
  const uint8_t  inten = useA ? Z.intensityA : Z.intensityB;
  const CRGB     tint  = useA ? Z.colorA     : Z.colorB;

  if (inten == 0) return;

  uint16_t sp = constrain(speed, 10, 1000);
  uint16_t breathP = (uint16_t)constrain(
      map((int)sp, 10, 1000, 8000, 2000),
      2000, 8000);
  uint16_t hueP = (uint16_t)constrain(
      map((int)sp, 10, 1000, 12000, 4000),
      4000, 12000);

  uint32_t now = millis();
  uint32_t breathMs = breathP ? (now % breathP) : 0;
  uint32_t hueMs    = hueP    ? (now % hueP)    : 0;

  uint8_t breathPhase = (uint8_t)((breathMs * 256UL) / breathP);
  uint8_t huePhase    = (uint8_t)((hueMs * 256UL)    / hueP);

  uint8_t breathWave = sin8(breathPhase);

  CHSV baseHsv = rgb2hsv_approximate(tint);
  uint8_t baseHue = baseHsv.h;
  uint8_t baseSat = baseHsv.s;

  const uint8_t minRange = 3;
  const uint8_t maxRange = 14;
  uint8_t hueRange = (uint8_t)(minRange +
      ((uint16_t)(maxRange - minRange) * inten) / 255U);

  uint8_t baseDimDepthMax = scale8(inten, 80);

  uint8_t overlayFloor = scale8(inten, 10);
  uint8_t overlayAmp   = inten;

  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    uint8_t pos   = (uint8_t)((i * 256U) / NUM_LEDS);
    uint8_t angle = pos + huePhase;

    uint8_t hueWave    = sin8(angle);
    int16_t signedWave = (int16_t)hueWave - 128;
    int16_t offset     = (signedWave * hueRange) / 128;
    uint8_t hue        = baseHue + (int8_t)offset;

    uint8_t dimAmt    = scale8((uint8_t)(255 - breathWave), baseDimDepthMax);
    uint8_t scaleBase = 255 - dimAmt;
    leds[i].nscale8_video(scaleBase);

    uint8_t overlayVal = qadd8(overlayFloor, scale8(breathWave, overlayAmp));

    CRGB halo;
    hsv2rgb_rainbow(CHSV(hue, baseSat, overlayVal), halo);

    leds[i].r = qadd8(leds[i].r, halo.r);
    leds[i].g = qadd8(leds[i].g, halo.g);
    leds[i].b = qadd8(leds[i].b, halo.b);
  }
}
static const uint8_t LGTN_CAP  = 240;
static const uint8_t LGTN_TINT = 90;

struct LightningZoneState {
  bool     active;
  uint8_t  flashesRemaining;
  uint32_t nextMs;
  uint16_t start;
  uint8_t  len;
  uint8_t  width;
  uint8_t  parity;
};
static LightningZoneState _lz = { false, 0, 0, 0, 0, 1, 0 };
static inline int pickOwnerForEffect(uint8_t eff, bool &useA){
  if (zones[0].effectA == eff){ useA = true;  return 0; }
  if (zones[0].effectB == eff){ useA = false; return 0; }
  if (zones[1].effectA == eff){ useA = true;  return 1; }
  if (zones[1].effectB == eff){ useA = false; return 1; }
  if (zones[2].effectA == eff){ useA = true;  return 2; }
  if (zones[2].effectB == eff){ useA = false; return 2; }
  useA = true; return -1;
}

static uint8_t _ltGlow[NUM_LEDS] = {0};
static uint8_t _ltHalo = 0;
static uint8_t _ltCore = 0;

static inline int firstZoneUsingEffect(uint8_t eff) {
  for (int z = 0; z < 3; z++) {
    if (zones[z].effectA == eff || zones[z].effectB == eff) return z;
  }
  return -1;
}
static inline CRGB tintMix(const CRGB &whiteish, const CRGB &tint, uint8_t mix /*0..255*/) {
  return blend(whiteish, tint, mix);
}

static void lightningUpdateAndOverlay() {
  bool useA = true;
  const int zMain = pickOwnerForEffect(E_Lightning, useA);

  if (zMain < 0) {
    if (_ltHalo) _ltHalo = qsub8(_ltHalo, 8);
    if (_ltCore) _ltCore = qsub8(_ltCore, 12);
    for (int i = 0; i < NUM_LEDS; i++) if (_ltGlow[i]) _ltGlow[i] = qsub8(_ltGlow[i], 8);
    return;
  }

  const Zone &Z = zones[zMain];
  /* useA provided by pickOwnerForEffect() */
  const uint16_t speed = useA ? Z.speedA : Z.speedB;
  const uint8_t  inten = useA ? Z.intensityA : Z.intensityB;
  const CRGB     tintC = useA ? Z.colorA : Z.colorB;

  const uint16_t zStart = Z.startLed;
  const uint16_t zEnd   = Z.endLed;
  const uint16_t zLen   = (zEnd >= zStart) ? (zEnd - zStart + 1) : 0;
  if (!zLen) return;

  const uint8_t fadeAmt = (uint8_t)map(inten, 0, 255, 8, 64);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (_ltGlow[i]) _ltGlow[i] = qsub8(_ltGlow[i], fadeAmt);
  }

  const uint32_t now = millis();

  if (!_lz.active && now >= _lz.nextMs) {
    _lz.active  = true;
    _lz.parity ^= 1;

    _lz.width = 1;
    if (inten >= 96 && random8() < (uint8_t)((inten - 96) / 2)) _lz.width = 2;

    const uint8_t maxLen = (uint8_t)max(3, min(6, (int)(3 + inten / 64)));
    _lz.len = random8(3, maxLen + 1);
    const uint16_t maxStart = (NUM_LEDS > _lz.len) ? (NUM_LEDS - _lz.len) : 0;
_lz.start = (maxStart ? random16(0, maxStart + 1) : 0);
uint8_t hi = (uint8_t)max(1, (int)(1 + inten / 20));
    uint8_t a = random8(1, hi + 1);
    uint8_t b = random8(1, hi + 1);
    _lz.flashesRemaining = min(a, b);
    if (_lz.flashesRemaining < 1) _lz.flashesRemaining = 1;

    _lz.nextMs = now;
  }

  if (_lz.active && now >= _lz.nextMs) {
    int basePeak = min<int>(LGTN_CAP, 200 + (inten / 4));
    int jitter   = (int)random8(41) - 20;
    int pk       = basePeak + jitter;
    if (pk < 0) pk = 0; if (pk > LGTN_CAP) pk = LGTN_CAP;
    const uint8_t peak = (uint8_t)pk;

    for (uint16_t j = 0; j < _lz.len; j++) {
  const uint16_t idx = _lz.start + j;
  if (idx < NUM_LEDS) {
    _ltGlow[idx] = qadd8(_ltGlow[idx], peak);
    if (_lz.width > 1) {
      if (idx > 0) _ltGlow[idx - 1] = qadd8(_ltGlow[idx - 1], (uint8_t)((peak * 3) / 4));
      if (idx + 1 < NUM_LEDS) _ltGlow[idx + 1] = qadd8(_ltGlow[idx + 1], (uint8_t)((peak * 3) / 4));
    }
  }
}
if (zMain == 0) {
      _ltHalo = qadd8(_ltHalo, scale8(peak, 160));
      if (_lz.parity) _ltCore = qadd8(_ltCore, peak);
    }

    uint16_t segMin = (uint16_t)map(inten, 0, 255, 0, 5);
    uint16_t segMax = (uint16_t)map(inten, 0, 255, 260, 30);
    if (segMax <= segMin) segMax = segMin + 1;
    const uint16_t segDelay = random16(segMin, segMax);
    _lz.nextMs = now + segDelay;

    if (_lz.flashesRemaining) _lz.flashesRemaining--;
    if (_lz.flashesRemaining == 0) {
      _lz.active = false;

      uint32_t minQuiet = (uint32_t)map((int)constrain(speed, 10, 1000), 10, 1000, 5000, 100);
      uint32_t maxQuiet = (uint32_t)map((int)constrain(speed, 10, 1000), 10, 1000, 20000, 1000);
      if (maxQuiet < minQuiet) maxQuiet = minQuiet + 1;
      const float ratio = (float)maxQuiet / (float)minQuiet;
      const float u = (float)random(0, 65535) / 65535.0f;
      uint32_t quiet = (uint32_t)((float)minQuiet * powf(ratio, u));
      if (quiet < minQuiet) quiet = minQuiet;
      if (quiet > maxQuiet) quiet = maxQuiet;
      _lz.nextMs = now + quiet;
    }
  }

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t g = _ltGlow[i];
    if (!g) continue;
    CRGB over = tintMix(CRGB(g, g, g), tintC, LGTN_TINT);
    over.r = min<uint8_t>(over.r, LGTN_CAP);
    over.g = min<uint8_t>(over.g, LGTN_CAP);
    over.b = min<uint8_t>(over.b, LGTN_CAP);
    leds[i] += over;
  }

  {
    if (_ltHalo) {
      const Zone &H = zones[1];
      const CRGB haloTint = (H.effectA == E_Off && H.effectB == E_Off) ? tintC : H.colorA;
      CRGB hOver = tintMix(CRGB(_ltHalo, _ltHalo, _ltHalo), haloTint, LGTN_TINT);
      hOver.nscale8_video(180);
      for (int i = H.startLed; i <= H.endLed; i++) leds[i] += hOver;
      _ltHalo = qsub8(_ltHalo, (uint8_t)max<uint8_t>(8, fadeAmt));
    }
    if (_ltCore) {
      const Zone &C = zones[2];
      const CRGB cTint = (C.effectA == E_Off && C.effectB == E_Off) ? tintC : C.colorA;
      CRGB cOver = tintMix(CRGB(_ltCore, _ltCore, _ltCore), cTint, LGTN_TINT);
      cOver.nscale8_video(200);
      leds[C.startLed] += cOver;
      _ltCore = qsub8(_ltCore, (uint8_t)(fadeAmt + 8));
    }
  }
}

enum DFPhase : uint8_t { DF_OFF, DF_SPUTTER, DF_SURGE, DF_BROWNOUT, DF_FLASH, DF__COUNT };

static inline const char* dfPhaseName(DFPhase p){
  switch(p){
    case DF_OFF:      return "OFF";
    case DF_SPUTTER:  return "SPUTTER";
    case DF_SURGE:    return "SURGE";
    case DF_BROWNOUT: return "BROWNOUT";
    case DF_FLASH:    return "FLASH";
    default:          return "?";
  }
}

struct DFState {
  bool     active;
  DFPhase  phase;
  uint32_t nextMs;
  uint32_t phaseStartMs;
  uint32_t phaseSpanMs;
    CRGB tintPrev;
  CRGB tintTarget;
  uint32_t tintStartMs;
  uint32_t tintSpanMs;
uint32_t stepMs;
  uint8_t  sputterIdx;
  uint8_t  brownoutRamp;
  uint8_t  surgeLevel;
};
static DFState _df[3];
static uint8_t _dfGlow[NUM_LEDS] = {0};

static inline uint8_t clampu8(int v){ if (v<0) return 0; if (v>255) return 255; return (uint8_t)v; }
static inline float brownoutFactor(float vbat) {
  if (vbat >= FLICKER_START_V) return 0.0f;
  if (vbat <= FULL_OFF_V)      return 1.0f;
  return (FLICKER_START_V - vbat) / (FLICKER_START_V - FULL_OFF_V);
}
static inline uint32_t pickLogUniform(uint32_t lo, uint32_t hi) {
  float r = (float)random(0, 65536) / 65536.0f;
  float ratio = (float)hi / (float)lo;
  return (uint32_t)((float)lo * powf(ratio, r));
}
static inline CRGB dfBaseTintForPhase(DFPhase ph);
static inline void dfScheduleNext(DFState &S, DFPhase next, uint16_t speed, float bo) {
  float s = 1.35f - 0.90f * ((float)constrain(speed,10,1000) - 10.0f) / 990.0f;
  uint32_t lo=500, hi=8000;
  switch (next) {
    case DF_OFF:      lo=1000; hi= 8000; break;
    case DF_SPUTTER:  lo= 500; hi= 4000; break;
    case DF_BROWNOUT: lo=2000; hi= 5000; break;
    case DF_SURGE:    lo=1000; hi= 5000; break;
    case DF_FLASH:    lo= 100; hi=  500; break;
    default:          lo= 500; hi= 2000; break;
}
  if (next == DF_OFF) {
    float extend = 1.0f + 1.5f * bo;
    lo = (uint32_t)(lo * extend);
    hi = (uint32_t)(hi * extend);
  }
  uint32_t t = pickLogUniform(lo, hi);
  t = (uint32_t)((float)t * s);
  S.tintPrev = S.tintTarget; /* prev tint endpoint */
  S.tintTarget = dfBaseTintForPhase(next); /* new target tint */
  S.tintStartMs = millis();
  S.tintSpanMs  = min<uint32_t>(350, (S.phaseSpanMs ? (S.phaseSpanMs/5) : 250));
S.phaseStartMs = millis();
  S.phaseSpanMs = t;
  S.phase  = next;
  S.nextMs = millis() + t;
}
static inline DFPhase dfPickNextPhase(float bo) {
  float wOff      = 0.6f + 1.4f * bo;
  float wSputter  = 0.6f + 0.8f * bo;
  float wBrown    = 0.7f + 1.2f * bo;
  float wSurge    = 0.5f + 1.0f * bo;
  float wFlash    = 0.3f + 0.7f * bo;
  float total = wOff + wSputter + wBrown + wSurge + wFlash;
  float r = ((float)random16() / 65535.0f) * total;
  if ((r -= wOff)     < 0.0f) return DF_OFF;
  if ((r -= wSputter) < 0.0f) return DF_SPUTTER;
  if ((r -= wBrown)   < 0.0f) return DF_BROWNOUT;
  if ((r -= wSurge)   < 0.0f) return DF_SURGE;
  return DF_FLASH;
}

static inline CRGB dfBaseTintForPhase(DFPhase ph){
  switch (ph){
    case DF_BROWNOUT: return CRGB(128, 71, 28);
    case DF_SURGE:    { CRGB c; hsv2rgb_rainbow(CHSV(30, 235, 200), c); return c; }
    case DF_SPUTTER:  { CRGB c; hsv2rgb_rainbow(CHSV(22, 245, 190), c); return c; }
    case DF_FLASH:    return CRGB(255,255,255);
    default:          { CRGB c; hsv2rgb_rainbow(CHSV(26, 240, 195), c); return c; }
  }
}

static inline void dfOverlayForZone(uint8_t z, uint8_t effA, uint8_t effB, uint16_t spA, uint16_t spB,
                                    uint8_t intA, uint8_t intB, CRGB colA, CRGB colB,
                                    bool autoDFOnThisZone, float bo)
{
  if (!autoDFOnThisZone) return;

  Zone &Z = zones[z];
  DFState &S = _df[z];

  const uint16_t speed     = spB;
  const uint8_t  intensity = intB;
  uint8_t warmHue = 20 + (uint8_t)((millis()/16 + (z*23)) % 20);
  uint8_t warmSat = 235;
  uint8_t warmVal = 200;
  CRGB warm; hsv2rgb_rainbow(CHSV(warmHue, warmSat, warmVal), warm);
  CRGB savedAvg = blend(colA, colB, 128);
  uint8_t tintAlpha = 255;
  if (S.phase != DF_FLASH && S.tintSpanMs > 0) {
    uint32_t tnow = millis();
    uint32_t tel  = (tnow > S.tintStartMs) ? (tnow - S.tintStartMs) : 0;
    if (tel >= S.tintSpanMs) tintAlpha = 255;
    else tintAlpha = (uint8_t)((tel * 255UL) / S.tintSpanMs);
  }
  CRGB tintPhase = S.tintTarget;
  /* crossfade via blend(): */
  tintPhase = blend(S.tintPrev, S.tintTarget, tintAlpha);

  CRGB tintC    = blend(savedAvg, tintPhase, 40);

  for (int i = Z.startLed; i <= Z.endLed; i++) if (_dfGlow[i]) _dfGlow[i] = qsub8(_dfGlow[i], 6);

  if (!S.active) {
    S.active = true;
    S.sputterIdx = 0;
    S.brownoutRamp = 0;
    S.surgeLevel = 0;
    dfScheduleNext(S, dfPickNextPhase(bo), speed, bo);
  }

  const uint32_t now = millis();
  switch (S.phase) {
    case DF_OFF:
      break;

    case DF_SPUTTER: {
      if (now >= S.stepMs) {
        uint8_t level = clampu8(140 + random8(70));
        if (bo > 0.0f) level = scale8(level, clampu8(255 - (uint8_t)(120*bo)));
        int jitter = (int)random8(41) - 20;
        uint8_t peak = (uint8_t)min(240, max(0, (int)level + jitter));
        for (int i=Z.startLed; i<=Z.endLed; i++){
          CRGB over = blend(CRGB(peak, peak*2/3, 0), tintC, 85);
          leds[i] += over;
        }
        uint16_t minMs = map(intensity, 0, 255, 180, 12);
        uint16_t maxMs = map(intensity, 0, 255, 320, 40);
        uint16_t next = minMs + (random16(maxMs - minMs + 1));
        S.stepMs = now + next;
      }
    } break;

    case DF_SURGE: {
      {
        const uint8_t FLOOR_PWM = 7;
        uint32_t nowMs = now;
        uint32_t span  = S.phaseSpanMs ? S.phaseSpanMs : (uint32_t)800;
        uint32_t el    = (nowMs > S.phaseStartMs) ? (nowMs - S.phaseStartMs) : 0;
        if (el > span) el = span;
        float p = span ? ((float)el / (float)span) : 1.0f;

        float dwellTop = 0.05f + 0.20f * ((float)intensity / 255.0f);
        if (dwellTop > (dwellTop * 1.30f)) dwellTop =  0.364f;
        float fadeFrac = 0.10f;
        float inhaleF  = 1.0f - dwellTop - fadeFrac;
        if (inhaleF < 0.20f) { inhaleF = 0.20f; fadeFrac = 1.0f - dwellTop - inhaleF; if (fadeFrac < 0.06f) fadeFrac = 0.06f; }

        float y;
        if (p < inhaleF) {
          float x = p / inhaleF;
          y = x*x*(3.0f - 2.0f*x);
        } else if (p < inhaleF + dwellTop) {
          y = 1.0f;
        } else {
          float x = (p - inhaleF - dwellTop) / max(0.001f, fadeFrac);
          float f = 1.0f - (x*x);
          if (f < 0.0f) f = 0.0f;
          y = f;
        }

        int peak255 = FLOOR_PWM + (int)((255 - FLOOR_PWM) * y + 0.5f);
        if (peak255 > (int)intensity) peak255 = intensity;

        if (bo > 0.0f) peak255 = scale8(peak255, clampu8(255 - (uint8_t)(120*bo)));

        if (z == 0) {
          for (int zi=0; zi<3; ++zi){
            Zone &ZZ = zones[zi];
            for (int iLed=ZZ.startLed; iLed<=ZZ.endLed; ++iLed){
              CRGB over = blend(CRGB(peak255, (peak255*2)/3, 0), tintC, 85);
              leds[iLed] += over;
            }
          }
        }
        uint16_t sub = map(intensity, 0, 255, 28, 12);
        S.stepMs = nowMs + sub;
      }
    } break;

    case DF_BROWNOUT: {
      {
        uint32_t nowMs = now;
        uint32_t span  = S.phaseSpanMs ? S.phaseSpanMs : (uint32_t)900;
        uint32_t el    = (nowMs > S.phaseStartMs) ? (nowMs - S.phaseStartMs) : 0;
        if (el > span) el = span;
        float p = span ? ((float)el / (float)span) : 1.0f;

        float decay = 1.0f - (p*p*(3.0f - 2.0f*p));
        int v = (int)(decay * 255.0f + 0.5f);
        if (v > (int)intensity) v = intensity;

        CRGB brownBase = CRGB(128, 71, 28);
        uint8_t deepen = (uint8_t)(p * 60.0f);
        CRGB brownP = CRGB(
          qadd8(brownBase.r, deepen/3),
          qsub8(brownBase.g, deepen/5),
          qsub8(brownBase.b, (uint8_t)(deepen/4))
        );

        uint8_t blendAmt = (uint8_t)(p * 160.0f);
        CRGB warmBrown = blend(tintC, brownP, blendAmt);

        uint8_t vv = (uint8_t)v;
        if (bo > 0.0f) vv = scale8(vv, clampu8(255 - (uint8_t)(120*bo)));
        int8_t jit = (int8_t)random8(7) - 3;
        vv = clampu8((int)vv + jit);

        CRGB over = warmBrown;
        over.nscale8_video(vv);

        for (int iLed=Z.startLed; iLed<=Z.endLed; ++iLed){
          leds[iLed] += over;
        }

        uint16_t sub = map(intensity, 0, 255, 30, 14);
        S.stepMs = nowMs + sub;
      }
    } break;

    case DF_FLASH: {
      if (now >= S.stepMs) {
        uint8_t peak = clampu8(map(intensity, 0, 255, 180, 255));
        int8_t jitter = (int8_t)random8(21) - 10;
        peak = clampu8((int)peak + jitter);

        if (z == 0) {
          for (int zi=0; zi<3; ++zi){
            Zone &ZZ = zones[zi];
            for (int iLed=ZZ.startLed; iLed<=ZZ.endLed; ++iLed){
              CRGB burst = blend(CRGB(255,255,255).nscale8_video(peak), tintC, 20);
              leds[iLed] += burst;
            }
          }
        }
        uint16_t next = map(intensity, 0, 255, 140, 60);
        S.stepMs = now + next;
      }
    } break;

    default: break;
  }
  for (int i = Z.startLed; i <= Z.endLed; i++) {
    const uint8_t g = _dfGlow[i];
    if (!g) continue;
    CRGB ember = blend(CRGB(g, (g*3)/4, 0), tintC, 80);
    leds[i] += ember;
  }
if (now >= S.nextMs) dfScheduleNext(S, dfPickNextPhase(bo), speed, bo);
}

static void dyingFlickerUpdateAndOverlay(bool autoDF, float vbat) {
  const bool autoDFActive = (autoDF && (vbat < dfThresholdV));
  float bo = brownoutFactor(vbat);
  for (uint8_t z=0; z<3; z++){
    const bool autoDFHere = autoDFActive;
    dfOverlayForZone(
      z,
      zones[z].effectA, zones[z].effectB,
      zones[z].speedA,  zones[z].speedB,
      zones[z].intensityA, zones[z].intensityB,
      zones[z].colorA,  zones[z].colorB,
      autoDFHere,
      bo
    );
  }
}

static void applyAllEffects(){
  static bool lowBattMode = false;
#if LBF_HYSTERESIS_MV > 0
  if (!lowBattMode && batteryVoltage < FLICKER_START_V) lowBattMode = true;
  if (lowBattMode && batteryVoltage > (FLICKER_START_V + (LBF_HYSTERESIS_MV/1000.0f))) lowBattMode = false;
#endif

  for(int i=0;i<NUM_LEDS;i++) leds[i]=CRGB::Black;
  refreshSeeds();
  float gateV = simVbatEnabled ? simVbat : ( (batt.count >= BAT_SAMPLES_PER_WINDOW) ? batteryVoltage : 99.0f );
  bool autoDFActive = (autoDFEnabled && (gateV < dfThresholdV));

  if (!autoDFActive) {
    for(int z=0; z<3; z++) applyZoneEffects(zones[z]);
  }

  if (!autoDFActive) {
    overloadUpdateAndOverlay();
    auroraUpdateAndOverlay();
    lightningUpdateAndOverlay();
  }
  if (!autoDFActive && powerPulseAnyZoneSelected()) {
    PP_applyPowerPulseOverlay(leds, millis());
  }

  dyingFlickerUpdateAndOverlay(autoDFEnabled, gateV);

  if (autoDFActive && gateV < FLICKER_START_V) {
    float bo = brownoutFactor(gateV);
    uint8_t scaled = (uint8_t)((float)masterBrightness * (0.75f - 0.25f * bo));
    FastLED.setBrightness(scaled);
  } else {
    FastLED.setBrightness(masterBrightness);
  }

  {
    CRGB &cz3 = leds[Z3_START];
    cz3.r = qadd8(cz3.r, (uint8_t)((uint16_t(cz3.r) * 51U) >> 8));
    cz3.g = qadd8(cz3.g, (uint8_t)((uint16_t(cz3.g) * 51U) >> 8));
    cz3.b = qadd8(cz3.b, (uint8_t)((uint16_t(cz3.b) * 51U) >> 8));
  }
FastLED.show();}

// === INSERTION POINT: PRESET_APPLY_HELPERS ===
static void snapshotCurrentStateToJson(JsonDocument& st){
  st.clear();
  st["master"] = (uint8_t)masterBrightness;
  st["autoDF"] = autoDFEnabled;
  st["dfThresholdV"] = dfThresholdV;
  st["simVbatEnabled"] = simVbatEnabled;
  st["simVbat"] = simVbat;
    st["wifiIdleAutoOff"] = wifiIdleAutoOff;
auto zarr = st["zones"].to<JsonArray>();
  for (int z = 0; z < 3; ++z) {
    JsonObject jo = zarr.add<JsonObject>();
    char nmA[24], nmB[24];
    effectName((Effect)zones[z].effectA, nmA, sizeof(nmA));
    effectName((Effect)zones[z].effectB, nmB, sizeof(nmB));
    jo["effectA"] = nmA;
    jo["effectB"] = nmB;
    jo["colorA"]  = colorToHex(zones[z].colorA);
    jo["colorB"]  = colorToHex(zones[z].colorB);
    jo["speedA"]  = clampSpeed((uint16_t)zones[z].speedA);
    jo["speedB"]  = clampSpeed((uint16_t)zones[z].speedB);
    jo["intensityA"] = (uint8_t)zones[z].intensityA;
    jo["intensityB"] = (uint8_t)zones[z].intensityB;
  }
}

static void applyPresetJson(const JsonDocument& stDoc){
  auto st = stDoc.as<JsonVariantConst>();

if (st["master"].is<uint8_t>()) masterBrightness = st["master"].as<uint8_t>();
    if (st["autoDF"].is<bool>())    autoDFEnabled    = st["autoDF"].as<bool>();

    if (st.containsKey("dfThresholdV")) { 
      float th = st["dfThresholdV"].as<float>(); 
      if (th < 3.55f) th = 3.55f; if (th > 3.70f) th = 3.70f; 
      dfThresholdV = th; 
    
      prefWriteFloat(PREF_DF_THRESH, dfThresholdV);
    }
    if (st.containsKey("simVbatEnabled")) simVbatEnabled = st["simVbatEnabled"].as<bool>();
    if (st.containsKey("simVbat")) {
      float sv = st["simVbat"].as<float>();
      if (sv < 3.50f) sv = 3.50f; if (sv > 4.20f) sv = 4.20f;
      simVbat = sv;
    }
    if (st.containsKey("wifiIdleAutoOff")) wifiIdleAutoOff = st["wifiIdleAutoOff"].as<bool>();
for(int z=0; z<3; z++){
      JsonObjectConst zobj = st["zones"][z].as<JsonObjectConst>();
      if (zobj["effectA"].is<const char*>()) zones[z].effectA = (uint8_t)effectFromCString(zobj["effectA"]);
      if (zobj["effectB"].is<const char*>()) zones[z].effectB = (uint8_t)effectFromCString(zobj["effectB"]);
      if (zobj["colorA"].is<const char*>()){ String c=(const char*)zobj["colorA"]; long v=strtol(c.substring(1).c_str(),NULL,16); zones[z].colorA=CRGB((v>>16)&0xFF,(v>>8)&0xFF,v&0xFF); }
      if (zobj["colorB"].is<const char*>()){ String c=(const char*)zobj["colorB"]; long v=strtol(c.substring(1).c_str(),NULL,16); zones[z].colorB=CRGB((v>>16)&0xFF,(v>>8)&0xFF,v&0xFF); }
      if (zobj.containsKey("speedA")) zones[z].speedA = clampSpeed(zobj["speedA"].as<uint16_t>());
      if (zobj.containsKey("speedB")) zones[z].speedB = clampSpeed(zobj["speedB"].as<uint16_t>());
      if (zobj.containsKey("intensityA")) zones[z].intensityA = zobj["intensityA"].as<uint8_t>();
      if (zobj.containsKey("intensityB")) zones[z].intensityB = zobj["intensityB"].as<uint8_t>();
    }
    FastLED.setBrightness(masterBrightness);

}

static bool applyPresetIndexInternal(int idx, bool persistActive){
  if (idx < 0 || idx > 7) return false;
  String blob = prefReadString(presetKeyFor(idx));
  if (!blob.length()) return false;

  StaticJsonDocument<4096> st;
  if (deserializeJson(st, blob)) return false;

  applyPresetJson(st);

  if (persistActive){
    activePreset = idx;
    prefWriteChar(PREF_ACTIVE_PRESET, activePreset);
  }
  stateEpoch++;
  return true;
}

static bool isPresetSaved(int idx){
  if (idx < 0 || idx > 7) return false;
  String blob = prefReadString(presetKeyFor(idx));
  return blob.length() > 0;
}

static bool cyclePresetNext(){
  int savedCount = 0; for (int i=0;i<8;i++) if (isPresetSaved(i)) savedCount++;
  if (savedCount == 0) return false;

  int start = (activePreset >= 0 && activePreset <= 7) ? (activePreset + 1) : 0;
  for (int k=0; k<8; k++){
    int idx = (start + k) % 8;
    if (isPresetSaved(idx)){
      return applyPresetIndexInternal(idx, true);
    }
  }
  if (savedCount == 1){
    for (int i=0;i<8;i++) if (isPresetSaved(i)) return applyPresetIndexInternal(i, true);
  }
  return false;
}
static void loadFavorites(){
  Preferences prefs;
  prefBeginRead(prefs);
  for (uint8_t i=0;i<9;i++){
    String def = favHex[i];
    String got = prefs.getString(favKeyFor(i), def);
    if (got.length()==7 && got[0]=='#') favHex[i]=got;
  }
  prefEnd(prefs);
}
static void saveFavorite(uint8_t idx, const String &hex){
  if (idx>8) return;
  favHex[idx]=hex;
  prefWriteString(favKeyFor(idx), hex);
  addDebugf("Saved favorite %u: %s", idx, hex.c_str());
}

static uint32_t rgbToU32(const CRGB &c){
  return (uint32_t(c.r)<<16) | (uint32_t(c.g)<<8) | uint32_t(c.b);
}
static void loadPrefs(){
  Preferences prefs;
  prefBeginRead(prefs);
  masterBrightness = prefs.getUInt(PREF_MASTER, masterBrightness);
  autoDFEnabled    = prefs.getBool(PREF_AUTO_DF, autoDFEnabled);
  dfThresholdV     = prefs.getFloat(PREF_DF_THRESH, dfThresholdV);
  wifiIdleAutoOff  = prefs.getBool(PREF_WIFI_IDLE, wifiIdleAutoOff);
  if (!(dfThresholdV>=3.55f && dfThresholdV<=3.70f)) dfThresholdV=3.60f;
  activePreset     = prefs.getChar(PREF_ACTIVE_PRESET, activePreset);

  for(int z=0;z<3;z++){
    String p="zone"+String(z);

    String sA = prefs.getString((p+"A").c_str(), "");
    String sB = prefs.getString((p+"B").c_str(), "");
    if (sA.length()) zones[z].effectA = (uint8_t)effectFromCString(sA.c_str());
    if (sB.length()) zones[z].effectB = (uint8_t)effectFromCString(sB.c_str());

    uint32_t cA = prefs.getUInt((p+"colorA").c_str(),
                    (zones[z].colorA.r<<16)|(zones[z].colorA.g<<8)|zones[z].colorA.b);
    uint32_t cB = prefs.getUInt((p+"colorB").c_str(),
                    (zones[z].colorB.r<<16)|(zones[z].colorB.g<<8)|zones[z].colorB.b);
    zones[z].colorA = CRGB((cA>>16)&0xFF,(cA>>8)&0xFF,cA&0xFF);
    zones[z].colorB = CRGB((cB>>16)&0xFF,(cB>>8)&0xFF,cB&0xFF);
    zones[z].speedA = prefs.getUInt((p+"speedA").c_str(), zones[z].speedA);
    zones[z].speedB = prefs.getUInt((p+"speedB").c_str(), zones[z].speedB);
    zones[z].intensityA = prefs.getUChar((p+"intA").c_str(), zones[z].intensityA);
    zones[z].intensityB = prefs.getUChar((p+"intB").c_str(), zones[z].intensityB);
  }
  prefEnd(prefs);
  FastLED.setBrightness(masterBrightness);
  loadFavorites();
}
static void savePrefsFromJSON(const ArduinoJson::JsonObject &root){
  Preferences prefs;
  prefBeginWrite(prefs);

  if (root.containsKey("master")){
    masterBrightness = root["master"].as<uint8_t>();
    prefs.putUInt(PREF_MASTER, masterBrightness);
  }
  if (root.containsKey("autoDF")){
    autoDFEnabled = root["autoDF"].as<bool>();
    prefs.putBool(PREF_AUTO_DF, autoDFEnabled);
  }
  if (!root["zones"].is<JsonArrayConst>() || root["zones"].size() < 3) {
    prefEnd(prefs); return;
  }

  for(int z=0;z<3;z++){
    String p="zone"+String(z);
    ArduinoJson::JsonObjectConst zobj = root["zones"][z].as<JsonObjectConst>();

    if (zobj.containsKey("effectA")){
      const char* nm = zobj["effectA"];
      prefs.putString((p+"A").c_str(), nm ? nm : "Off");
    }
    if (zobj.containsKey("effectB")){
      const char* nm = zobj["effectB"];
      prefs.putString((p+"B").c_str(), nm ? nm : "Off");
    }
    if (zobj.containsKey("colorA")){
      String c=(const char*)zobj["colorA"];
      long v=strtol(c.substring(1).c_str(),NULL,16);
      zones[z].colorA=CRGB((v>>16)&0xFF,(v>>8)&0xFF,v&0xFF);
      prefs.putUInt((p+"colorA").c_str(), rgbToU32(zones[z].colorA));
    }
    if (zobj.containsKey("colorB")){
      String c=(const char*)zobj["colorB"];
      long v=strtol(c.substring(1).c_str(),NULL,16);
      zones[z].colorB=CRGB((v>>16)&0xFF,(v>>8)&0xFF,v&0xFF);
      prefs.putUInt((p+"colorB").c_str(), rgbToU32(zones[z].colorB));
    }
    if (zobj.containsKey("speedA")){
      zones[z].speedA = constrain(zobj["speedA"].as<uint16_t>(), 10, 1000);
      prefs.putUInt((p+"speedA").c_str(), zones[z].speedA);
    }
    if (zobj.containsKey("speedB")){
      zones[z].speedB = constrain(zobj["speedB"].as<uint16_t>(), 10, 1000);
      prefs.putUInt((p+"speedB").c_str(), zones[z].speedB);
    }
    if (zobj.containsKey("intensityA")){
      zones[z].intensityA = zobj["intensityA"].as<uint8_t>();
      prefs.putUChar((p+"intA").c_str(), zones[z].intensityA);
    }
    if (zobj.containsKey("intensityB")){
      zones[z].intensityB = zobj["intensityB"].as<uint8_t>();
      prefs.putUChar((p+"intB").c_str(), zones[z].intensityB);
    }
  }
  
  if (root.containsKey("dfThresholdV")) {
    float th = root["dfThresholdV"].as<float>();
    if (th < 3.55f) th = 3.55f;
    if (th > 3.70f) th = 3.70f;
    dfThresholdV = th;
    prefs.putFloat(PREF_DF_THRESH, dfThresholdV);
  }

  prefEnd(prefs);
  FastLED.setBrightness(masterBrightness);
  addDebug("Preferences saved");
}

static bool wifiOn = true;
static unsigned long lastActivityMs = 0;
static const unsigned long IDLE_OFF_MS  = 5UL * 60UL * 1000UL;
// AP readiness timing constants - WiFi.softAP() is asynchronous
// Initial delay: 100ms for AP to start initialization
// Retry delay: 100ms between IP availability checks
// Max retries: 10 attempts = 1 second total timeout
static const unsigned long AP_READY_INITIAL_DELAY_MS = 100;
static const unsigned long AP_READY_RETRY_DELAY_MS = 100;
static const int AP_READY_MAX_RETRIES = 10;
static inline void recordActivity(){ lastActivityMs = millis(); }

// Helper function to start DNS server for captive portal detection
static inline void startDNSServer(){
  // Redirect all DNS requests to the ESP32's IP address
  // This enables captive portal detection on phones
  dnsServer.start(53, "*", WiFi.softAPIP());
}

// Helper function to wait for AP to be fully ready
static inline void waitForAPReady(){
  // WiFi.softAP is asynchronous - wait for it to be fully initialized
  delay(AP_READY_INITIAL_DELAY_MS);
  
  // Ensure AP is fully up by checking for valid IP
  int retries = 0;
  while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0) && retries < AP_READY_MAX_RETRIES) {
    delay(AP_READY_RETRY_DELAY_MS);
    retries++;
  }
  
  // Log warning if AP didn't get IP after retries (though it may still work with default 192.168.4.1)
  if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
    uint32_t totalWaitMs = AP_READY_INITIAL_DELAY_MS + (AP_READY_MAX_RETRIES * AP_READY_RETRY_DELAY_MS);
    addDebugf("Warning: AP IP not ready after %lums (%d retries), proceeding anyway", totalWaitMs, AP_READY_MAX_RETRIES);
  }
}

static void wifiEnable(){
  if (wifiOn) return;
  
  // Set WiFi mode and configure AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  // Wait for AP to be fully ready before starting services
  waitForAPReady();
  
  // Now start DNS and web server services
  startDNSServer();
  server.begin();
  
  addDebugf("Wi-Fi ON  AP %s %s", AP_SSID, WiFi.softAPIP().toString().c_str());
  wifiOn = true;
  recordActivity();
}
static void wifiDisable(){
  if (!wifiOn) return;
  
  // Stop services in reverse order: web server, then DNS, then WiFi
  server.stop();
  dnsServer.stop();
  
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  addDebug("Wi-Fi OFF");
  wifiOn = false;
}

static inline void appendSeg(String& m, const String& seg) {
  if (m.length()) m += char(0x0A);
  m += seg;
}

struct MsgInputs {
  bool     allowAutoSleep;
  bool     simVbatEnabled;
  bool     autoDFEnabled;
  bool     wifiOn;
  float    batteryVoltage;
  float    simVbat;
  float    dfThresholdV;
  const char* phaseName;
  uint32_t idleMs;
  uint32_t idleOffMs;
  uint32_t simLowSinceMs;
};

static inline int ceilDivu(uint32_t num, uint32_t den){
  return (int)((num + den - 1) / den);
}

static String buildStatusMessage(const MsgInputs& s){
  String m;

  if (!s.allowAutoSleep) appendSeg(m, "Grace period active");

  float gateV = s.simVbatEnabled ? s.simVbat : s.batteryVoltage;
  if (s.autoDFEnabled && gateV < s.dfThresholdV) {
    appendSeg(m, String("DyingFlicker active  Phase: ") + (s.phaseName ? s.phaseName : "?"));
  }

  if (s.allowAutoSleep && s.simVbatEnabled && s.simVbat < FULL_OFF_V && s.simLowSinceMs != 0) {
    uint32_t elapsed = millis() - s.simLowSinceMs;
    if (elapsed < 10000UL) {
      uint32_t remain = 10000UL - elapsed;
        if (remain <= 800UL) {
    appendSeg(m, "Sleeping. Long-press to wake.");
  } else {
    appendSeg(m, String("Simulation is active — deep sleep in ") + ceilDivu(remain, 1000) + "s");
  }
}
  }

  if (s.wifiOn && s.idleMs < s.idleOffMs) {
    uint32_t remain = s.idleOffMs - s.idleMs;
    if (remain <= 30000UL) {
      if (remain <= 1000UL) {
        appendSeg(m, "Wi-Fi off. Press button to turn on.");
      } else {
        appendSeg(m, String("Wi-Fi turning off in ") + ceilDivu(remain, 1000) + "s");
      }
    }
  }
  return m;
}

static void sendStatusJSON(){
  StaticJsonDocument<3072> doc;

  doc["epoch"]    = stateEpoch;
  doc["buildTag"] = BUILD_TAG;
  doc["compiled"] = __DATE__ " " __TIME__;
  doc["file"]     = __FILE__;
  doc["battery"]  = batteryVoltage;
  doc["master"]   = masterBrightness;
  doc["autoDF"]   = autoDFEnabled;
  doc["dfThresholdV"]   = dfThresholdV;
  doc["simVbatEnabled"] = simVbatEnabled;
  doc["simVbat"]        = simVbat;
    doc["wifiIdleAutoOff"] = wifiIdleAutoOff;
doc["activePreset"]   = activePreset;

  const char* phaseName = nullptr;
  for (int zi=0; zi<3; ++zi){
    if (_df[zi].active) { phaseName = dfPhaseName(_df[zi].phase); break; }
  }
  unsigned long now = millis();
  unsigned long idle = (wifiOn ? (now - lastActivityMs) : 0UL);
  MsgInputs s = {
    .allowAutoSleep = allowAutoSleep,
    .simVbatEnabled = simVbatEnabled,
    .autoDFEnabled  = autoDFEnabled,
    .wifiOn         = wifiOn,
    .batteryVoltage = batteryVoltage,
    .simVbat        = simVbat,
    .dfThresholdV   = dfThresholdV,
    .phaseName      = phaseName ? phaseName : "OFF",
    .idleMs         = idle,
    .idleOffMs      = (wifiIdleAutoOff ? IDLE_OFF_MS : 0),
    .simLowSinceMs  = simLowSinceMs
  };
  String msg = buildStatusMessage(s);
  doc["message"] = msg;

  auto zarr = doc["zones"].to<JsonArray>();
  for (int z = 0; z < 3; z++){
    JsonObject jo = zarr.add<JsonObject>();
    jo["name"]    = (z==0) ? "Plasma Halo" : (z==1) ? "Flux Matrix" : "Repulsor Nexus";
    char nmA[16], nmB[16];
    effectName((Effect)zones[z].effectA, nmA, sizeof(nmA));
    effectName((Effect)zones[z].effectB, nmB, sizeof(nmB));
    jo["effectA"] = nmA; jo["effectB"] = nmB;
    jo["colorA"]  = colorToHex(zones[z].colorA);
    jo["colorB"]  = colorToHex(zones[z].colorB);
    jo["speedA"]  = zones[z].speedA;
    jo["speedB"]  = zones[z].speedB;
    jo["intensityA"] = zones[z].intensityA;
    jo["intensityB"] = zones[z].intensityB;
  }

  auto favs = doc["favs"].to<JsonArray>();
  for (int i=0; i<9; i++) favs.add(favHex[i]);

  auto dbg = doc["debug"].to<JsonArray>();
  for (int i=0;i<DEBUG_LINES;i++) dbg.add(debugLog[i]);

  String out; serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void enterDeepSleepNow(const char* reason){
  addDebugf("Entering deep sleep: %s", reason ? reason : "");
  FastLED.clear(true); FastLED.show();
  WiFi.mode(WIFI_OFF);
  delay(50);
  esp_deep_sleep_start();
}

// NOTE: INDEX_HTML has been extracted to web_pages.h
#include "web_pages.h"

// ---------- Setup ----------

static void applyActivePresetOnBoot(){
  int8_t idx = prefReadChar(PREF_ACTIVE_PRESET, -1);
  if (idx < 0) return;
  
  String blob = prefReadString(presetKeyFor(idx));
  if (!blob.length()) return;

  StaticJsonDocument<4096> st;
  DeserializationError e = deserializeJson(st, blob);
  if (e) return;

  applyPresetJson(st);
}

void setup(){
  Serial.begin(115200);
  
  // Boot banner (keeps runtime info accurate even if comments drift)
  Serial.printf("Build %s | File %s | Compiled %s %s\r\n", BUILD_TAG, __FILE__, __DATE__, __TIME__);
delay(100);
  Serial.println(String("Arc Reactor MK1 boot (") + BUILD_TAG + ") (" + String(BUILD_TAG) + " — gateV dim-bias; DF-only gating; updated timings; warm tint overlay)");

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setDither(false); // v4c7b14: disable temporal dithering for smooth low-end breath

  FastLED.clear(true);
  FastLED.setBrightness(masterBrightness);

  // RNG seed once
  randomSeed(esp_random());

  // Default zones (first boot)
  zones[0] = {0, 15, E_Candle,     E_ArcFlicker,   CRGB::Orange, CRGB::Blue,  300, 300, 255, 255};
  zones[1] = {16,23, E_ArcFlicker, E_WavePulse,    CRGB::Blue,   CRGB::Cyan,  400, 400, 255, 255};
  zones[2] = {24,24, E_WavePulse,  E_Off,          CRGB::White,  CRGB::Black, 350, 350, 255, 255};

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(VBAT_PIN, INPUT);
  batteryVoltage = 2.0f * analogReadMilliVolts(VBAT_PIN) / 1000.0f;

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  addDebug("C3: GPIO wake enabled (LOW on GPIO3)");
#else
  #if defined(ESP_GPIO_WAKEUP_GPIO_LOW)
    esp_sleep_enable_ext1_wakeup(1ULL<<BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  #else
    esp_sleep_enable_ext1_wakeup(1ULL<<BUTTON_PIN, 0);
  #endif
  addDebug("ESP32: EXT1 wake enabled");
#endif

  // Wake gate after deep sleep
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause == ESP_SLEEP_WAKEUP_GPIO || cause == ESP_SLEEP_WAKEUP_EXT1) {
    addDebug("Wake cause: GPIO - enforcing hold-to-wake");
    if (digitalRead(BUTTON_PIN) != LOW) { enterDeepSleepNow("shortWake"); }
    uint32_t t0 = millis();
    while (digitalRead(BUTTON_PIN)==LOW && (millis() - t0) < BUTTON_HOLD_MS) { delay(10); }
    if ((millis() - t0) < BUTTON_HOLD_MS) enterDeepSleepNow("shortWake");
  }

  // AP up
  WiFi.softAP(AP_SSID, AP_PASS);
  
  // Wait for AP to be fully ready before starting services
  waitForAPReady();
  
  // Start DNS server for captive portal detection
  startDNSServer();
  
  addDebugf("AP: %s IP %s", AP_SSID, WiFi.softAPIP().toString().c_str());
  recordActivity();

  loadPrefs();

  // Auto-apply last active preset on boot (restores full state)
  // ===== Routes =====
server.on("/update", HTTP_POST, [](){
    if (!server.hasArg("plain")) { server.send(400,"text/plain","Missing body"); return; }
    recordActivity();

    StaticJsonDocument<4096> doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err){ server.send(400,"text/plain","Bad JSON"); return; }

    JsonObject root = doc.as<JsonObject>();
    if (root.containsKey("master")) masterBrightness = root["master"].as<uint8_t>();
    if (root.containsKey("autoDF")) autoDFEnabled = root["autoDF"].as<bool>();

    if (root.containsKey("wifiIdleAutoOff")) { 
      wifiIdleAutoOff = root["wifiIdleAutoOff"].as<bool>(); 
      prefWriteBool(PREF_WIFI_IDLE, wifiIdleAutoOff); 
    }
    if (root.containsKey("dfThresholdV")) { float th = root["dfThresholdV"].as<float>(); if (th<3.55f) th=3.55f; if (th>3.70f) th=3.70f; dfThresholdV = th; }
    if (root.containsKey("simVbatEnabled")) simVbatEnabled = root["simVbatEnabled"].as<bool>();
    if (root.containsKey("simVbat")) { float sv = root["simVbat"].as<float>(); if (sv<3.50f) sv=3.50f; if (sv>4.20f) sv=4.20f; simVbat = sv; }

    if (!root["zones"].is<JsonArrayConst>() || root["zones"].size() < 3) {
      server.send(400,"text/plain","Bad zones"); return;
    }

    for(int z=0; z<3; z++){
      JsonObjectConst zobj = root["zones"][z].as<JsonObjectConst>();
      if (zobj.containsKey("effectA")) { zones[z].effectA = (uint8_t)effectFromCString(zobj["effectA"]); }
      if (zobj.containsKey("effectB")) { zones[z].effectB = (uint8_t)effectFromCString(zobj["effectB"]); }
      if (zobj.containsKey("colorA")){ String c=(const char*)zobj["colorA"]; long v=strtol(c.substring(1).c_str(),NULL,16); zones[z].colorA=CRGB((v>>16)&0xFF,(v>>8)&0xFF,v&0xFF); }
      if (zobj.containsKey("colorB")){ String c=(const char*)zobj["colorB"]; long v=strtol(c.substring(1).c_str(),NULL,16); zones[z].colorB=CRGB((v>>16)&0xFF,(v>>8)&0xFF,v&0xFF); }
      if (zobj.containsKey("speedA"))  zones[z].speedA  = constrain(zobj["speedA"].as<uint16_t>(), 10, 1000);
      if (zobj.containsKey("speedB"))  zones[z].speedB  = constrain(zobj["speedB"].as<uint16_t>(), 10, 1000);
      if (zobj.containsKey("intensityA")) zones[z].intensityA = zobj["intensityA"].as<uint8_t>();
      if (zobj.containsKey("intensityB")) zones[z].intensityB = zobj["intensityB"].as<uint8_t>();
    }
    masterBrightness = constrain(masterBrightness, 0, 255);
    FastLED.setBrightness(masterBrightness);
    server.send(200,"text/plain","OK");
  });

  server.on("/save", HTTP_POST, [](){
    if (!server.hasArg("plain")) { server.send(400,"text/plain","Missing body"); return; }
    recordActivity();
    StaticJsonDocument<4096> doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err){ server.send(400,"text/plain","Bad JSON"); return; }
    savePrefsFromJSON(doc.as<JsonObject>());
    server.send(200,"text/plain","SAVED");
  });

  server.on("/getFavs", HTTP_GET, [](){
    StaticJsonDocument<256> doc;
    auto arr = doc["favs"].to<JsonArray>();
    for (int i=0;i<9;i++) arr.add(favHex[i]);
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  server.on("/setFav", HTTP_POST, [](){
    if (!server.hasArg("plain")) { server.send(400,"text/plain","Missing body"); return; }
    recordActivity();
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err){ server.send(400,"text/plain","Bad JSON"); return; }
    int idx = doc["idx"] | -1;
    const char* hx = doc["hex"] | "#FFFFFF";
    if (idx < 0 || idx > 8) { server.send(400,"text/plain","Bad index"); return; }
    String s = String(hx); s.toUpperCase();
    if (s.length()!=7 || s[0] != '#') { server.send(400,"text/plain","Bad hex"); return; }
    saveFavorite(idx, s);
    server.send(200,"text/plain","OK");
  });

  // Presets endpoints
  server.on("/presets", HTTP_GET, [](){
    StaticJsonDocument<512> doc;
    doc["active"] = activePreset;
    auto items = doc["items"].to<JsonArray>();
    Preferences prefs;
    prefBeginRead(prefs);
    for(int i=0;i<8;i++){
      String blob = prefs.getString(presetKeyFor(i), "");
      JsonObject it = items.add<JsonObject>();
      if (blob.length()){
        StaticJsonDocument<768> pd;
        DeserializationError e = deserializeJson(pd, blob);
        String face = "#EEEEEE";
        if (!e && pd["zones"][0]["colorA"].is<const char*>()){
          face = String(pd["zones"][0]["colorA"].as<const char*>());
        }
        it["color"] = face;
      } else {
        it["color"] = "#EEEEEE";
      }
    }
    prefEnd(prefs);
    String out; serializeJson(doc, out);
    server.send(200,"application/json",out);
  });

  server.on("/presetSave", HTTP_POST, [](){
        StaticJsonDocument<4096> st;
    snapshotCurrentStateToJson(st);

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400,"text/plain","Bad JSON"); return; }
    int idx = doc["idx"] | -1;
    if (idx < 0 || idx > 7){ server.send(400,"text/plain","Bad index"); return; }

    String blob; serializeJson(st, blob);
    Preferences prefs;
    prefBeginWrite(prefs);
    prefs.putString(presetKeyFor(idx), blob);
    activePreset = idx;
    prefs.putChar(PREF_ACTIVE_PRESET, activePreset);
    prefEnd(prefs);

    server.send(200,"application/json", blob);
  });
server.on("/presetApply", HTTP_POST, [](){
    if (!server.hasArg("plain")) { server.send(400,"text/plain","Missing body"); return; }
    recordActivity();
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, server.arg("plain"))){ server.send(400,"text/plain","Bad JSON"); return; }
    int idx = doc["idx"] | -1;
    if (idx < 0 || idx > 7){ server.send(400,"text/plain","Bad index"); return; }

    String blob = prefReadString(presetKeyFor(idx));
    if (!blob.length()){ server.send(404,"text/plain","No preset"); return; }

    StaticJsonDocument<4096> st;
    if (deserializeJson(st, blob)){ server.send(400,"text/plain","Corrupt preset"); return; }

    
  // Unified path: apply ALL preset fields with the same helper as GPIO3 double-click
  applyPresetJson(st);
  // Persist activePreset = idx (same behavior as internal path when persistActive=true)
  activePreset = idx;
  prefWriteChar(PREF_ACTIVE_PRESET, (char)activePreset);
  stateEpoch++; // trigger epoch-gated UI hydrate
  server.send(200, "text/plain", "OK");
  return;
});
server.on("/status",  HTTP_GET, [](){
  const bool batteryReady = (batt.count >= BAT_SAMPLES_PER_WINDOW); sendStatusJSON(); });
  server.on("/battery", HTTP_GET, [](){ server.send(200,"text/plain",String(batteryVoltage,3)); });

  
  // --- Debug: Read-only preset dump (v4c7b3) ---
  server.on("/presetDump", HTTP_GET, [](){
    if (!server.hasArg("idx")) { server.send(400,"text/plain","Missing idx"); return; }
    int idx = server.arg("idx").toInt();
    if (idx < 0 || idx > 7) { server.send(400,"text/plain","Bad idx"); return; }
    String blob = prefReadString(presetKeyFor(idx));
    if (!blob.length()) { server.send(404,"text/plain","NO_PRESET"); return; }
    server.send(200, "application/json", blob);
  });

  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });
server.begin();
  addDebug("HTTP server started");

  // Auto-apply last active preset on boot (restores full state)
  applyActivePresetOnBoot();

  wakeMillis    = millis();
  allowAutoSleep = false;
}

// ---------- Loop ----------
static bool wifiOnCached = true; // mirror (avoid linker surprises)
void loop(){
  // Process DNS requests when WiFi is on
  if (wifiOn) {
    dnsServer.processNextRequest();
  }
  
  server.handleClient();
  sampleBattery();

  // Wi-Fi inactivity handling
  if (wifiOn) {
    unsigned long now = millis();
    unsigned long idle = now - lastActivityMs;
    if (wifiIdleAutoOff && idle >= IDLE_OFF_MS) wifiDisable();
} else {
    if (digitalRead(BUTTON_PIN)==LOW) {
      wifiEnable();
      while (digitalRead(BUTTON_PIN)==LOW) delay(5); // debounce
    }
  }

  // Grace timer for low-battery actions
  if (!allowAutoSleep && (millis() - wakeMillis >= AUTO_SLEEP_GRACE_MS)){
    allowAutoSleep = true;
    addDebug("Auto-sleep allowed");
  }

  
  // --- Button double-click to cycle presets (awake only) ---
  int raw = digitalRead(BUTTON_PIN);
  unsigned long nowms = millis();
  if (raw != lastBtnState && (nowms - lastBtnChange) >= BTN_DEBOUNCE_MS){
    lastBtnChange = nowms;
    lastBtnState = raw;
    if (raw == LOW){
      shortPressStarted = nowms;
    } else {
      if (shortPressStarted && (nowms - shortPressStarted) < BUTTON_HOLD_MS){
        if (nowms - lastReleaseMs <= BTN_DBL_MS){ clickCount++; } else { clickCount = 1; }
        lastReleaseMs = nowms;
        if (clickCount >= 2){
          if (cyclePresetNext()){
            addDebug("Double-click: next preset");
          }
          clickCount = 0;
        }
      }
      shortPressStarted = 0;
    }
  }
 // Manual deep sleep via long-press
  if (digitalRead(BUTTON_PIN)==LOW) {
    if (buttonDownAt == 0) buttonDownAt = millis();
    else if (millis() - buttonDownAt >= BUTTON_HOLD_MS) {
      addDebug("Manual deep sleep requested (long press)");
      FastLED.clear(true); FastLED.show();
      while (digitalRead(BUTTON_PIN)==LOW) delay(5);
      delay(60);
      enterDeepSleepNow("manual");
    }
  } else {
    buttonDownAt = 0;
  }

  // Battery-driven deep sleep / rendering
  
  // v4c7h: Simulated low-V deep sleep hold (post-grace only)
  if (allowAutoSleep && simVbatEnabled) {
    if (simVbat <= FULL_OFF_V) {
      if (simLowSinceMs == 0UL) simLowSinceMs = millis();
      else if ((millis() - simLowSinceMs) >= 10000UL) {
        addDebug("Sim LowV -> deep sleep");
        FastLED.clear(true); FastLED.show();
        WiFi.mode(WIFI_OFF);
        delay(60);
        enterDeepSleepNow("simLowV");
      }
    } else {
      simLowSinceMs = 0;
    }
  } else {
    simLowSinceMs = 0;
  }
if (allowAutoSleep && batteryVoltage <= FULL_OFF_V){
    addDebug("Battery critical -> deep sleep");
    FastLED.clear(true); FastLED.show();
    WiFi.mode(WIFI_OFF);
    delay(60);
    enterDeepSleepNow("battery");
  } else {
    applyAllEffects();
  }

  delay(20);

}
