#if !OTA_APP_FLAG

#include <globals.h>
#include <WiFi.h>
#include <esp_wifi.h>

extern "C" {
  #include "mesh_now.h"
  #include "message_queue.h"
}

String hostMac =          "00:00:00:00:00:00";
String currentDirectMAC = "00:1A:2B:3C:4D:5E";
int currentPeers = 0;

static constexpr const char* TAG = "COMM";

enum CommState { PEER_LIST, DIRECT_CHAT, LOCAL_CHAT };
CommState CurrentCommState = PEER_LIST;

enum MsgStyle { ESP_NOW, MESHTASTIC };
MsgStyle CurrentMsgStyle = ESP_NOW;

#pragma region Messaging Setup
struct message {
  int timestamp = 0;                        
  bool sentByLocal = false;                 
  String senderMac   = "00:00:00:00:00:00"; 
  String receiverMac = "00:00:00:00:00:00"; 
  bool isRead = false;                      
  String Content = "";                       
};

#pragma region Helpers
String checkContactsForMac(String mac) {
  if (mac == "00:1A:2B:3C:4D:5E") return "Chris";
  else return "_NULL_";
}


void COMM_INIT() {
  setCpuFrequencyMhz(240); 
  CurrentAppState = COMM;
  CurrentCommState = PEER_LIST;
  KB().setKeyboardState(NORMAL);
  newState = true;
}

void processKB_COMM() {
  int currentMillis = millis();
  char inchar = 0;

  KB().setKeyboardState(FUNC);
  
  inchar = KB().updateKeypress();

  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    if (inchar != 0) {
      KBBounceMillis = currentMillis;

      if (inchar == 127 || inchar == 8 || inchar == 12) {
        HOME_INIT();
      }
      else if (inchar == 29 || inchar == 7 || inchar == 25) {
        if (CurrentCommState == PEER_LIST) CurrentCommState = DIRECT_CHAT;
        else if (CurrentCommState == DIRECT_CHAT) CurrentCommState = LOCAL_CHAT;
        else if (CurrentCommState == LOCAL_CHAT) CurrentCommState = PEER_LIST;
        newState = true;
      }
    }
  }

  currentMillis = millis();
  if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
    OLEDFPSMillis = currentMillis;
      OLED().oledWord("Chat");
  }
}

void einkHandler_COMM() {
  if (newState) {
    newState = false;
  
    switch (CurrentCommState) {
      case PEER_LIST:
        display.drawRect(0,0,display.width(),20,GxEPD_BLACK);
        display.drawFastVLine(162,0,20,GxEPD_BLACK);
        display.drawFastVLine(266,0,20,GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);

        display.setCursor(4,16);
        display.print("Select Room:");

        display.setFont(&Font5x7Fixed);
        display.setCursor(164,14);
        display.print("ME " + hostMac);

        display.setCursor(270,14);
        display.print("Peers: " + String(currentPeers));
        break;

      case DIRECT_CHAT:
      case LOCAL_CHAT:
        display.drawRect(0,0,display.width(),20,GxEPD_BLACK);
        display.drawFastVLine(162,0,20,GxEPD_BLACK);
        display.drawFastVLine(266,0,20,GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);

        if (CurrentCommState == DIRECT_CHAT) {
          String contact = checkContactsForMac(currentDirectMAC);
          display.setCursor(4,16);
          if (contact != "_NULL_")  display.print(">" + contact);
          else                      display.print(">" + currentDirectMAC);

          display.setFont(&Font5x7Fixed);
          display.setCursor(164,14);
          if (CurrentMsgStyle == ESP_NOW) display.print("ESP-NOW");
          else if (CurrentMsgStyle == MESHTASTIC) display.print("Meshtastic");
        }
        else {
          display.setCursor(4,16);
          display.print("Local: Public Chat");

          display.setFont(&Font5x7Fixed);
          display.setCursor(164,14);
          if (CurrentMsgStyle == ESP_NOW) display.print("ESP-NOW");
          else if (CurrentMsgStyle == MESHTASTIC) display.print("Meshtastic");
        }

        display.setFont(&Font5x7Fixed);
        display.setCursor(270,14);
        display.print("Peers: " + String(currentPeers));

        break;
    }

    EINK().refresh();
  }
}

#endif