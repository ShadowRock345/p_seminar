#include <WiFi.h>
#include <esp_now.h>
#include "SH1106Wire.h"
#include <Wire.h> 
#include <Arduino.h>

SH1106Wire display(0x3c, SDA, SCL);

String usedmode = "";

uint8_t broadcastAddress[] = {0xA8, 0x42, 0xE3, 0xCF, 0x07, 0x78};

typedef struct struct_message {
    float id;
    String msg;
} struct_message;

struct_message incomingReadings;

int screenW = 128;
int screenH = 64;
int oledcenterx = screenW / 2;
int oledcentery = ((screenH - 16) / 2) + 16;

//Constants
#define LED 2
#define UPDATE_TIME 500

//Parameters
String nom = "Slave0";
const char* ssid = "Babapapa2";
const char* password = "onlY fOr My! EYES";

//Variables
String command;
unsigned long previousRequest = 0;

//Objects
WiFiClient master;
IPAddress server(192, 168, 178, 45);

float incomingid;
String incomingmsg;

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingid = incomingReadings.id;
  incomingmsg = incomingReadings.msg;
  Serial.println(incomingid);
  Serial.println(incomingmsg);
}

esp_now_peer_info_t peerInfo;

void setup() {
  //Init Serial USB
  delay(1000);
  randomSeed(analogRead(1));
  Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  Serial.println(F("Initialize System"));
  //Init ESP32 Wifi
  while(true) {
    if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    if (input == "wifi") {
        usedmode = "wifi";
        // Konfiguration des ESP32 als Access Point
        
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(F("."));
        }
        
        Serial.print(nom);
        Serial.print(F(" connected to Wifi! IP address : http://")); 
        Serial.println(WiFi.localIP()); // Print the IP address
        break;
    } else if (input == "espnow") {
      usedmode = "espnow";
      WiFi.mode(WIFI_STA);
      if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        //esp_now_register_send_cb(OnDataSent);
        memcpy(peerInfo.peer_addr, broadcastAddress, 6);
        peerInfo.channel = 0;  
        peerInfo.encrypt = false;
        
        
        if (esp_now_add_peer(&peerInfo) != ESP_OK){
          Serial.println("Failed to add peer");
          return;
        }
        
        esp_now_register_recv_cb(OnDataRecv);
        return;
      }
      break;
    } else {
      Serial.println("unknown mode!");
    }
  }
  }
}

void loop() {
  requestMaster();
}

void displayMultilineText(String text) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  int lineHeight = 12; // Höhe einer Zeile
  int lines = text.length() / 24; // Annahme: Zeilenlänge beträgt 16 Zeichen für ein 128x64 Display
  
  // Zeige den Text in mehreren Zeilen an
  for (int i = 0; i <= lines; i++) {
    int startIndex = i * 24;
    int endIndex = min((i + 1) * 24, (int) text.length());
    String line = text.substring(startIndex, endIndex);
    display.drawString(oledcenterx, oledcentery + (i - lines / 2) * lineHeight, line);
  }
  display.display();
}


void requestMaster( ) { /* function requestMaster */
  ////Request to master
  if (usedmode="wifi") {
    if ((millis() - previousRequest) > UPDATE_TIME) { // client connect to server every 500ms
    previousRequest = millis();

    if (master.connect(server, 80)) { // Connection to the server
        master.println(nom + ": Hello! I am a slave talking to the master" + "\r");

        //answer
        String answer = master.readStringUntil('\r');   // receives the answer from the sever
        master.flush();
        Serial.println("from " + answer);
        if (answer.indexOf("x") >= 0) {
          command = answer.substring(answer.indexOf("x") + 1, answer.length());
          Serial.print("command received: "); Serial.println(command);
          displayMultilineText("From: " + answer);
        }
      }
    }
  }
  
}



