#ifndef PREFS_KEYS_H
#define PREFS_KEYS_H

#include <Preferences.h>

// Centralized Preference key names (preserve exact string values).
// Use these constants in code in place of literal strings to prevent accidental typos.
// Keep the string values identical to existing uses.

static constexpr char PREF_NAMESPACE[] = "arcReactor";
static constexpr char PREF_DF_THRESH[] = "dfThresh";        // dfThreshold stored key (preserve)
static constexpr char PREF_AUTO_DF[] = "autoDF";
static constexpr char PREF_MASTER[] = "master";
static constexpr char PREF_ACTIVE_PRESET[] = "activePreset";
static constexpr char PREF_WIFI_IDLE[] = "wifiIdleAutoOff";
static constexpr char PREF_AP_SSID[] = "apSSID";            // Access Point SSID
static constexpr char PREF_AP_PASS[] = "apPass";            // Access Point password

// Presets: "preset0".."preset7"
static inline const char* presetKeyFor(int idx) {
  static char tmp[16];
  snprintf(tmp, sizeof(tmp), "preset%d", idx);
  return tmp; // careful: caller must not rely on pointer across calls if used concurrently
}

// Favorites keys: "fav0".."fav8"
static inline const char* favKeyFor(int idx) {
  static char tmp[16];
  snprintf(tmp, sizeof(tmp), "fav%d", idx);
  return tmp;
}

// ===================================================================
// PrefsManager: Centralized preference read/write helper functions
// ===================================================================
// These wrapper functions reduce code duplication and centralize the
// begin/end pattern, making the codebase more maintainable.
//
// Usage examples:
//   prefReadString("myKey", "default")
//   prefWriteUInt("myKey", 123)
//   prefReadFloat("voltage", 3.6f)
// ===================================================================

// Read operations (read-only mode)
static inline String prefReadString(const char* key, const String& defaultValue = "") {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);  // read-only
  String value = prefs.getString(key, defaultValue);
  prefs.end();
  return value;
}

static inline uint32_t prefReadUInt(const char* key, uint32_t defaultValue = 0) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  uint32_t value = prefs.getUInt(key, defaultValue);
  prefs.end();
  return value;
}

static inline bool prefReadBool(const char* key, bool defaultValue = false) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  bool value = prefs.getBool(key, defaultValue);
  prefs.end();
  return value;
}

static inline float prefReadFloat(const char* key, float defaultValue = 0.0f) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  float value = prefs.getFloat(key, defaultValue);
  prefs.end();
  return value;
}

static inline int8_t prefReadChar(const char* key, int8_t defaultValue = 0) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  int8_t value = prefs.getChar(key, defaultValue);
  prefs.end();
  return value;
}

static inline uint8_t prefReadUChar(const char* key, uint8_t defaultValue = 0) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  uint8_t value = prefs.getUChar(key, defaultValue);
  prefs.end();
  return value;
}

// Write operations (read-write mode)
static inline void prefWriteString(const char* key, const String& value) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);  // read-write
  prefs.putString(key, value);
  prefs.end();
}

static inline void prefWriteUInt(const char* key, uint32_t value) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putUInt(key, value);
  prefs.end();
}

static inline void prefWriteBool(const char* key, bool value) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putBool(key, value);
  prefs.end();
}

static inline void prefWriteFloat(const char* key, float value) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat(key, value);
  prefs.end();
}

static inline void prefWriteChar(const char* key, int8_t value) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putChar(key, value);
  prefs.end();
}

static inline void prefWriteUChar(const char* key, uint8_t value) {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putUChar(key, value);
  prefs.end();
}

// Batch operations for efficiency (when multiple keys need to be read/written)
// These allow the caller to manage prefs.begin/end for better performance
static inline void prefBeginRead(Preferences& prefs) {
  prefs.begin(PREF_NAMESPACE, true);
}

static inline void prefBeginWrite(Preferences& prefs) {
  prefs.begin(PREF_NAMESPACE, false);
}

static inline void prefEnd(Preferences& prefs) {
  prefs.end();
}

// Deprecated direct access (for backward compatibility during transition)
// Use the pref* helper functions above instead for new code
#define DEPRECATED_PREFS_DIRECT_ACCESS 1

#endif // PREFS_KEYS_H