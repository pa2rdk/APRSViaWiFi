/////////////////////////////////////////////////////////////////////////////////////////
// V1.8 Beacon Update
// V1.7 Verbeterde WiFi recovery
// V1.5 Webserver implemented including Google Maps
// V1.4
//
// LibAPRS from :https://codeload.github.com/tomasbrincil/LibAPRS-esp32/zip/refs/heads/master
//
//  *********************************
//  **   Display connections       **
//  *********************************
//  |------------|------------------|
//  |Display 2.8 |      ESP32       |
//  |  ILI9341   |                  |
//  |------------|------------------|
//  |   Vcc      |     3V3          |
//  |   GND      |     GND          |
//  |   CS       |     15           |
//  |   Reset    |      4           |
//  |   D/C      |      2           |
//  |   SDI      |     23           |
//  |   SCK      |     18           |
//  |   LED Coll.|     14 2K        |
//  |   SDO      |                  |
//  |   T_CLK    |     18           |
//  |   T_CS     |      5           |
//  |   T_DIN    |     23           |
//  |   T_DO     |     19           |
//  |   T_IRQ    |     34           |
//  |------------|------------------|
/////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>  // https://github.com/Bodmer/TFT_eSPI
#include <WiFi.h>
#include <WifiMulti.h>
#include <EEPROM.h>
#include "NTP_Time.h"
#include <TinyGPS++.h>
#include <LibAPRS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>
#include <HardwareSerial.h>
#include "esp_adc_cal.h"
#include <RDKOTA.h>
#include <TJpg_Decoder.h>

#define offsetEEPROM 32
#define EEPROM_SIZE 2048
#define VERSION "PA2RDK_IGATE_TCP"
#define INFO "Arduino PARDK IGATE"
#define WDT_TIMEOUT 30

#define RXD1 39
#define TXD1 -1
#define DISPLAYLEDPIN 14
#define BEEPPIN 32

#define TFT_GREY 0x5AEB
#define TFT_LIGTHYELLOW 0xFF10
#define TFT_DARKBLUE 0x016F
#define TFT_SHADOW 0xE71C
#define TFT_BUTTONTOPCOLOR 0xB5FE

#define OTAHOST      "https://www.rjdekok.nl/Updates/APRSViaWiFi"
#define OTAVERSION   "v1.8"

#include "NotoSansBold15.h"
#include "NotoSansBold36.h"
#define AA_FONT_LARGE NotoSansBold15
#define AA_FONT_LARGE NotoSansBold36

#define NEEDLE_LENGTH 35  // Visible length
#define NEEDLE_WIDTH   5  // Width of needle - make it an odd number
#define NEEDLE_RADIUS 90  // Radius at tip
#define NEEDLE_COLOR1 TFT_MAROON  // Needle periphery colour
#define NEEDLE_COLOR2 TFT_RED     // Needle centre colour
#define DIAL_CENTRE_X 160
#define DIAL_CENTRE_Y 120

//#define DebugEnabled
#ifdef DebugEnabled
#define DebugPrint(x) Serial.print(x)
#define DebugPrintln(x) Serial.println(x)
#define DebugPrintf(x, ...) Serial.printf(x, __VA_ARGS__)
#else
#define DebugPrint(x)
#define DebugPrintln(x)
#define DebugPrintf(x, ...)
#endif

typedef struct {  // WiFi Access
  const char *SSID;
  const char *PASSWORD;
} wlanSSID;

typedef struct {
  byte chkDigit;
  char wifiSSID[25];
  char wifiPass[25];
  char aprsIP[25];
  uint16_t aprsPort;
  char aprsPassword[6];
  char dest[8];
  byte destSsid;
  char call[8];
  byte ssid;
  byte serverSsid;
  uint16_t aprsGatewayRefreshTime;
  char comment[16];
  char symbool;  // = auto.
  char path1[8];
  byte path1Ssid;
  char path2[8];
  byte path2Ssid;
  byte interval;
  byte multiplier;
  byte power;
  byte height;
  byte gain;
  byte directivity;
  uint16_t preAmble;
  uint16_t tail;
  uint16_t currentBrightness;
  float lat;
  float lon;
  bool useAPRS;
  bool preEmphase;  
  bool highPass;
  bool lowPass;
  bool isDebug;
  uint16_t calData0;
  uint16_t calData1;
  uint16_t calData2;
  uint16_t calData3;
  uint16_t calData4; 
  bool doRotate;
  bool rotateTouch;
  char GoogleMapKey[50];
} Settings;

long lastBeacon = millis();
long loopDelay = millis();
long startTime = millis();
long gpsTime = millis();
long saveTime = millis();
long aprsGatewayRefreshed = millis();
long webRefresh = millis();
bool wifiAvailable = false;
bool wifiAPMode = false;
bool aprsGatewayConnected = false;
char httpBuf[120] = "\0";
char buf[300] = "\0";
uint16_t lastCourse = 0;
bool doTouch = false;
long startedDebugScreen = millis();
int debugLine = 0;

uint16_t* tft_buffer;
uint16_t* crc_buffer;
bool      buffer_loaded = false;
uint16_t  spr_width = 0;
uint16_t  bg_color  = 0;

AX25Msg incomingPacket;
uint8_t *packetData;

const int ledFreq = 5000;
const int ledResol = 8;
const int ledChannelforTFT = 0;

//#include "config.h"
#include "rdk_config.h"  // Change to config.h

TFT_eSPI tft       = TFT_eSPI();  // Invoke custom library
TFT_eSprite needle = TFT_eSprite(&tft); // Sprite object for needle
TFT_eSprite spr    = TFT_eSprite(&tft); // Sprite for meter reading

WiFiMulti wifiMulti;
WiFiClient httpNet;
TinyGPSPlus gps;
RDKOTA rdkOTA(OTAHOST);
AsyncWebServer server(80);
AsyncEventSource events("/events");

HardwareSerial GPSSerial(1);

#include "dial.h"
#include "webpages.h"

void setup() {
  crc_buffer =  (uint16_t*) malloc(52 * 52 * 2);  
  pinMode(DISPLAYLEDPIN, OUTPUT);
  digitalWrite(DISPLAYLEDPIN, 0);
  
  if (BEEPPIN > -1) {
    pinMode(BEEPPIN, OUTPUT);
    digitalWrite(BEEPPIN, 0);
  }

  ledcSetup(ledChannelforTFT, ledFreq, ledResol);
  ledcAttachPin(DISPLAYLEDPIN, ledChannelforTFT);

  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, RXD1, TXD1);

  if (!EEPROM.begin(EEPROM_SIZE)) {
    DrawButton(80, 120, 160, 30, "EEPROM Failed", "", TFT_BLACK, TFT_WHITE, "");
    DebugPrintln("failed to initialise EEPROM");
    while (1)
      ;
  }

  if (!LoadConfig()) {
    DebugPrintln(F("Writing defaults"));
    SaveConfig();
  }

  LoadConfig();

  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  
  tft.begin();
  tft.init();
  tft.setRotation(settings.doRotate?1:3);
  uint16_t calData[5] = {settings.calData0, settings.calData1, settings.calData2, settings.calData3, settings.rotateTouch?7:1};
  tft.setTouch(calData);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  // add Wi-Fi networks from All_Settings.h
  for (int i = 0; i < sizeof(wifiNetworks) / sizeof(wifiNetworks[0]); i++) {
    wifiMulti.addAP(wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);
    DebugPrintf("Wifi:%s, Pass:%s.\r\n", wifiNetworks[i].SSID, wifiNetworks[i].PASSWORD);
  }

  DrawButton(80, 80, 160, 30, "Connecting to WiFi", "", TFT_BLACK, TFT_WHITE, "");

  wifiMulti.addAP(settings.wifiSSID, settings.wifiPass);
  DebugPrintf("Wifi:%s, Pass:%s.\r\n", settings.wifiSSID, settings.wifiPass);
  if (Connect2WiFi()) {
    wifiAvailable = true;
    DrawButton(80, 80, 160, 30, "Connected to WiFi", WiFi.SSID(), TFT_BLACK, TFT_WHITE, "");
    delay(1000);

    tft.fillScreen(TFT_BLACK);
    if (rdkOTA.checkForUpdate(OTAVERSION)){
      if (questionBox("Install update", TFT_WHITE, TFT_NAVY, 80, 80, 160, 40)){
        DrawButton(80, 80, 160, 40, "Installing update", "", TFT_BLACK, TFT_RED, "");
        rdkOTA.installUpdate();
      } 
    }

    udp.begin(localPort);
    syncTime();
  } else {
    wifiAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("APRSWiFi", NULL);
  }
  DebugPrintf("Main loop running in Core:%d.\r\n", xPortGetCoreID());
  ledcWrite(ledChannelforTFT, 256 - (settings.currentBrightness * 2.56));
  SetAPRSParameters();

  xTaskCreatePinnedToCore(
    ReadTask, "ReadTask"  // A name just for humans
    ,
    10000  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,
    NULL, 0  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,
    NULL, 0);  // Core 0

  esp_task_wdt_init(WDT_TIMEOUT, true);  //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                //add current thread to WDT watch

  PrintGPSInfo();
  delay(1000);
  tft.fillScreen(TFT_BLACK);

  // Draw the dial
  TJpgDec.drawJpg(40, 0, dial, sizeof(dial));
  tft.drawCircle(DIAL_CENTRE_X, DIAL_CENTRE_Y, NEEDLE_RADIUS-NEEDLE_LENGTH, TFT_DARKGREY);

  // Load the font and create the Sprite for reporting the value
  spr.loadFont(AA_FONT_LARGE);
  spr_width = spr.textWidth("777"); // 7 is widest numeral in this font
  spr.createSprite(spr_width, spr.fontHeight());
  bg_color = tft.readPixel(120, 120); // Get colour from dial centre
  spr.fillSprite(bg_color);
  spr.setTextColor(TFT_WHITE, bg_color, true);
  spr.setTextDatum(MC_DATUM);
  spr.setTextPadding(spr_width);
  spr.drawNumber(0, spr_width/2, spr.fontHeight()/2);
  spr.pushSprite(DIAL_CENTRE_X - spr_width / 2, DIAL_CENTRE_Y - spr.fontHeight() / 2);

  // Plot the label text
  tft.setTextColor(TFT_YELLOW, bg_color);
  //tft.setTextColor(TFT_WHITE, bg_color);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("KM/H", DIAL_CENTRE_X, DIAL_CENTRE_Y + 48, 2);

  // Define where the needle pivot point is on the TFT before
  // creating the needle so boundary calculation is correct
  tft.setPivot(DIAL_CENTRE_X, DIAL_CENTRE_Y);

  // Create the needle Sprite
  createNeedle();

  // Reset needle position to 0
  plotNeedle(0, 0);
}

void loop() {
  esp_task_wdt_reset();

  if (millis() - gpsTime > 200) {
    gpsTime = millis();
    if (settings.isDebug) PrintGPSInfo();
    tft.fillCircle(12, 12, 10, gps.location.age()<1000?TFT_GREEN:TFT_RED);
    tft.setTextColor(TFT_BLACK, gps.location.age()<1000?TFT_GREEN:TFT_RED, true);
    spr_width = tft.textWidth("77");
    tft.setTextPadding(spr_width);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(String(gps.satellites.value()), 12, 12);
    tft.fillCircle(308, 12, 10, WiFi.status() == WL_CONNECTED?TFT_GREEN:TFT_RED);
    tft.setTextColor(TFT_BLACK, WiFi.status() == WL_CONNECTED?TFT_GREEN:TFT_RED, true);
    spr_width = tft.textWidth("W");
    tft.setTextPadding(spr_width);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("W", 310, 12);

    spr_width = tft.textWidth("7777777"); // 7 is widest numeral in this font
    tft.setTextColor(TFT_GREEN, bg_color, true);
    tft.setTextPadding(spr_width);
    
    tft.drawString("   Lat:" + String(gps.location.lat()==0?settings.lat:gps.location.lat(),6) + "N  ", 160, 180);
    tft.drawString("   Lon:" + String(gps.location.lng()==0?settings.lon:gps.location.lng(),6) + "E  ", 160, 190);
    tft.drawString("   Height:" + String(gps.altitude.meters(),0) + "M   ", 160, 200);
    String sCourse = gps.course.isValid()?String(gps.course.deg(),0):"***";
    tft.drawString("   Course:" + sCourse + "   ", 160,210);   
    tft.setTextColor(TFT_WHITE, bg_color, true);
    char sz[50];
    tft.setTextDatum(ML_DATUM);
    tft.drawString(OTAVERSION, 2, 215, 1); 
    sprintf(sz, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    tft.drawString(sz, 2, 225, 1); 
    sprintf(sz, "%02d/%02d/%02d", gps.date.month(), gps.date.day(), gps.date.year());
    tft.drawString(sz, 2, 235, 1); 
    if (wifiAvailable){
      sprintf(sz, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    } else {
      sprintf(sz, "%d.%d.%d.%d", 192,168,4,1);
    }

    tft.setTextDatum(MR_DATUM);
    tft.drawString("  " + String(settings.call) + "-"  + String(settings.ssid),318,215,1);
    tft.drawString("  " + String(wifiAvailable?WiFi.SSID():"APRSWiFi"), 318, 225, 1);
    tft.drawString(sz, 318, 235, 1);
    plotNeedle(gps.speed.kmph(), 30);
  }  

  uint16_t touchX = 0, touchY = 0;
  bool pressed = tft.getTouch(&touchX, &touchY);
  bool beaconPressed = false;
  if (pressed){
    Serial.printf("Pressed = x:%d,y:%d\r\n",touchX,touchY);
    if (touchY<50 && touchX<50) ESP.restart();
    if (touchY<50 && touchX>270) beaconPressed = true;
  }

  if (settings.useAPRS && (gps.location.isValid() || settings.isDebug)) {
    bool doBeacon = beaconPressed;
    long beaconInterval = settings.interval * settings.multiplier * 1000;
    int gpsSpeed = gps.speed.isValid() ? gps.speed.kmph() : 0;
    if (gpsSpeed > 5) beaconInterval = settings.interval * 4 * 1000;
    if (gpsSpeed > 25) beaconInterval = settings.interval * 2 * 1000;
    if (gpsSpeed > 80) beaconInterval = settings.interval * 1000;

    if (millis() - lastBeacon > beaconInterval) doBeacon = true;
    if (millis() - lastBeacon > 20000 && settings.isDebug) doBeacon = true;

    if (gps.course.isValid()) {
      uint16_t sbCourse = (abs(gps.course.deg() - lastCourse));
      if (sbCourse > 180) sbCourse = 360 - sbCourse;
      if (sbCourse > 27) doBeacon = true;
    }
    if (millis() - lastBeacon < 5000) doBeacon = false;

    if (doBeacon) {
      lastBeacon = millis();
      lastCourse = gps.course.isValid() ? gps.course.deg() : -1;
      SendBeacon(false);
    }
  }

  if (millis() - aprsGatewayRefreshed > (settings.aprsGatewayRefreshTime * 1000)) {
    APRSGatewayUpdate();
    aprsGatewayRefreshed = millis();
  }  

  if ((millis() - webRefresh) > 1000) {
    RefreshWebPage();
    webRefresh = millis();
  }

}

void ReadTask(void *pvParameters) { // This is a task.
  if (wifiAvailable || wifiAPMode) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html, Processor);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/css", css_html);
    });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", settings_html, Processor);
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Rebooting");
      ESP.restart();
    });

    server.on("/calibrate", HTTP_GET, [] (AsyncWebServerRequest *request) {
      if (request->client()->remoteIP()[0] == 192 || request->client()->remoteIP()[0] == 10 || request->client()->remoteIP()[0] == 172){
        request->send_P(200, "text/html", index_html, Processor);
        doTouch = true;
      }
      else
        request->send_P(200, "text/html", warning_html, Processor);
    });

    server.on("/store", HTTP_GET, [](AsyncWebServerRequest *request) {
      SaveSettings(request);
      SaveConfig();
      SetAPRSParameters();
      request->send_P(200, "text/html", settings_html, Processor);
    });

    events.onConnect([](AsyncEventSourceClient *client) {
      DebugPrintln("Connect web");
      if (client->lastId()) {
        DebugPrintf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      client->send("hello!", NULL, millis(), 10000);
    });
    server.addHandler(&events);    

    server.begin();
    DebugPrintln("HTTP server started");
    DebugPrintf("HTTP server running in Core:%d.\r\n", xPortGetCoreID());
  }

  for (;;)  // A Task shall never return or exit.
  {
    readGPSData();
    vTaskDelay(50 / portTICK_PERIOD_MS);  // wait for 50 miliSec
  }
}

void SetAPRSParameters() {
  APRS_setCallsign(settings.call, settings.ssid);
  APRS_setDestination(settings.dest, settings.destSsid);
  APRS_setSymbol(settings.symbool);
  APRS_setPath1(settings.path1, settings.path1Ssid);
  APRS_setPath2(settings.path2, settings.path2Ssid);
  APRS_setPower(settings.power);
  APRS_setHeight(settings.height);
  APRS_setGain(settings.gain);
  APRS_setDirectivity(settings.directivity);
  APRS_setPreamble(settings.preAmble);
  APRS_setTail(settings.tail);
  APRS_setLat(Deg2Nmea(settings.lat, true));
  APRS_setLon(Deg2Nmea(settings.lon, false));
  APRS_printSettings();

  APRSGatewayUpdate();
  aprsGatewayRefreshed = millis();
}

bool Connect2WiFi() {
  startTime = millis();
  DebugPrint("Connect to Multi WiFi");
  while (wifiMulti.run() != WL_CONNECTED && millis() - startTime < 30000) {
    esp_task_wdt_reset();
    delay(1000);
    DebugPrint(".");
  }
  DebugPrintln();
  esp_task_wdt_reset();
  return (WiFi.status() == WL_CONNECTED);
}

void doBeep(int timeLen){
    if (BEEPPIN > -1) {
    digitalWrite(BEEPPIN, 1);
    delay(timeLen);
    digitalWrite(BEEPPIN, 0);
  }
}

void RefreshWebPage() {
  if (WiFi.status() == WL_CONNECTED || wifiAPMode) {
    DebugPrintln("Refresh webpage");
    sprintf(buf, "%s", String(gps.location.lat()==0?settings.lat:gps.location.lat(),4));
    events.send(buf, "LATINFO", millis());
    sprintf(buf, "%s", String(gps.location.lng()==0?settings.lon:gps.location.lng(),4));
    events.send(buf, "LONINFO", millis());
    sprintf(buf, "%s", String(gps.speed.isValid() ? gps.speed.kmph() : 0, 0));
    events.send(buf, "SPEEDINFO", millis());
    sprintf(buf, "%s", gps.location.age() > 5000 ? "Inv." : String(gps.location.age()));
    events.send(buf, "AGEINFO", millis());
    sprintf(buf, "%s,%s", String(gps.location.lat()==0?settings.lat:gps.location.lat(),6), String(gps.location.lng()==0?settings.lon:gps.location.lng(),6));
    events.send(buf, "DRAWMAP", millis());
    //sprintf(buf, "%s", String(gps.location.lat(), 4), String(gps.location.lng(), 4), String(gps.speed.isValid() ? gps.speed.kmph() : 0, 0), gps.location.age() > 5000 ? "Inv." : String(gps.location.age()));
  }
}

void PrintGPSInfo() {
  char sz[50];
  tft.fillRect(2, 2, 320, 200, TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextPadding(50);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  tft.drawString("GPS:", 2, 10, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  String GPSValid = gps.location.isValid() ? "true" : "false";
  tft.drawString("Valid       :" + GPSValid, 2, 25, 1);
  tft.drawString("Lat         :" + String(gps.location.lat(), 6), 2, 33, 1);
  tft.drawString("Lon         :" + String(gps.location.lng(), 6), 2, 41, 1);
  tft.drawString("Age         :" + String(gps.location.age()), 2, 49, 1);
  sprintf(sz, "Date & Time :%02d/%02d/%02d %02d:%02d:%02d", gps.date.month(), gps.date.day(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second());
  tft.drawString(sz, 2, 57, 1);
  tft.drawString("Height      :" + String(gps.altitude.meters()), 2, 65, 1);
  tft.drawString("Course      :" + String(gps.course.deg()), 2, 73, 1);
  tft.drawString("Speed       :" + String(gps.speed.isValid() ? gps.speed.kmph() : 0), 2, 81, 1);
  sprintf(sz, "Course valid:%s", gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "***");
  tft.drawString(sz, 2, 89, 1);

  if (wifiAvailable || wifiAPMode) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("WiFi:", 2, 105, 2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("SSID        :" + String(WiFi.SSID()), 2, 120, 1);
    sprintf(sz, "IP Address  :%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    DebugPrintln(sz);
    tft.drawString(sz, 2, 128, 1);
    tft.drawString("RSSI        :" + String(WiFi.RSSI()), 2, 136, 1);
  }
}

void SendBeacon(bool manual) {
  if (gps.location.age() < 5000 || manual || settings.isDebug) {
    if (settings.isDebug) ShowDebugScreen("Send beacon");
    if (APRSGatewayConnect()) SendBeaconViaWiFi();
    lastBeacon = millis();
  }
}

bool APRSGatewayConnect() {
  char c[20];
  sprintf(c, "%s", WiFi.status() == WL_CONNECTED ? "WiFi Connected" : "WiFi NOT Connected");
  DrawDebugInfo(c);

  if (wifiAvailable && (WiFi.status() != WL_CONNECTED)) {
    aprsGatewayConnected = false;
    Connect2WiFi();
    esp_task_wdt_reset();
  } else DebugPrintln("WiFi available and WiFiStatus connected");

  if (wifiAvailable && (WiFi.status() == WL_CONNECTED)) {
    if (!aprsGatewayConnected) {
      DrawDebugInfo("Connecting to APRS server...");
      if (httpNet.connect(settings.aprsIP, settings.aprsPort)) {
        esp_task_wdt_reset();
        if (ReadHTTPNet()) DrawDebugInfo(httpBuf);
        esp_task_wdt_reset();  
        sprintf(buf, "user %s-%d pass %s vers ", settings.call, settings.serverSsid, settings.aprsPassword, VERSION);
        DrawDebugInfo(buf);
        httpNet.println(buf);
        if (ReadHTTPNet()) {
          esp_task_wdt_reset();  
          if (strstr(httpBuf, " verified")) {
            DrawDebugInfo(httpBuf);
            DrawDebugInfo("Connected to APRS.FI");
            aprsGatewayConnected = true;
            esp_task_wdt_reset();  
          } else DrawDebugInfo("Not connected to APRS.FI");
        } else DrawDebugInfo("No response from ReadHTTPNet");
      } else DrawDebugInfo("Failed to connect to APRS.FI, server unavailable");
    } else DrawDebugInfo("Already connected to APRS.FI");
  } else DrawDebugInfo("Failed to connect to APRS.FI, WiFi not available");
  return aprsGatewayConnected;
}

bool ReadHTTPNet() {
  httpBuf[0] = '\0';
  bool retVal = false;
  long timeOut = millis();
  while (!httpNet.available() && millis() - timeOut < 2500) 
  {
    esp_task_wdt_reset();
  }

  if (httpNet.available()) {
    int i = 0;
    while (httpNet.available() && i < 118) {
      esp_task_wdt_reset();  
      char c = httpNet.read();
      httpBuf[i++] = c;
    }
    retVal = true;
    httpBuf[i++] = '\0';
    // DebugPrint("Read from HTTP:");
    // DebugPrint(httpBuf);
  } else {
    DebugPrint("No data returned from HTTP");
  }
  DebugPrintln();
  return retVal;
}

void FillAPRSInfo() {
  buf[0] = '\0';
  sprintf(buf, "APRS:%s-%d, %s-%d", settings.call, settings.ssid, settings.dest, settings.destSsid);
}

void ShowDebugScreen(char header[]) {
  if (settings.isDebug){
    startedDebugScreen = millis();
    debugLine = 0;
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(50);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(header, 2, 10, 2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }
}

void SendBeaconViaWiFi() {
  if (APRSGatewayConnect()) {
    if (!settings.isDebug) {
      //tft.fillCircle(12, 12, 10, TFT_BLACK);
      tft.readRect(135, 95, 50, 50, crc_buffer);
      tft.fillCircle(160, 120, 24, TFT_YELLOW);
      tft.setTextDatum(MC_DATUM);
      tft.setTextPadding(tft.textWidth("AP"));
      tft.setTextColor(TFT_BLACK, TFT_YELLOW);
      tft.drawString("AP", 160, 122, 4);
    }
    String sLat;
    String sLon;
    if (gps.location.age() > 5000) {
      sLat = Deg2Nmea(settings.lat, true);
      sLon = Deg2Nmea(settings.lon, false);
    } else {
      sLat = Deg2Nmea(gps.location.lat(), true);
      sLon = Deg2Nmea(gps.location.lng(), false);
    }
    sprintf(buf, "%s-%d>%s:=%s/%s%sPHG5000%s", settings.call, settings.ssid, settings.dest, sLat, sLon, String(settings.symbool), settings.comment);
    DrawDebugInfo(buf);
    httpNet.println(buf);
    if (ReadHTTPNet()) DrawDebugInfo(buf);
    if (!settings.isDebug) {
      //tft.fillCircle(24, 24, 24, TFT_BLACK);
      tft.pushRect(135, 95, 50, 50, crc_buffer);
    }
  }
}

char *Deg2Nmea(float fdeg, boolean is_lat) {
  long deg = fdeg * 1000000;
  bool is_negative = 0;
  if (deg < 0) is_negative = 1;

  // Use the absolute number for calculation and update the buffer at the end
  deg = labs(deg);

  unsigned long b = (deg % 1000000UL) * 60UL;
  unsigned long a = (deg / 1000000UL) * 100UL + b / 1000000UL;
  b = (b % 1000000UL) / 10000UL;

  buf[0] = '0';
  // in case latitude is a 3 digit number (degrees in long format)
  if (a > 9999) {
    snprintf(buf, 6, "%04u", a);
  } else snprintf(buf + 1, 5, "%04u", a);

  buf[5] = '.';
  snprintf(buf + 6, 3, "%02u", b);
  buf[9] = '\0';
  if (is_lat) {
    if (is_negative) {
      buf[8] = 'S';
    } else buf[8] = 'N';
    return buf + 1;
    // buf +1 because we want to omit the leading zero
  } else {
    if (is_negative) {
      buf[8] = 'W';
    } else buf[8] = 'E';
    return buf;
  }
}

void APRSGatewayUpdate() {
  ShowDebugScreen("Update APRS Gateway");
  DrawDebugInfo("aprsGatewayUpdate:");
  sprintf(buf, "Date & Time :%02d/%02d/%02d %02d:%02d:%02d", gps.date.month(), gps.date.day(), gps.date.year(), gps.time.hour(), gps.time.minute(), gps.time.second());
  DrawDebugInfo(buf);
  if (!settings.isDebug) {
    //tft.fillCircle(12, 12, 10, TFT_BLACK);
    tft.readRect(135, 95, 50, 50, crc_buffer);
    tft.fillCircle(160, 120, 24, TFT_PINK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(tft.textWidth("GW"));
    tft.setTextColor(TFT_BLACK, TFT_PINK);
    tft.drawString("GW", 160, 122, 4);
  }
  if (APRSGatewayConnect()) {
    DrawDebugInfo("Update IGate info on APRS");
    sprintf(buf, "%s-%d", settings.call, settings.serverSsid);
    DrawDebugInfo(buf);
    httpNet.print(buf);

    sprintf(buf, ">APRS,TCPIP*:@%02d%02d%02dz", gps.date.day(), gps.time.hour(), gps.time.minute());
    DrawDebugInfo(buf);
    httpNet.print(buf);
    String sLat = Deg2Nmea(settings.lat, true);
    String sLon = Deg2Nmea(settings.lon, false);
    sprintf(buf, "%s/%s", sLat, sLon);
    DrawDebugInfo(buf);
    httpNet.print(buf);

    sprintf(buf, "I/A=000012 %s", INFO);
    DrawDebugInfo(buf);
    httpNet.println(buf);
    if (!ReadHTTPNet()) aprsGatewayConnected = false;
  } else DrawDebugInfo("APRS Gateway not connected");
  if (!settings.isDebug) {
    //tft.fillCircle(24, 24, 24, TFT_BLACK);
    tft.pushRect(135, 95, 50, 50, crc_buffer);
  }
}

void DrawBox(int xPos, int yPos, int width, int height) {
  tft.drawRoundRect(xPos + 2, yPos + 2, width, height, 4, TFT_SHADOW);
  tft.drawRoundRect(xPos, yPos, width, height, 3, TFT_WHITE);
  tft.fillRoundRect(xPos + 1, yPos + 1, width - 2, height - 2, 3, TFT_BLACK);
}

void DrawButton(int xPos, int yPos, int width, int height, String caption, String waarde, uint16_t bottomColor, uint16_t topColor, String Name) {
  tft.setTextDatum(MC_DATUM);
  DrawBox(xPos, yPos, width, height);

  uint16_t gradientStartColor = TFT_BLACK;

  tft.fillRectVGradient(xPos + 2, yPos + 2, width - 4, (height / 2) + 1, gradientStartColor, topColor);
  tft.setTextPadding(tft.textWidth(caption));
  tft.setTextColor(TFT_WHITE);
  if (gradientStartColor == topColor) tft.setTextColor(TFT_BLACK);
  tft.drawString(caption, xPos + (width / 2), yPos + (height / 2) - 5, 2);

  if (Name == "Navigate") {
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(tft.textWidth("     <<     <"));
    tft.setTextColor(TFT_WHITE);
    tft.drawString("     <<     <", 5, yPos + (height / 2) - 5, 2);
    tft.setTextDatum(MR_DATUM);
    tft.setTextPadding(tft.textWidth(">     >>     "));
    tft.setTextColor(TFT_WHITE);
    tft.drawString(">     >>     ", 309, yPos + (height / 2) - 5, 2);
  }

  // tft.fillRectVGradient(xPos + 2,yPos + 2 + (height/2), width-4, (height/2)-4, TFT_BLACK, bottomColor);
  tft.fillRoundRect(xPos + 2, yPos + 2 + (height / 2), width - 4, (height / 2) - 4, 3, bottomColor);
  if (waarde != "") {
    tft.setTextPadding(tft.textWidth(waarde));
    tft.setTextColor(TFT_YELLOW);
    if (bottomColor != TFT_BLACK) tft.setTextColor(TFT_BLACK);
    if (bottomColor == TFT_RED) tft.setTextColor(TFT_WHITE);
    tft.drawString(waarde, xPos + (width / 2), yPos + (height / 2) + 9, 1);
  }
}

bool SaveConfig() {
  bool commitEeprom = false;
  for (unsigned int t = 0; t < sizeof(settings); t++) {
    if (*((char *)&settings + t) != EEPROM.read(offsetEEPROM + t)) {
      EEPROM.write(offsetEEPROM + t, *((char *)&settings + t));
      commitEeprom = true;
    }
  }
  if (commitEeprom) EEPROM.commit();
  return true;
}

bool LoadConfig() {
  bool retVal = true;
  if (EEPROM.read(offsetEEPROM + 0) == settings.chkDigit) {
    for (unsigned int t = 0; t < sizeof(settings); t++)
      *((char *)&settings + t) = EEPROM.read(offsetEEPROM + t);
  } else retVal = false;
  DebugPrintln("Configuration:" + retVal ? "Loaded" : "Not loaded");
  DebugPrintf("CalData[0,1,2,3,4]=%d,%d,%d,%d,%d", settings.calData0,settings.calData1,settings.calData2,settings.calData3,settings.calData4);
  return retVal;
}

void PrintTXTLine() {
  tft.setTextPadding(tft.textWidth(buf));
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString(buf, 2, 34, 1);
}

void DrawDebugInfo(char debugInfo[]) {
  if (settings.isDebug){
    tft.drawString(debugInfo, 2, 25 + (debugLine * 8), 1);
    DebugPrintln(debugInfo);
    debugLine++;
  }
}

bool questionBox(const char *msg, uint16_t fgcolor, uint16_t bgcolor, int x, int y, int w, int h) {
  uint16_t current_textcolor = tft.textcolor;
  uint16_t current_textbgcolor = tft.textbgcolor;

  //tft.loadFont(AA_FONT_SMALL);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(fgcolor, bgcolor);
  tft.fillRoundRect(x, y, w, h, 5, fgcolor);
  tft.fillRoundRect(x + 2, y + 2, w - 4, h - 4, 5, bgcolor);
  tft.setTextPadding(tft.textWidth(msg));
  tft.drawString(msg, x + 4 + w/2, y + (h / 4));

  tft.fillRoundRect(x + 4, y + (h/2) - 2, (w - 12)/2, (h - 4)/2, 5, TFT_GREEN);
  tft.setTextColor(fgcolor, TFT_GREEN);
  tft.setTextPadding(tft.textWidth("Yes"));
  tft.drawString("Yes", x + 4 + ((w - 12)/4),y + (h/2) - 2 + (h/4));
  tft.fillRoundRect(x + (w/2) + 2, y + (h/2) - 2, (w - 12)/2, (h - 4)/2, 5, TFT_RED);
  tft.setTextColor(fgcolor, TFT_RED);
  tft.setTextPadding(tft.textWidth("No"));
  tft.drawString("No", x + (w/2) + 2 + ((w - 12)/4),y + (h/2) - 2 + (h/4));
  Serial.printf("Yes = x:%d,y:%d,w:%d,h:%d\r\n",x + 4, y + (h/2) - 2, (w - 12)/2, (h - 4)/2);
  Serial.printf("No  = x:%d,y:%d,w:%d,h:%d\r\n",x + (w/2) + 2, y + (h/2) - 2, (w - 12)/2, (h - 4)/2);
  tft.setTextColor(current_textcolor, current_textbgcolor);
  //tft.unloadFont();

  uint16_t touchX = 0, touchY = 0;

  long startWhile = millis();
  while (millis()-startWhile<30000) {
    bool pressed = tft.getTouch(&touchX, &touchY);
    if (pressed){
      Serial.printf("Pressed = x:%d,y:%d\r\n",touchX,touchY);
      if (touchY>=y + (h/2) - 2 && touchY<=y + (h/2) - 2 + ((h - 4)/2)){
        if (touchX>=x + 4 && touchX<=x + 4 + ((w - 12)/2)) return true;
        if (touchX>=x + (w/2) + 2 && touchX<=x + (w/2) + 2 + ((w - 12)/2)) return false;
      }
    }
  }
  return false;
}

void readGPSData() {
  if (GPSSerial.available()) {
    while (GPSSerial.available()) {
      //Serial.write(GPSSerial.read());
      gps.encode(GPSSerial.read());
    }
  }
}

String Processor(const String &var) {
  if (var == "APRSINFO") {
    FillAPRSInfo();
    return buf;
  }

  if (var == "LATINFO") {
    sprintf(buf, "%s", String(gps.location.lat(), 4));
    return buf;
  }

  if (var == "LONINFO") {
    sprintf(buf, "%s", String(gps.location.lng(), 4));
    return buf;
  }

  if (var == "SPEEDINFO") {
    sprintf(buf, "%s", String(gps.speed.isValid() ? gps.speed.kmph() : 0, 0));
    return buf;
  }

  if (var == "AGEINFO") {
    sprintf(buf, "%s", gps.location.age() > 5000 ? "Inv." : String(gps.location.age()));
    return buf;
  }  

  if (var == "wifiSSID") return settings.wifiSSID;
  if (var == "wifiPass") return settings.wifiPass;

  if (var == "aprsIP") return String(settings.aprsIP);
  if (var == "aprsPort") return String(settings.aprsPort);
  if (var == "aprsPassword") return String(settings.aprsPassword);
  if (var == "serverSsid") return String(settings.serverSsid);
  if (var == "aprsGatewayRefreshTime") return String(settings.aprsGatewayRefreshTime);
  if (var == "call") return String(settings.call);
  if (var == "ssid") return String(settings.ssid);
  if (var == "symbool") {
    char x = settings.symbool;
    return String(x);
  }
  if (var == "dest") return String(settings.dest);
  if (var == "destSsid") return String(settings.destSsid);
  if (var == "path1") return String(settings.path1);
  if (var == "path1Ssid") return String(settings.path1Ssid);
  if (var == "path2") return String(settings.path2);
  if (var == "path2Ssid") return String(settings.path2Ssid);
  if (var == "comment") return settings.comment;
  if (var == "interval") return String(settings.interval);
  if (var == "multiplier") return String(settings.multiplier);
  if (var == "lat") return String(settings.lat, 6);
  if (var == "lon") return String(settings.lon, 6);
  if (var == "GoogleMapKey") return settings.GoogleMapKey;
  if (var == "isDebug") return settings.isDebug ? "checked" : "";
  if (var == "doRotate") return settings.doRotate ? "checked" : "";
  if (var == "rotateTouch") return settings.rotateTouch ? "checked" : "";
  return var;
}

void SaveSettings(AsyncWebServerRequest *request) {
  if (request->hasParam("wifiSSID")) request->getParam("wifiSSID")->value().toCharArray(settings.wifiSSID, 25);
  if (request->hasParam("wifiPass")) request->getParam("wifiPass")->value().toCharArray(settings.wifiPass, 25);
  if (request->hasParam("aprsIP")) request->getParam("aprsIP")->value().toCharArray(settings.aprsIP, 25);
  if (request->hasParam("aprsPort")) settings.aprsPort = request->getParam("aprsPort")->value().toInt();
  if (request->hasParam("aprsPassword")) request->getParam("aprsPassword")->value().toCharArray(settings.aprsPassword, 6);
  if (request->hasParam("serverSsid")) settings.serverSsid = request->getParam("serverSsid")->value().toInt();
  if (request->hasParam("aprsGatewayRefreshTime")) settings.aprsGatewayRefreshTime = request->getParam("aprsGatewayRefreshTime")->value().toInt();
  if (request->hasParam("call")) request->getParam("call")->value().toCharArray(settings.call, 8);
  if (request->hasParam("ssid")) settings.ssid = request->getParam("ssid")->value().toInt();
  //if (request->hasParam("symbool")) DrawDebugInfo(request->getParam("symbol")->value());
  if (request->hasParam("dest")) request->getParam("dest")->value().toCharArray(settings.dest, 8);
  if (request->hasParam("destSsid")) settings.destSsid = request->getParam("destSsid")->value().toInt();
  if (request->hasParam("path1")) request->getParam("path1")->value().toCharArray(settings.path1, 8);
  if (request->hasParam("path1Ssid")) settings.path1Ssid = request->getParam("path1Ssid")->value().toInt();
  if (request->hasParam("path2")) request->getParam("path2")->value().toCharArray(settings.path2, 8);
  if (request->hasParam("path2Ssid")) settings.path2Ssid = request->getParam("path2Ssid")->value().toInt();
  if (request->hasParam("comment")) request->getParam("comment")->value().toCharArray(settings.comment, 16);
  if (request->hasParam("interval")) settings.interval = request->getParam("interval")->value().toInt();
  if (request->hasParam("multiplier")) settings.multiplier = request->getParam("multiplier")->value().toInt();
  if (request->hasParam("lat")) settings.lat = request->getParam("lat")->value().toFloat();
  if (request->hasParam("lat")) settings.lon = request->getParam("lon")->value().toFloat();
  if (request->hasParam("GoogleMapKey")) request->getParam("GoogleMapKey")->value().toCharArray(settings.GoogleMapKey, 50);  
  settings.isDebug = request->hasParam("isDebug");
  settings.doRotate = request->hasParam("doRotate");
  settings.rotateTouch = request->hasParam("rotateTouch");
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap){
  // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // Return 1 to decode next block
  return 1;
}

// =======================================================================================
// Create the needle Sprite
// =======================================================================================
void createNeedle(void){
  needle.setColorDepth(16);
  needle.createSprite(NEEDLE_WIDTH, NEEDLE_LENGTH);  // create the needle Sprite

  needle.fillSprite(TFT_BLACK); // Fill with black

  // Define needle pivot point relative to top left corner of Sprite
  uint16_t piv_x = NEEDLE_WIDTH / 2; // pivot x in Sprite (middle)
  uint16_t piv_y = NEEDLE_RADIUS;    // pivot y in Sprite
  needle.setPivot(piv_x, piv_y);     // Set pivot point in this Sprite

  // Draw the red needle in the Sprite
  needle.fillRect(0, 0, NEEDLE_WIDTH, NEEDLE_LENGTH, TFT_MAROON);
  needle.fillRect(1, 1, NEEDLE_WIDTH-2, NEEDLE_LENGTH-2, TFT_RED);

  // Bounding box parameters to be populated
  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;

  // Work out the worst case area that must be grabbed from the TFT,
  // this is at a 45 degree rotation
  needle.getRotatedBounds(45, &min_x, &min_y, &max_x, &max_y);

  // Calculate the size and allocate the buffer for the grabbed TFT area
  tft_buffer =  (uint16_t*) malloc( ((max_x - min_x) + 2) * ((max_y - min_y) + 2) * 2 );
}

// =======================================================================================
// Move the needle to a new position
// =======================================================================================
void plotNeedle(int16_t angle, uint16_t ms_delay){
  static int16_t old_angle = -120; // Starts at -120 degrees

  // Bounding box parameters
  static int16_t min_x;
  static int16_t min_y;
  static int16_t max_x;
  static int16_t max_y;

  if (angle < 0) angle = 0; // Limit angle to emulate needle end stops
  if (angle > 240) angle = 240;

  angle -= 120; // Starts at -120 degrees

  // Move the needle until new angle reached
  while (angle != old_angle || !buffer_loaded) {

    if (old_angle < angle) old_angle++;
    else old_angle--;

    // Only plot needle at even values to improve plotting performance
    if ( (old_angle & 1) == 0)
    {
      if (buffer_loaded) {
        // Paste back the original needle free image area
        tft.pushRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
      }

      if ( needle.getRotatedBounds(old_angle, &min_x, &min_y, &max_x, &max_y) )
      {
        // Grab a copy of the area before needle is drawn
        tft.readRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
        buffer_loaded = true;
      }

      // Draw the needle in the new position, black in needle image is transparent
      needle.pushRotated(old_angle, TFT_BLACK);

      // Wait before next update
      delay(ms_delay);
    }

    // Update the number at the centre of the dial
    spr.setTextColor(TFT_WHITE, bg_color, true);
    spr.drawNumber(old_angle+120, spr_width/2, spr.fontHeight()/2);
    spr.pushSprite(160 - spr_width / 2, 120 - spr.fontHeight() / 2);

    // Slow needle down slightly as it approaches the new position
    if (abs(old_angle - angle) < 10) ms_delay += ms_delay / 5;
  }
}