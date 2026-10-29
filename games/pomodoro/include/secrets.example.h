// Copy to include/secrets.h (git-ignored) and fill in. Without it the build uses this file,
// the WiFi stays off and the clock never syncs; the focus timer works either way.
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
// POSIX TZ string; Brasília is UTC-3 with no daylight saving
#define TIMEZONE "<-03>3"
