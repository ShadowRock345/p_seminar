#include <Arduino.h>
#include <WiFi.h>
#include "SH1106Wire.h"
#include <Wire.h> 
#include "OLEDDisplayUi.h"
#include "images.h"
#include <esp_now.h>
#include "server-extended.h"

SH1106Wire display(0x3c, SDA, SCL);

OLEDDisplayUi ui     ( &display );

const char* ssid = "Babapapa2"; // SSID des Access Points
const char* password = "onlY fOr My! EYES"; // Passwort des Access Points

IPAddress ip(192, 168, 178, 45);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);

int screenW = 128;
int screenH = 64;
int oledcenterx = screenW / 2;
int oledcentery = ((screenH - 16) / 2) + 16;

String nom = "Master";
bool sendCmd = false;
String slaveCmd = "0";
String slaveState = "0";

WiFiServer server(80);
WiFiClient browser;

#define NUM_SLAVES 1
#define LED 2

void drawProgressBar(int counter) {
  int progress = (counter / 5) % 100;
  // draw the progress bar
  display.drawProgressBar(0, 32, 120, 10, progress);

  // draw the percentage as String
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 15, String(progress) + "%");

}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis()));
}

bool shouldExitRandomly() {
  // Generiere eine Zufallszahl zwischen 0 und 9
  int randomNumber = random(500);

  // Wenn die Zufallszahl kleiner oder gleich 2 ist, beende die Schleife
  return randomNumber <= 2;
}

bool shouldExitRandomly2() {
  // Generiere eine Zufallszahl zwischen 0 und 9
  int randomNumber = random(700);

  // Wenn die Zufallszahl kleiner oder gleich 2 ist, beende die Schleife
  return randomNumber <= 2;
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  //display->setFont(ArialMT_Plain_20);
  display->drawString(0 + x, 10 + y , "ESPNOW");
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

FrameCallback frames[] = { drawFrame1, drawFrame2};

int frameCount = 2;

OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;
String usedmode = "";

uint8_t broadcastAddress[] = {0xA8, 0x42, 0xE3, 0xCF, 0x02, 0xE0};

typedef struct struct_message {
    float id;
    String msg;
} struct_message;

struct_message incomingReadings;

struct_message Valuestosend;

esp_now_peer_info_t peerInfo;

String success = "";

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

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

void setup() {
 
  ui.setTargetFPS(60);
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.setOverlays(overlays, overlaysCount);
  ui.init();
  delay(1000);
  randomSeed(analogRead(1));
  Serial.begin(115200);
  Serial.println(F("Initialize System"));
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  while(true) {
    if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    if (input == "wifi") {
        usedmode = "wifi";
        // Konfiguration des ESP32 als Access Point
        WiFi.config(ip, gateway, subnet);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(F("."));
        }
        server.begin();
        Serial.print(nom);
        Serial.print(F(" connected to Wifi! IP address : http://")); 
        Serial.println(WiFi.localIP()); // Print the IP address
        break;
    } else if (input == "espnow") {
      usedmode = "espnow";
      WiFi.mode(WIFI_STA);
      if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        esp_now_register_send_cb(OnDataSent);
        memcpy(peerInfo.peer_addr, broadcastAddress, 6);
        peerInfo.channel = 0;  
        peerInfo.encrypt = false; //possibility to encrypt espnow  
        
        
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
  
  
  
  
  for(int i = 0; i <= 100; i++) {
    display.clear();
    drawProgressBar(i);
    display.display();
    delay(100);

    if (shouldExitRandomly()) {
      break; 
    }
  }
}

int t = 0;

void loop() {
  //if (shouldExitRandomly2()) {
  //    t = 1;
 // }

  //if (t == 0) {
  //  int remainingTimeBudget = ui.update();
  //  if (remainingTimeBudget > 0) {
  //    delay(remainingTimeBudget);
  //  }
 // } else {
 //   clientRequest();
  //}
  clientRequest();
  
}

void clientRequest( ) { /* function clientRequest */
  ////Check if client connected
  if (usedmode == "wifi") {
    WiFiClient client = server.available();
    client.setTimeout(50);
    if (client) {
      if (client.connected()) {
        //Print client IP address
        Serial.print(" ->");Serial.println(client.remoteIP());
        String request = client.readStringUntil('\r'); //receives the message from the client
        
        if (request.indexOf("Slave0") == 0) {
          //Handle slave request
          Serial.print("From "); Serial.println(request);
          displayMultilineText("From: " + request);
          int index = request.indexOf(":");
          String slaveid = request.substring(0, index);
          slaveState = request.substring(request.indexOf("x") + 1, request.length());
          Serial.print("state received: "); Serial.println(slaveState);
          client.print(nom);
          client.println(": Ok " + slaveid + "! Statement: x" + " :)" + "\r");
          client.stop();                // terminates the connection with the client
        } else {
          Serial.print("From Browser : "); Serial.println(request);
        
          handleLEDRequest(request);
          webpage(client);
        }
      }
    }
  } else if (usedmode == "espnow") {
    Valuestosend.id = random(1, 101);

    // Aktuelle Zeit in Millisekunden seit dem Start des Programms
    unsigned long currentTime = millis();

    // Zeit in Stunden, Minuten und Sekunden
    int hours = (currentTime / 3600000) % 24;
    int minutes = (currentTime / 60000) % 60;
    int seconds = (currentTime / 1000) % 60;

    Valuestosend.msg = "t";//String(hours) + ":" + String(minutes) + ":" + String(seconds);

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &Valuestosend, sizeof(Valuestosend));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
  
}

void handleLEDRequest(String request) {
  // Behandle die Anfragen für die LED-Koordinaten
  
    String req = request;
    Serial.println("Received request: " + req);
    if (req.indexOf("/setLED") != -1) {
    int indexRow = req.indexOf("row=") + 4;
    int indexCol = req.indexOf("&col=");
    int row = req.substring(indexRow, indexCol).toInt();
    int col = req.substring(indexCol + 5).toInt();

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);

    int ledSize = 10; // Größe einer LED
    int padding = 2; // Abstand zwischen den LEDs

    // Zeichne die LEDs auf dem Display entsprechend ihrer Position in der Matrix
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 10; j++) {
        int xPos = (j * (ledSize + padding)) + padding; // X-Position der aktuellen LED
        int yPos = (i * (ledSize + padding)) + padding; // Y-Position der aktuellen LED

        // Prüfe, ob die aktuelle LED die ausgewählte LED ist
        if (i == row && j == col) {
          display.setColor(BLACK); // Setze die Farbe auf Rot für die ausgewählte LED
        } else {
          display.setColor(WHITE); // Setze die Farbe auf Grün für andere LEDs
        }

        // Zeichne ein Rechteck als LED
        display.fillRect(xPos, yPos, ledSize, ledSize);
      }
    }

    display.display();
  }
  
}

const char* webpage2 = R"(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 LED Matrix Control</title>
  <script>
    function toggleLED(row, col) {
      // Funktion, um die gedrückte LED zu markieren und die Koordinaten an den ESP32 zu senden
      let selectedLED = document.getElementById('led_' + row + '_' + col);
      selectedLED.style.backgroundColor = selectedLED.style.backgroundColor === 'rgb(255, 255, 255)' ? '#000' : '#FFF'; // Ändere die Farbe um, um zu markieren

      // Sende die Koordinaten an den ESP32
      let xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          // Wenn die Koordinaten erfolgreich gesendet wurden, zeige die Bestätigung auf der Konsole an
          console.log("LED-Koordinaten gesendet: " + row + ", " + col);
        }
      };
      xhttp.open("GET", "/setLED?row=" + row + "&col=" + col, true); // Sende die Koordinaten an den ESP32
      xhttp.send();
    }
  </script>
</head>
<body>
  <h2> LED Matrix Simulation </h2>
  <center>
    <table border='1'>
      <!-- Hier fügst du den Code für die LED-Matrix ein -->
      <!-- Beispiel für eine 4x4 LED-Matrix -->
      <tr>
        <td id='led_0_0' onclick='toggleLED(0, 0)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_1' onclick='toggleLED(0, 1)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_2' onclick='toggleLED(0, 2)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_3' onclick='toggleLED(0, 3)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_4' onclick='toggleLED(0, 4)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_5' onclick='toggleLED(0, 5)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_6' onclick='toggleLED(0, 6)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_7' onclick='toggleLED(0, 7)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_8' onclick='toggleLED(0, 8)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_0_9' onclick='toggleLED(0, 9)' style='width: 20px; height: 20px; background-color: #000;'></td>
      </tr>
      <tr>
        <td id='led_1_0' onclick='toggleLED(1, 0)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_1' onclick='toggleLED(1, 1)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_2' onclick='toggleLED(1, 2)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_3' onclick='toggleLED(1, 3)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_4' onclick='toggleLED(1, 4)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_5' onclick='toggleLED(1, 5)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_6' onclick='toggleLED(1, 6)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_7' onclick='toggleLED(1, 7)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_8' onclick='toggleLED(1, 8)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_1_9' onclick='toggleLED(1, 9)' style='width: 20px; height: 20px; background-color: #000;'></td>
      </tr>
      <tr>
        <td id='led_2_0' onclick='toggleLED(2, 0)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_1' onclick='toggleLED(2, 1)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_2' onclick='toggleLED(2, 2)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_3' onclick='toggleLED(2, 3)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_4' onclick='toggleLED(2, 4)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_5' onclick='toggleLED(2, 5)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_6' onclick='toggleLED(2, 6)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_7' onclick='toggleLED(2, 7)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_8' onclick='toggleLED(2, 8)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_2_9' onclick='toggleLED(2, 9)' style='width: 20px; height: 20px; background-color: #000;'></td>
      </tr>
      <tr>
        <td id='led_3_0' onclick='toggleLED(3, 0)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_1' onclick='toggleLED(3, 1)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_2' onclick='toggleLED(3, 2)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_3' onclick='toggleLED(3, 3)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_4' onclick='toggleLED(3, 4)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_5' onclick='toggleLED(3, 5)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_6' onclick='toggleLED(3, 6)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_7' onclick='toggleLED(3, 7)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_8' onclick='toggleLED(3, 8)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_3_9' onclick='toggleLED(3, 9)' style='width: 20px; height: 20px; background-color: #000;'></td>
      </tr>
      <tr>
        <td id='led_4_0' onclick='toggleLED(4, 0)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_1' onclick='toggleLED(4, 1)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_2' onclick='toggleLED(4, 2)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_3' onclick='toggleLED(4, 3)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_4' onclick='toggleLED(4, 4)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_5' onclick='toggleLED(4, 5)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_6' onclick='toggleLED(4, 6)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_7' onclick='toggleLED(4, 7)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_8' onclick='toggleLED(4, 8)' style='width: 20px; height: 20px; background-color: #000;'></td>
        <td id='led_4_9' onclick='toggleLED(4, 9)' style='width: 20px; height: 20px; background-color: #000;'></td>
      </tr>
    </table>
  </center>
</body>
</html>
)";

void webpage(WiFiClient browser) { /* function webpage */
  ////Send webpage to browser
  browser.println("HTTP/1.1 200 OK");
  browser.println("Content-Type: text/html");
  browser.println(""); // Leere Zeile notwendig
  browser.println(webpage2);
  delay(1);
}
