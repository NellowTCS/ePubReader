#include <globals.h>

// SPI busses
SPIClass *vspi = NULL;
SPIClass *hspi = NULL;

// Filesystem
fs::FS* global_fs = nullptr;

// NVS preferences
Preferences prefs;

// App state
AppState CurrentAppState = HOME;

// OTA app names
String OTA1_APP, OTA2_APP, OTA3_APP, OTA4_APP;

// Persistent settings (defaults)
int   TIMEOUT         = 120;
bool  DEBUG_VERBOSE   = false;
bool  SYSTEM_CLOCK    = true;
bool  SHOW_YEAR       = true;
bool  SAVE_POWER      = false;
bool  ALLOW_NO_MICROSD = true;
bool  HOME_ON_BOOT    = false;
int   OLED_BRIGHTNESS = 128;
int   OLED_MAX_FPS    = 30;
bool  SD_SPI_COMPATIBILITY = false;

// Other globals
volatile bool newState         = false;
volatile bool disableTimeout   = false;
bool          fileLoaded       = false;
unsigned int  flashMillis      = 0;
int           OLEDFPSMillis    = 0;
int           KBBounceMillis   = 0;

// App state names (unused in OTA app but referenced by library)
const String appStateNames[] = { "", "", "", "", "", "", "", "", "", "", "" };

// Functions called by PocketMage library / PocketMage_INIT
void loadState(bool) {}
void checkCrashState() {}
void checkRTCPowerLoss() {}


