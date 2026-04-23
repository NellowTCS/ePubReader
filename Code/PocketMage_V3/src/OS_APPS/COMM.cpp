#if !OTA_APP_FLAG

#include <globals.h>
#include <WiFi.h>
#include <esp_wifi.h>

extern "C" {
  #include "mesh_now.h"
  #include "message_queue.h"
}

char* hostMac =          "00:00:00:00:00:00";
char* currentDirectMAC = "00:1A:2B:3C:4D:5E";
int currentPeers = 0;

static constexpr const char* TAG = "COMM";

enum CommState { PEER_LIST, DIRECT_CHAT, LOCAL_CHAT };
CommState CurrentCommState = PEER_LIST;

#pragma region Messaging Setup
struct message {
  int timestamp = 0;                        // Unix timestamp of send/receive
  bool sentByLocal = false;                 // Was this message sent my me?
  char* senderMac   = "00:00:00:00:00:00";  // MAC address of sender
  char* receiverMac = "00:00:00:00:00:00";  // MAC address of receiver
  bool isRead = false;                      // Has message been read?
  char* Content = "";                       // Message content
}

#pragma region Helpers
String checkContactsForMac(String mac) {
  // Check contacts to see if Mac address is assiciated with any of them.

  // Temp
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

  // Placeholder OLED UI

  KB().setKeyboardState(FUNC);
  
  // 1. Drain the hardware buffer continuously at loop speed
  inchar = KB().updateKeypress();

  // 2. Only process the actual input if the cooldown has expired
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    if (inchar != 0) {
      KBBounceMillis = currentMillis;

      //BKSP Recieved
      if (inchar == 127 || inchar == 8 || inchar == 12) {
        HOME_INIT();
      }
      // Change state (center button)
      else if (inchar == 29 || inchar == 7 || inchar == 25) {
        if (CurrentCommState == PEER_LIST) CurrentCommState = DIRECT_CHAT;
        else if (CurrentCommState == DIRECT_CHAT) CurrentCommState = LOCAL_CHAT;
        else if (CurrentCommState == LOCAL_CHAT) CurrentCommState = PEER_LIST;
        newState = true;
      }
    }
  }

  // 3. Update OLED at true OLED_MAX_FPS, completely independent of keyboard bounce
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
      // Home / Peer List
      case PEER_LIST:
        display.drawRect(0,0,display.width(),20,GxEPD_BLACK);
        display.drawFastVLine(162,0,20,GxEPD_BLACK);
        display.drawFastVLine(266,0,20,GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);

        display.setCursor(4,16);
        display.print("Select Room:");

        // Display my MAC
        display.setFont(&Font5x7Fixed);
        display.setCursor(164,14);
        display.print("ME " + hostMac);

        // Display peers
        display.setCursor(270,14);
        display.print("Peers: " + String(currentPeers));
        break;

      // Chat
      case DIRECT_CHAT:
      case LOCAL_CHAT:
        // Draw Top menu bar
        display.drawRect(0,0,display.width(),20,GxEPD_BLACK);
        display.drawFastVLine(162,0,20,GxEPD_BLACK);
        display.drawFastVLine(266,0,20,GxEPD_BLACK);
        display.setFont(&FreeSans9pt7b);

        // Direct message
        if (CurrentCommState == DIRECT_CHAT) {
          // Display contact
          String contact = checkContactsForMac(currentDirectMAC);
          display.setCursor(4,16);
          if (contact != "_NULL_")  display.print("["+contact+"]");
          else                      display.print("[Unknown]");

          // Display connected MAC
          display.setFont(&Font5x7Fixed);
          display.setCursor(164,14);
          display.print("TO " + currentDirectMAC);
        }
        // Local chat
        else {
          display.setCursor(4,16);
          display.print("Local: Public Chat");

          // Display my MAC
          display.setFont(&Font5x7Fixed);
          display.setCursor(164,14);
          display.print("ME " + hostMac);
        }

        // Display peers
        display.setFont(&Font5x7Fixed);
        display.setCursor(270,14);
        display.print("Peers: " + String(currentPeers));

        // Display messages
        //for() {}

        break;
    }

    EINK().refresh();
  }
}

#endif