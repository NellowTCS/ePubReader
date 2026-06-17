//  oooooooooooo         ooooo ooooo      ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //

#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <vector>
#include <config.h>

// Type alias for readability
using PanelT   = GxEPD2_310_GDEQ031T10;
using DisplayT = GxEPD2_BW<PanelT, PanelT::HEIGHT>;

// E-ink display
extern DisplayT display;

// U8g2-for-Adafruit-GFX bridge, use this for ALL text rendering on e-ink
extern U8G2_FOR_ADAFRUIT_GFX u8g2f;

extern void einkHandler(void *parameter);

class PocketmageEink {
public:
  explicit PocketmageEink(DisplayT& display) : display_(display) {}

  void setLineSpacing(uint8_t lineSpacing)              { lineSpacing_ = lineSpacing; };
  void setFullRefreshAfter(uint8_t fullRefreshAfter)    { fullRefreshAfter_ = fullRefreshAfter; };

  void refresh();
  void multiPassRefresh(int passes);
  void setFastFullRefresh(bool setting);
  void statusBar(const String& input, bool fullWindow=false);
  void drawStatusBar(const String& input);
  void resetDisplay(bool clearScreen=true, uint16_t color = GxEPD_WHITE);
  int  countLines(const String& input, size_t maxLineLength = 29);

  uint8_t getFontHeight();
  int maxLines() { return display_.height() / (getFontHeight() + lineSpacing_); }
  uint16_t getEinkTextWidth(const String& s);
  uint8_t getLineSpacing() { return lineSpacing_; };
  DisplayT& getDisplay() { return display_; };
  void forceSlowFullUpdate(bool force);

private:
  DisplayT&             display_;
  bool                  forceSlowFullUpdate_  = false;
  uint8_t               partialCounter_       = 0;
  uint8_t               fullRefreshAfter_     = FULL_REFRESH_AFTER;
  uint8_t               lineSpacing_          = 6;
};

void setupEink();

PocketmageEink& EINK();
