#ifndef PREFS_KEYS_H
#define PREFS_KEYS_H

// Centralized Preference key names (preserve exact string values).
// Use these constants in code in place of literal strings to prevent accidental typos.
// Keep the string values identical to existing uses.

static constexpr char PREF_NAMESPACE[] = "arcReactor";
static constexpr char PREF_DF_THRESH[] = "dfThresh";        // dfThreshold stored key (preserve)
static constexpr char PREF_AUTO_DF[] = "autoDF";
static constexpr char PREF_MASTER[] = "master";
static constexpr char PREF_ACTIVE_PRESET[] = "activePreset";
static constexpr char PREF_WIFI_IDLE[] = "wifiIdleAutoOff";

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

#endif // PREFS_KEYS_H