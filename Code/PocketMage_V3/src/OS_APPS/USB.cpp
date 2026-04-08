// AUDIT 1
#include <globals.h>
#include <USB.h>
#include <USBMSC.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <driver/sdmmc_defs.h>
#if !OTA_APP // POCKETMAGE_OS
static String currentLine = "";
static constexpr const char* TAG = "USB";
static USBMSC msc;
static sdmmc_card_t* card = nullptr;     // SD card pointer

static volatile bool usb_is_shutting_down = false;

void USBAppShutdown() {
  if (!mscEnabled) return;
  
  usb_is_shutting_down = true;
  ESP_LOGI(TAG, "Shutting down USB MSC...");

  msc.mediaPresent(false);
  delay(500); 

  // Stop MSC functionality and detach from USB host
  msc.end();

  // Free card struct
  if (card) {
    free(card);
    card = nullptr;
  }

  // Deinitialize SDMMC host to clean hardware state
  sdmmc_host_deinit();

  mscEnabled = false;

  // The system is about to esp_restart(), so remounting the SD card 
  // right before a hardware reboot is dangerous and unnecessary.

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  disableTimeout = false;

  // Switch USB contol to BMS
  PowerSystem.setUSBControlBMS();
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  if (usb_is_shutting_down) return -1; // Safety Lock
  
  SDActive = true;
  if (!card || card->csd.sector_size == 0) {
    SDActive = false;
    return -1;
  }
  
  uint32_t secSize = card->csd.sector_size;
  for (uint32_t i = 0; i < bufsize / secSize; ++i) {
    esp_err_t err = sdmmc_write_sectors(card, buffer + i * secSize, lba + i, 1);
    if (err != ESP_OK) {
      SDActive = false;
      return -1;
    }
  }
  SDActive = false;
  return bufsize;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  if (usb_is_shutting_down) return -1; // Safety Lock
  
  SDActive = true;
  if (!card || card->csd.sector_size == 0) {
    SDActive = false;
    return -1;
  }
  
  uint32_t secSize = card->csd.sector_size;
  for (uint32_t i = 0; i < bufsize / secSize; ++i) {
    esp_err_t err = sdmmc_read_sectors(card, (uint8_t*)buffer + i * secSize, lba + i, 1);
    if (err != ESP_OK) {
      SDActive = false;
      return -1;
    }
  }
  SDActive = false;
  return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool eject) {
  SDActive = true;
  ESP_LOGI(TAG, "MSC Start/Stop: power=%u, start=%d, eject=%d\n", power_condition, start, eject);

  SDActive = false;
  return true;
}

static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  SDActive = true;
  if (event_base == ARDUINO_USB_EVENTS) {
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT: ESP_LOGI(TAG, "USB Connected"); break;
      case ARDUINO_USB_STOPPED_EVENT: ESP_LOGI(TAG, "USB Disconnected"); break;
      case ARDUINO_USB_SUSPEND_EVENT: ESP_LOGI(TAG, "USB Suspended"); break;
      case ARDUINO_USB_RESUME_EVENT:  ESP_LOGI(TAG, "USB Resumed"); break;
    }
  }
  SDActive = false;
}

void USB_INIT() {
  usb_is_shutting_down = false;
  
  // Switch USB contol to ESP
  PowerSystem.setUSBControlESP();

  // OPEN USB FILE TRANSFER
  OLED().oledWord("Initializing USB");
  pocketmage::setCpuSpeed(240);
  delay(50);

  disableTimeout = true;

  if (mscEnabled) return;

  ESP_LOGI(TAG, "Unmounting SD_MMC for USB MSC...");

  SD_MMC.end();  // unmount FS before raw access
  delay(200);    // Give the card hardware a moment to settle state

  // Configure SDMMC host and slot manually
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = SDMMC_FREQ_DEFAULT; // Force 20MHz for stability
  
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.clk = (gpio_num_t)SD_CLK;
  slot_config.cmd = (gpio_num_t)SD_CMD;
  slot_config.d0 = (gpio_num_t)SD_D0;
  slot_config.d1 = (gpio_num_t)0;
  slot_config.d2 = (gpio_num_t)0;
  slot_config.d3 = (gpio_num_t)0;
  slot_config.width = 1;  // or 4 for 4-bit mode
  
  // CRITICAL FIX: Force internal pull-ups to prevent ESP_ERR_TIMEOUT
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  // Initialize host
  esp_err_t err = sdmmc_host_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Host init failed %s\n", esp_err_to_name(err));
    OLED().sysMessage("Host init failed: " + String(esp_err_to_name(err)),2000);
    return;
  }

  err = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slot_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Slot init failed: %s\n", esp_err_to_name(err));
    OLED().sysMessage("Slot init failed: " + String(esp_err_to_name(err)),2000);
    sdmmc_host_deinit();
    return;
  }

  // Allocate card object and mount
  card = (sdmmc_card_t*)malloc(sizeof(sdmmc_card_t));
  if (!card) {
    ESP_LOGE(TAG, "Failed to allocate card struct\n", esp_err_to_name(err));
    OLED().sysMessage("Failed malloc: " + String(esp_err_to_name(err)),2000);
    sdmmc_host_deinit();
    return;
  }

  err = sdmmc_card_init(&host, card);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Card init failed: %s\n", esp_err_to_name(err));
    OLED().sysMessage("Card init failed: " + String(esp_err_to_name(err)),2000);
    OLED().sysMessage("Please type 'sdreset' in home",2000);
    free(card);
    card = nullptr;
    sdmmc_host_deinit();
    return;
  }

  // Setup USB MSC
  ESP_LOGI(TAG, "Initializing USB MSC...");

  msc.vendorID("ESP32");
  msc.productID("PocketMage");
  msc.productRevision("1.0");
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.onStartStop(onStartStop);
  msc.mediaPresent(true);
  msc.begin(card->csd.capacity, card->csd.sector_size);

  USB.onEvent(usbEventCallback);
  USB.begin();

  mscEnabled = true;
  delay(50);

  // INIT App
  CurrentAppState = USB_APP;
  KB().setKeyboardState(NORMAL);
  newState = true;
}

void processKB_USB() {
  int currentMillis = millis();
  char inchar = 0;

  // 1. Drain the hardware buffer continuously at loop speed
  inchar = KB().updateKeypress();

  // 2. Only process the actual input if the cooldown has expired
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    if (inchar != 0) {
      KBBounceMillis = currentMillis; // Reset the debounce timer

      // HANDLE INPUTS
      // Home / Exit / App Switcher recieved
      if (inchar == 12 || inchar == 8 || inchar == 19 || inchar == 28 || inchar == 23) {
        USBAppShutdown();
        prefs.begin("PocketMage", false);
        prefs.putInt("CurrentAppState", static_cast<int>(HOME));
        prefs.putBool("Seamless_Reboot", true);
        prefs.end();
        esp_restart();
      }
    }
  }

  // 3. Update OLED at 10FPS, completely independent of keyboard bounce
  currentMillis = millis();
  if (currentMillis - OLEDFPSMillis >= (1000/10 /*OLED_MAX_FPS*/)) {
    OLEDFPSMillis = currentMillis;
    OLED().oledLine(currentLine, currentLine.length(), false);
  }
}

void einkHandler_USB() {
  if (newState) {
    newState = false;
    
    display.fillScreen(GxEPD_WHITE);

    // Display Status Bar
    EINK().drawStatusBar("Connect to a Computer:");

    // Display Background
    display.drawBitmap(0, 0, _usb, 320, 218, GxEPD_BLACK);

    EINK().multiPassRefresh(2);
  }
}
#endif