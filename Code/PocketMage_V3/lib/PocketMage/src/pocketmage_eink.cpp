//  oooooooooooo         ooooo ooooo       ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //

#include <pocketmage.h>

static constexpr const char* tag = "EINK";

GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>
  display(GxEPD2_310_GDEQ031T10(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

U8G2_FOR_ADAFRUIT_GFX u8g2f;

TaskHandle_t einkHandlerTaskHandle = NULL;

volatile bool GxEPD2_310_GDEQ031T10::useFastFullUpdate = true;

static PocketmageEink pm_eink(display);

PocketmageEink& EINK() { return pm_eink; }

void PocketmageEink::refresh() {
  if ((partialCounter_ >= fullRefreshAfter_) || forceSlowFullUpdate_) {
    forceSlowFullUpdate_ = false;
    partialCounter_ = 0;
    setFastFullRefresh(false);
  } else {
    setFastFullRefresh(true);
    partialCounter_++;
  }
  display_.display(false);
  display_.setFullWindow();
  display_.fillScreen(GxEPD_WHITE);
  display_.hibernate();
}

void PocketmageEink::multiPassRefresh(int passes) {
  #if POCKETMAGE_HW_VERSION == 2
    EINK().refresh();
  #else
    display_.display(false);
    if (passes > 0) {
      for (int i = 0; i < passes; i++) {
        delay(250);
        display_.display(true);
      }
    }
    delay(100);
    display_.setFullWindow();
    display_.fillScreen(GxEPD_WHITE);
    display_.hibernate();
  #endif
}

void PocketmageEink::setFastFullRefresh(bool setting) {
  if (PanelT::useFastFullUpdate != setting) {
    PanelT::useFastFullUpdate = setting;
  }
}

void PocketmageEink::statusBar(const String& input, bool fullWindow) {
  u8g2f.setFont(u8g2_font_courB10_tf);
  u8g2f.setForegroundColor(GxEPD_BLACK);
  u8g2f.setBackgroundColor(GxEPD_WHITE);
  u8g2f.setFontMode(1);
  if (!fullWindow){
    display_.setPartialWindow(0, display_.height() - 20, display_.width(), 20);
    display_.fillRect(0, display_.height() - 26, display_.width(), 26, GxEPD_WHITE);
    display_.drawRect(0, display_.height() - 20, display_.width(), 20, GxEPD_BLACK);
    u8g2f.setCursor(4, display_.height() - 6);
    u8g2f.print(input);
  }
  display_.drawRect(display_.width() - 30, display_.height() - 20, 30, 20, GxEPD_BLACK);
}

void PocketmageEink::drawStatusBar(const String& input) {
  display_.fillRect(0, display_.height() - 26, display_.width(), 26, GxEPD_WHITE);
  display_.drawRect(0, display_.height() - 20, display_.width(), 20, GxEPD_BLACK);
  u8g2f.setFont(u8g2_font_courB10_tf);
  u8g2f.setForegroundColor(GxEPD_BLACK);
  u8g2f.setBackgroundColor(GxEPD_WHITE);
  u8g2f.setFontMode(1);
  u8g2f.setCursor(4, display_.height() - 6);
  u8g2f.print(input);
}

void PocketmageEink::resetDisplay(bool clearScreen, uint16_t color) {
  display_.setRotation(3);
  display_.setFullWindow();
  if (clearScreen) display_.fillScreen(color);
}

int PocketmageEink::countLines(const String& input, size_t maxLineLength) {
  size_t inputLength = input.length();
  uint8_t charCounter = 0;
  uint16_t lineCounter = 1;
  for (size_t c = 0; c < inputLength; c++) {
    if (input[c] == '\n') {
        charCounter = 0;
        lineCounter++;
        continue;
    }
    if (charCounter > (maxLineLength - 1)) {
        charCounter = 0;
        lineCounter++;
    }
    charCounter++;
  }
  return lineCounter;
}

void PocketmageEink::forceSlowFullUpdate(bool force) { forceSlowFullUpdate_ = force; }

void setupEink() {
  display.init(115200);
  display.setRotation(3);
  display.setFullWindow();
  u8g2f.begin(display);
  u8g2f.setForegroundColor(GxEPD_BLACK);
  u8g2f.setBackgroundColor(GxEPD_WHITE);
  u8g2f.setFontMode(1);
  display.fillScreen(GxEPD_WHITE);

  xTaskCreatePinnedToCore(
    einkHandler,
    "einkHandlerTask",
    10000,
    NULL,
    1,
    &einkHandlerTaskHandle,
    0
  );
}

uint8_t PocketmageEink::getFontHeight() {
  return u8g2f.getFontAscent() - u8g2f.getFontDescent();
}

uint16_t PocketmageEink::getEinkTextWidth(const String& s) {
  return u8g2f.getUTF8Width(s.c_str());
}
