/*
 * VERSION 0.53
 * August 7, 2022
 * Adds time sync upon initial boot
 * Adds new button press options: Next text effect (green), clear notifications (red)
 * Fix missing mqttConnected = true in main loop MQTT reconnect
 * By ResinChem Tech - licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License
*/
#include <Wire.h>
#include <RtcDS3231.h>                        // Include RTC library by Makuna: https://github.com/Makuna/Rtc
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <FastLED.h>
#include <FS.h>                               // Please read the instructions on http://arduino.esp8266.com/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system
#include <PubSubClient.h>
#include <ESP8266mDNS.h>                      
#include <WiFiUdp.h>                          
#include <ArduinoOTA.h>                       
#include "Settings.h"
#define countof(a) (sizeof(a) / sizeof(a[0]))
// ==============================================================================================
//  *** REVIEW THE Settings.h and Credentials.h FILES AND SET TO MATCH YOUR ENVIRONMENT/BUILD ***
//      You may also set various default boot options in the Settings.h file
// ==============================================================================================
// Do not change any values below unless you are sure you know what you are doing!

byte oldMode = 0;
byte oldMQTTMode = 0;
int oldTemp = 0;
bool ota_flag = true;               //v1.00
uint16_t ota_time_elapsed = 0;      //v1.00
uint16_t ota_time_window = 2500;    //v1.00 minimum time on boot for IP address to show in IDE ports
unsigned long tempUpdatePeriod = 60;  //Seconds to elapse before updating temp display in clock mode. Default 1 minute if update period not specified in Settings
unsigned long tempUpdateCount = 0;
int externalTemperature = 0;      // Will only be used if external temp selected in Settings.h (MQTT is required to provide external temp)
String textTop = "";
String textBottom = "";
String textFull = "";
long lastReconnectAttempt = 0;
unsigned long prevTime = 0;
unsigned long prevTempTime = 0;                   // To limit MQTT Temp updates so as not to flood broker
byte r_val = 255;
byte g_val = 0;
byte b_val = 0;
bool dotsOn = true;
unsigned long countdownMilliSeconds;
unsigned long endCountDownMillis;
bool mqttConnected = false;                       // Will be set to true upon valid connection
bool timerRunning = false; 
unsigned long remCountdownMillis = 0;             // Stores value on pause/resume
unsigned long initCountdownMillis = 0;            // Stores initial last countdown value for reset    
//Variables for processing text effects
unsigned int textEffectPeriod = 1000;             // Update period for text effects in millis (min 250 max 2500)
bool effectTextHide = false;                      // For text Flash and FlashAlternate effects
int effectBrightness = 0;                         // For text Fade In and Fade Out effects
byte oldTextEffect = 0;                           // For determining effect switch and to setup new effect
int appearCount = 99;                            // Counter for Appear and Appear Flash effects

#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2)
  const char* APssid = "MATRIX_AP";        
  const char* APpassword = "1234567890";  
#endif
  
#if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2)
  #include "Credentials.h"                    // Edit this file in the same directory as the .ino file and add your own credentials
  const char *ssid = SID;
  const char *password = PW;
  #if (WIFIMODE == 1 || WIFIMODE == 2) && (MQTTMODE == 1)    // MQTT only available when on local wifi
    const char *mqttUser = MQTTUSERNAME;
    const char *mqttPW = MQTTPWD;
  #endif
#endif
WiFiClient espClient;
PubSubClient client(espClient);    

RtcDS3231<TwoWire> Rtc(Wire);
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdateServer;
CRGB LEDs[NUM_LEDS];
//Large Numbers 
unsigned long numbers[3][18] = {
   // Original 7-Segment Font
   {
    0b00000000001110111011101111110111,  // [0,0] 0
    0b00000000001110000000000000000111,  // [1] 1
    0b00000011101110111000001111110000,  // [2] 2
    0b00000011101110111000000001110111,  // [3] 3
    0b00000011101110000011100000000111,  // [4] 4
    0b00000011100000111011100001110111,  // [5] 5
    0b00000011100000111011101111110111,  // [6] 6
    0b00000000001110111000000000000111,  // [7] 7
    0b00000011101110111011101111110111,  // [8] 8
    0b00000011101110111011100001110111,  // [9] 9
    0b00000000000000000000000000000000,  // [10] off
    0b00000011101110111011100000000000,  // [11] degrees symbol
    0b00000000000000111011101111110000,  // [12] C(elsius)
    0b00000011100000111011101110000000,  // [13] F(ahrenheit)
    0b00000000001110000011101111110111,  // [14] U
    0b00000011101110111011101110000000,  // [15] P
    0b00000000000000000011101111110000,  // [16] L
    0b00000011101110000000001111110111,  // {17] d
   },
   // Modern Font
   {
    0b00000000011110111011111111110111,  // [1,0] 0
    0b11111101000000000000000000000000,  // [1] 1
    0b00000011101110111010001111111000,  // [2] 2
    0b00000011101110111010000011110111,  // [3] 3
    0b00000011111111000111100000001111,  // [4] 4
    0b00000011100000111111100011110111,  // [5] 5
    0b00000011100000111011111111110111,  // [6] 6
    0b11100000101111111000000000100000,  // [7] 7
    0b00000011101110111011101111110111,  // [8] 8
    0b00000011101110111011100011110111,  // [9] 9
    0b00000000000000000000000000000000,  // [10] off
    0b00000011101110111011100000000000,  // [11] degrees symbol
    0b00000000000000111011101111110000,  // [12] C(elsius)
    0b00000011100000111011101110000000,  // [13] F(ahrenheit)
    0b00000000001110000011101111110111,  // [14] U
    0b00000011101110111011101110000000,  // [15] P
    0b00000000000000000011101111110000,  // [16] L
    0b00000011101110000000001111110111,  // {17] d
  },
  // Hybrid Font
  {
    0b00000000011110111011111111110111,  // [0] 0
    0b11111101000000000000000000000000,  // [1] 1
    0b00000011101110111000001111110000,  // [2] 2
    0b00000011101110111000000001110111,  // [3] 3
    0b00000011111110000011100000000111,  // [4] 4
    0b00000011100000111011100001110111,  // [5] 5
    0b00000011100000111011111111110111,  // [6] 6
    0b00000000011110111000000000000111,  // [7] 7
    0b00000011101110111011101111110111,  // [8] 8
    0b00000011101110111011100001110111,  // [9] 9
    0b00000000000000000000000000000000,  // [10] off
    0b00000011101110111011100000000000,  // [11] degrees symbol
    0b00000000000000111011101111110000,  // [12] C(elsius)
    0b00000011100000111011101110000000,  // [13] F(ahrenheit)
    0b00000000001110000011101111110111,  // [14] U
    0b00000011101110111011101110000000,  // [15] P
    0b00000000000000000011101111110000,  // [16] L
    0b00000011101110000000001111110111,  // {17] d
  }
  };
//#endif

//Define small numbers (3 x 5) - 12 pixels total - For temp display (right-to-left)
long smallNums[] = {
  0b0111111111111, // [0] 0
  0b0111000000011, // [1] 1
  0b1111110111110, // [2] 2
  0b1111110101111, // [3] 3
  0b1111011100011, // [4] 4
  0b1101111101111, // [5] 5
  0b1101111111111, // [6] 6
  0b0111110000011, // [7] 7
  0b1111111111111, // [8] 8
  0b1111111100011, // [9] 9
  0b1110101111011, // [10] A
  0b1010111111000, // [11] P
  0b0001111111110, // [12] C 
  0b1001111111000, // [13] F
  0b0111011110101, // [14] V
  0b1111011111011, // [15] H
  0b0000000000000, // [16] Off
};

//Define small letters (3 x 5) - 14 pixels total - for text display mode (right-to-left)
//Valid ascii values: 32-122 (both upper and lower case letters render the same)
//Must use & or $ for leading blanks - these will always render as blank
long letters[] = {
  0b000000000000000, // [0]  ' ' ascii 32 (space)
  0b000000011101000, // [1]  '!' ascii 33 (exclamation)
  0b000011011000000, // [2]  '"' ascii 34 (quote)
  0b111010101010101, // [3]  '#' ascii 35 (hashtag)
  0b000000000000000, // [4]  '$' ascii 36 (dollar - render as blank)
  0b001010010010010, // [5]  '%' ascii 37 (percent)
  0b000000000000000, // [6]  '&' ascii 38 (ampersand - render as blank)
  0b000000101000000, // [7]  ''' ascii 39 (apostrophe)
  0b111001000000010, // [8]  '(' ascii 40 (left paren)
  0b111000010001000, // [9]  ')' ascii 41 (right paren)
  0b010101010100000, // [10] '*' ascii 42 (asterisk)
  0b111100000100000, // [11] '+' ascii 43 (plus)
  0b100000000001000, // [12] ',' ascii 44 (comma)
  0b001100000100000, // [13] '-' ascii 45 (minus/dash)
  0b000000000001000, // [14] '.' ascii 46 (period)
  0b001010000010000, // [15] '/' ascii 47 (slash)
  0b000110101110101, // [16] '0' ascii 48       //numbers
  0b111000100000100, // [17] '1' ascii 49
  0b001111110111110, // [18] '2' ascii 50
  0b001111110001111, // [19] '3' ascii 51
  0b001111011100011, // [20] '4' ascii 52
  0b001101111101111, // [21] '5' ascii 53
  0b001101111111111, // [22] '6' ascii 54
  0b000111110000011, // [23] '7' ascii 55
  0b001111111111111, // [24] '8' ascii 56
  0b001111111100011, // [25] '9' ascii 57
  0b110000000000000, // [26] ':' ascii 58 (colon)
  0b110000000001000, // [27] ';' ascii 59 (semicolon)
  0b110001000100010, // [28] '<' ascii 60 (less than)
  0b110010001010001, // [29] '=' ascii 61 (equal)
  0b110100010001000, // [30] '>' ascii 62 (greater than)
  0b101111110000100, // [31] '?' ascii 63 (question)
  0b001111111110110, // [32] '@' ascii 64 (at)      
  0b001110101111011, // [33] 'A' ascii 65/96    //alphas
  0b001100011111111, // [34] 'B' ascii 66/98
  0b000001111111110, // [35] 'C' ascii 67/99
  0b000110111111101, // [36] 'D' ascii 68/100
  0b001001111111110, // [37] 'E' ascii 69/101
  0b001001111111000, // [38] 'F' ascii 70/102
  0b001101101111101, // [39] 'G' ascii 71/103
  0b001111011111011, // [40] 'H' ascii 72/104
  0b111000100000100, // [41] 'I' ascii 73/105
  0b000111000010101, // [42] 'J' ascii 74/106
  0b001011011111011, // [43] 'K' ascii 75/107
  0b000000011111110, // [44] 'L' ascii 76/108
  0b010111011111011, // [45] 'M' ascii 77/109
  0b010100011111011, // [46] 'N' ascii 78/110
  0b000111111111111, // [47] 'O' ascii 79/111
  0b001111111111000, // [48] 'P' ascii 80/112
  0b100110101110111, // [49] 'Q' ascii 81/113
  0b001010111111011, // [50] 'R' ascii 82/114
  0b001101111101111, // [51] 'S' ascii 83/115
  0b111001110000100, // [52] 'T' ascii 84/116
  0b000111011111111, // [53] 'U' ascii 85/117
  0b000111011110101, // [54] 'V' ascii 86/118
  0b100111011111011, // [55] 'W' ascii 87/119
  0b001011011011011, // [56] 'X' ascii 88/120
  0b101111011100100, // [57] 'Y' ascii 89/121
  0b001011110011110, // [58] 'Z' ascii 90/122
  0b111001100000110, // [59] '[' ascii 91 (left bracket)
  0b001000001000001, // [60] '\' ascii 92 (back slash)
  0b111000110001100, // [61] ']' ascii 93 (right bracket)
  0b000010101000000, // [62] '^' ascii 94 (carat)
  0b000000000001110, // [63] '_' ascii 95 (underscore)
  0b000010100000000, // [64] '`' ascii 96 (tick)
};

//Pixel Locations for full digits.  First index is segment position on matrix. (0-3 Clock, 4-7 countdown, 8-11 score)
// Second index is the pixel position within the segment
unsigned int fullnumPixelPos[12][33] = {
  {226,225,176,175,174,173,172,180,221,230,271,280,321,330,371,372,373,374,375,326,325,276,275,274,273,272,328,323,278,228,223,178},  //[0]  Segment Pos 0
  {232,219,182,169,168,167,166,186,215,236,265,286,315,336,365,366,367,368,369,332,319,282,269,268,267,266,334,317,284,234,217,184},  //[1]  Segment Pos 1
  {240,211,190,161,160,159,158,194,207,244,257,294,307,344,357,358,359,360,361,340,311,290,261,260,259,258,342,309,292,242,209,192},  //[2]  Segment Pos 2
  {246,205,196,155,154,153,152,200,201,250,251,300,301,350,351,352,353,354,355,346,305,296,255,254,253,252,348,303,298,248,203,198},  //[3]  Segment Pos 3
  {176,175,126,125,124,123,122,130,171,180,221,230,271,280,321,322,323,324,325,276,275,226,225,224,223,222,278,273,228,178,173,128},  //[4]  Segment Pos 4
  {182,169,132,119,118,117,116,136,165,186,215,236,265,286,315,316,317,318,319,282,269,232,219,218,217,216,284,267,234,184,167,134},  //[5]  Segment Pos 5
  {190,161,140,111,110,109,108,144,157,194,207,244,257,294,307,308,309,310,311,290,261,240,211,210,209,208,292,259,242,192,159,142},  //[6]  Segment Pos 6
  {196,155,146,105,104,103,102,150,151,200,201,250,251,300,301,302,303,304,305,296,255,246,205,204,203,202,298,253,248,198,153,148},  //[7]  Segment Pos 7
  {125,76,75,26,27,28,29,71,80,121,130,171,180,221,230,229,228,227,226,225,176,175,126,127,128,129,223,178,173,123,78,73},            //[8]  Segment Pos 8
  {119,82,69,32,33,34,35,65,86,115,136,165,186,215,236,235,234,233,232,219,182,169,132,133,134,135,217,184,167,117,84,67},            //[9]  Segment Pos 9
  {111,90,61,40,41,42,43,57,94,107,144,157,194,207,244,243,242,241,240,211,190,161,140,141,142,143,209,192,159,109,92,59},            //[10] Segment Pos 10
  {105,96,55,46,47,48,49,51,100,101,150,151,200,201,250,249,248,247,246,205,196,155,146,147,148,149,203,198,153,103,98,53},           //[11] Segment Pos 11
  
};

//Pixel locations for temp digits. First index is segment position on matrix (0=ones, 1=tens)
// Second index is the pixel position within the segment
unsigned int smallPixelPos[6][13] = {
  {42,9,8,7,44,57,94,107,108,109,92,59,58},               //[0] Segment Pos Temp ones
  {46,5,4,3,48,53,98,103,104,105,96,55,54},               //[1] Segment Pos Temp tens
  {36,15,14,13,38,63,88,113,114,115,86,65,64},            //[2] Segment for Temp C/F symbol
  {27,24,23,22,29,72,79,122,123,124,77,74,73},            //[3] Segment for Clock AM/PM symbol
  {307,294,295,296,305,346,355,396,395,394,357,344,345},  //[4] Segment for Scoreboard V
  {321,280,281,282,319,332,369,382,381,380,371,330,331},  //[5] Segment for Scoreboard H
};

// Pixel locations for small letters.  First index is segment position on matrix (0-5 top row, right to left, 6-11 bottom row, right to left)
// Second index is the pixel position within the segment (0-14)
unsigned int letterPixelPos[18][15] = {
  {274,227,228,229,272,279,322,329,328,327,324,277,278,323,273},    //[0]  Top row, rightmost
  {270,231,232,233,268,283,318,333,332,331,320,281,282,319,269},    //[1]  Top row, second from right
  {266,235,236,237,264,287,314,337,336,335,316,285,286,315,265},    //[2]  Top row, third from right
  {262,239,240,241,260,291,310,341,340,339,312,289,290,311,261},    //[3]  Top row, fourth from right
  {258,243,244,245,256,295,306,345,344,343,308,293,294,307,257},    //[4]  Top row, fifth from right
  {254,247,248,249,252,299,302,349,348,347,304,297,298,303,253},    //[5]  Top row, sixth from right
  {77,74,73,72,79,122,129,172,173,174,127,124,123,128,78},          //[6]  Bottom row, rightmost
  {81,70,69,68,83,118,133,168,169,170,131,120,119,132,82},          //[7]  Bottom row, second from right
  {85,66,65,64,87,114,137,164,165,166,135,116,115,136,86},          //[8]  Botton row, third from right
  {89,62,61,60,91,110,141,160,161,162,139,112,111,140,90},          //[9]  Bottom row, fourth from right
  {93,58,57,56,95,106,145,156,157,158,143,108,107,144,94},          //[10] Bottom row, fifth from right
  {97,54,53,52,99,102,149,152,153,154,147,104,103,148,98},          //[11] Bottom row, sixth from right
  {325,276,277,278,323,328,373,378,377,376,375,326,327,374,324},    //[12] Scoreboard team right, rightmost
  {321,280,281,282,319,332,369,382,381,380,371,330,331,370,320},    //[13] Scoreboard team right, middle
  {317,284,285,286,315,336,365,386,385,384,367,334,335,366,316},    //[14] Scoreboard team right, leftmost
  {311,290,291,292,309,342,359,392,391,390,361,340,341,360,310},    //[15] Socreboard team left, rightmost
  {307,294,295,296,305,346,355,396,395,394,357,344,345,356,306},    //[16] Scoreboard team left, middle
  {303,298,299,300,301,350,351,400,399,398,353,348,349,352,302},    //[17] Scoreboard team left, leftmost
};

boolean reconnect() {
  if (client.connect("MatrixClient", mqttUser, mqttPW)) {
    // Once connected, publish an announcement...
    client.publish("stat/matrix/status", "connected");
    // ... and resubscribe
    client.subscribe("cmnd/matrix/#");
  }
  return client.connected();
}
// ============================================
//   SETUP
// ============================================
void setup() {
  pinMode(3, FUNCTION_3);
  pinMode(1, FUNCTION_3);
  pinMode(COUNTDOWN_OUTPUT, OUTPUT);
  pinMode(MODE_PIN, INPUT_PULLUP); //not used
  pinMode(V1_PIN, INPUT_PULLUP);   //not used
  pinMode(V0_PIN, INPUT_PULLUP);   //Blue
  pinMode(H1_PIN, INPUT_PULLUP);   //Green
  pinMode(H0_PIN, INPUT_PULLUP);   //Red
  delay(200);

  // RTC DS3231 Setup
  Rtc.Begin();    
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!Rtc.IsDateTimeValid()) {
      if (Rtc.LastError() != 0) {
          // we have a communications error see https://www.arduino.cc/en/Reference/WireEndTransmission for what the number means
          // Serial.print("RTC communications error = ");
          // Serial.println(Rtc.LastError());
      } else {
          // Common Causes:
          //    1) first time you ran and the device wasn't running yet
          //    2) the battery on the device is low or even missing
          // Serial.println("RTC lost confidence in the DateTime!");
          // following line sets the RTC to the date & time this sketch was compiled
          // it will also reset the valid flag internally unless the Rtc device is
          // having an issue
          Rtc.SetDateTime(compiled);
      }
  }

  WiFi.setSleepMode(WIFI_NONE_SLEEP);  

  delay(200);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);  
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(LEDs, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // WiFi - AP Mode or both
#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2) 
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(APssid, APpassword);    // IP is usually 192.168.4.1
#endif

  // WiFi - Local network Mode or both
#if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2) 
  byte count = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    // Stop if cannot connect
    if (count >= 60) {
      // Could not connect to local WiFi      
      return;
    }
    delay(500);
    LEDs[count] = CRGB::Green;
    FastLED.show();
    count++;
  }
  IPAddress ip = WiFi.localIP();
#endif   
  // MQTT - Only if on local wifi and MQTT enabled
#if defined(MQTTMODE) && (MQTTMODE == 1 && (WIFIMODE == 1 || WIFIMODE == 2))
  byte mcount = 0;
  client.setServer(MQTTSERVER, MQTTPORT);
  client.setCallback(callback);
  while (!client.connected( )) {
    client.connect("MatrixClient", mqttUser, mqttPW);
    if (mcount >= 60) {
      // Could not connect to MQTT broker
      return;
    }
    delay(500);
    LEDs[mcount] = CRGB::Blue;
    FastLED.show();
    mcount++;
  }
  mqttConnected = true;
  client.subscribe("cmnd/matrix/#");
  client.publish("stat/matrix/status", "connected");
  oldMQTTMode = clockMode;
  updateMqttMode();
#endif
  httpUpdateServer.setup(&server);
  
  //OTA Updates
  ArduinoOTA.setHostname("matrix");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
  });
  ArduinoOTA.begin();
  // Set tempDisplayUpdate 
  if (temperatureUpdatePeriod > 0) {
    // set display refresh rates to seconds
    tempUpdatePeriod = (temperatureUpdatePeriod / 1000);
  }
  // Set Text Effect Speed
  if (textEffectSpeed <= 10) {    //10 is max speed
    textEffectPeriod = int(1000 / textEffectSpeed);
  } else {
    textEffectPeriod = 100;
  }
  
  // ************ Update MQTT stat vals with boot values *****************
  if (mqttConnected) {
    //Force temperature update on MQTT broker via HA if external temp selected and temp = 0
    if ((temperatureSource == 1) && (externalTemperature == 0)) {
      client.publish("cmnd/matrix/temperature/refresh", "1");
      delay(500);
    }
    updateMqttBrightness(brightness);
    updateMqttFont();
    updateMqttClockDisplay(hourFormat);
    updateMqttTemperature();
    updateMqttTempSymbol(temperatureSymbol);
    updateMqttTempCorrection();
    updateMqttCountdownStartTime(initCountdownMillis);
    updateMqttCountdownStatus(timerRunning);
    updateMqttScoreboardScores();
    updateMqttTextMessage();
    // Default boot colors
    updateMqttColor(0, clockColor.r, clockColor.g, clockColor.b);  
    updateMqttColor(1, countdownColor.r, countdownColor.g, countdownColor.b);
    updateMqttColor(2, countdownColorPaused.r, countdownColorPaused.g, countdownColorPaused.b);
    updateMqttColor(3, countdownColorFinalMin.r, countdownColorFinalMin.g, countdownColorFinalMin.b);
    updateMqttColor(4, temperatureColor.r, temperatureColor.g, temperatureColor.b);
    updateMqttColor(5, scoreboardColorLeft.r, scoreboardColorLeft.g, scoreboardColorLeft.b);
    updateMqttColor(6, scoreboardColorRight.r, scoreboardColorRight.g, scoreboardColorRight.b);
    updateMqttColor(7, textColorTop.r, textColorTop.g, textColorTop.b);
    updateMqttColor(8, textColorBottom.r, textColorBottom.g, textColorBottom.b);
    //Reset clock mode in case any MQTT updates changed it
    clockMode = 0;
    //v 0.53 Request current time in case this is a cold boot. Automation system (e.g Home Assistant/NodeRed) should handle call and send/set current date/time via MQTT call
    if (autoSetTimeOnBoot) {
      updateMQTTTime();
    }
  }
  // Handlers
  server.on("/color", HTTP_POST, []() {    
    r_val = server.arg("r").toInt();
    g_val = server.arg("g").toInt();
    b_val = server.arg("b").toInt();
    clockColor = CRGB(r_val, g_val, b_val);
    server.send(200, "text/json", "{\"result\":\"ok\"}");
    if (mqttConnected) {
      updateMqttColor(0, r_val, g_val, b_val);
    }
  });

  server.on("/setdate", HTTP_POST, []() { 
    // Sample input: date = "Dec 06 2009", time = "12:34:56"
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    String datearg = server.arg("date");
    String timearg = server.arg("time");
    char d[12];
    char t[9];
    datearg.toCharArray(d, 12);
    timearg.toCharArray(t, 9);
    RtcDateTime compiled = RtcDateTime(d, t);
    Rtc.SetDateTime(compiled);   
    clockMode = 0; 
    tempUpdateCount = 0;    
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/brightness", HTTP_POST, []() {    
    brightness = server.arg("brightness").toInt();    
    server.send(200, "text/json", "{\"result\":\"ok\"}");
    if (mqttConnected) {
      updateMqttBrightness(brightness);
    }
  });
  
  server.on("/countdown", HTTP_POST, []() {    
    countdownMilliSeconds = server.arg("ms").toInt();     
    byte cd_r_val = server.arg("r").toInt();
    byte cd_g_val = server.arg("g").toInt();
    byte cd_b_val = server.arg("b").toInt();
    digitalWrite(COUNTDOWN_OUTPUT, LOW);
    countdownColor = CRGB(cd_r_val, cd_g_val, cd_b_val); 
    endCountDownMillis = millis() + countdownMilliSeconds;
    initCountdownMillis = countdownMilliSeconds;          // store for reset to init value
    allBlank(); 
    clockMode = 1;    
    timerRunning = true; 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
    if (mqttConnected) {
      updateMqttColor(1, server.arg("r").toInt(), server.arg("g").toInt(), server.arg("b").toInt());
      updateMqttCountdownStartTime(initCountdownMillis);
      updateMqttCountdownStatus(timerRunning);
    }
  });

  server.on("/temperature", HTTP_POST, []() {   
    temperatureCorrection = server.arg("correction").toInt();
    temperatureSymbol = server.arg("symbol").toInt();
    clockMode = 0;                                    //temp shares same mode with Clock
    tempUpdateCount = 0;
    server.send(200, "text/json", "{\"result\":\"ok\"}");
    if (mqttConnected) {
      updateMqttTempCorrection();
      updateMqttTempSymbol(temperatureSymbol);
      updateMqttTemperature();
    }
  });  

  server.on("/scoreboard", HTTP_POST, []() {   
    scoreboardLeft = server.arg("left").toInt();
    scoreboardRight = server.arg("right").toInt();
    scoreboardColorLeft = CRGB(server.arg("rl").toInt(),server.arg("gl").toInt(),server.arg("bl").toInt());
    scoreboardColorRight = CRGB(server.arg("rr").toInt(),server.arg("gr").toInt(),server.arg("br").toInt());
    clockMode = 2;     
    server.send(200, "text/json", "{\"result\":\"ok\"}");
    if (mqttConnected) {
      updateMqttColor(5, server.arg("rl").toInt(),server.arg("gl").toInt(),server.arg("bl").toInt());
      updateMqttColor(6, server.arg("rr").toInt(),server.arg("gr").toInt(),server.arg("br").toInt());
      updateMqttScoreboardScores();
    }
  });  

  server.on("/hourformat", HTTP_POST, []() {   
    hourFormat = server.arg("hourformat").toInt();
    clockMode = 0; 
    tempUpdateCount = 0;    
    server.send(200, "text/json", "{\"result\":\"ok\"}");
    if (mqttConnected) {
      updateMqttClockDisplay(hourFormat);
    }
  }); 

  server.on("/clock", HTTP_POST, []() {       
    clockMode = 0;
    tempUpdateCount = 0;     
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  

  // New web commands for OTA updates - v1.00
  server.on("/restart",[](){
    server.send(200, "text/html", "<h1>Restarting...</h1>");
    delay(1000);
    ESP.restart();
  });
  server.on("/otaupdate",[]() {
    server.send(200, "text/html", "<h1>Ready for upload...<h1><h3>Start upload from IDE now</h3>");
    ota_flag = true;
    ota_time_window = 20000;
    ota_time_elapsed = 0;
  });
  
  // Before uploading the files with the "ESP8266 Sketch Data Upload" tool, zip the files (if not already zipped) with the command "gzip -r ./data/" (on Windows you can do this with a Git Bash)
  // *.gz files are automatically unpacked and served from your ESP (so you don't need to create a handler for each file).
  server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  server.begin();     

  SPIFFS.begin();
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
  }
  
  digitalWrite(COUNTDOWN_OUTPUT, LOW);
  //Force MQTT Temperature update from MQTT on boot and if external temp = 0
};

// =============================================================
// *************** MQTT Message Processing *********************
// =============================================================
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = (char*)payload;
  
  // Mode
  if (strcmp(topic,"cmnd/matrix/mode")==0){
    clockMode = message.toInt();
    if (clockMode == 0)
      tempUpdateCount = 0;
    updateMqttMode();
    return;
   
  // Brightness
  } else if (strcmp(topic,"cmnd/matrix/brightness") == 0) {
      int testVal = message.toInt();
      if ((testVal >= 0) && (testVal <= 255)) {
        brightness = message.toInt();
        updateMqttBrightness(brightness);
      }
      return;
  // Large Number fort
  } else if (strcmp(topic, "cmnd/matrix/font") == 0) {
      int font = message.toInt();
      if (font > 2) {
        font = 0;
      }
      numFont = font;
      updateMqttFont();
      return;
  // Buzzer
  } else if (strcmp(topic, "cmnd/matrix/buzzer") == 0) {
      int buzzTime = message.toInt();
      if (buzzTime > 9999)    //max time 9.99 sec to avoid blocking issues
        buzzTime = 9999;
      soundBuzzer(buzzTime);
  
  // Clock
    // Color
  } else if (strcmp(topic, "cmnd/matrix/clock/color") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    clockColor = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(0, mqqt_r_val, mqqt_g_val, mqqt_b_val);
    // Display 12/24
  } else if (strcmp(topic, "cmnd/matrix/clock/display") == 0) {
    int testVal = message.toInt();
    if ((testVal == 12) || (testVal == 24)) {
      hourFormat = message.toInt();
      clockMode = 0;
      tempUpdateCount = 0;
      updateMqttClockDisplay(hourFormat);
    }
    // Time Set
  } else if (strcmp(topic, "cmnd/matrix/clock/settime") == 0) {
    int timeDelim = message.indexOf(";");
    String datearg = message.substring(0, timeDelim);
    String timearg = message.substring(timeDelim+1);
    char d[12];
    char t[9];
    datearg.toCharArray(d, 12);
    timearg.toCharArray(t, 9);
    RtcDateTime compiled = RtcDateTime(d, t);
    Rtc.SetDateTime(compiled);   
    clockMode = 0;
    tempUpdateCount = 0;     

 // Countdown    
    // Color
  } else if (strcmp(topic, "cmnd/matrix/countdown/color") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    countdownColor = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(1, mqqt_r_val, mqqt_g_val, mqqt_b_val);
    // Color Paused 
  } else if (strcmp(topic, "cmnd/matrix/countdown/colorpaused") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    countdownColorPaused = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(2, mqqt_r_val, mqqt_g_val, mqqt_b_val);
    // Color Final Min
  } else if (strcmp(topic, "cmnd/matrix/countdown/colorfinalmin") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    countdownColorFinalMin = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(3, mqqt_r_val, mqqt_g_val, mqqt_b_val);
  } else if (strcmp(topic, "cmnd/matrix/countdown/starttime") == 0) {
    int firstDelim = message.indexOf(":");
    int secondDelim = message.indexOf(":", firstDelim+1);
    int hh_val = message.substring(0, firstDelim).toInt();
    int mm_val = message.substring(firstDelim+1, secondDelim).toInt();
    int ss_val = message.substring(secondDelim+1).toInt();
    if (hh_val > 23)
    hh_val = 23;
    if (mm_val > 59)
    mm_val = 59;
    if (ss_val > 59)
    ss_val = 59;
    timerRunning = false;     //Stop timer if running to set time
    initCountdownMillis =  (hh_val * 3600000) + (mm_val * 60000) + (ss_val * 1000);  //convert to ms
    countdownMilliSeconds = initCountdownMillis;
    remCountdownMillis = initCountdownMillis;
    endCountDownMillis = countdownMilliSeconds + millis();
    updateMqttCountdownStartTime(initCountdownMillis);
    updateMqttCountdownStatus(timerRunning);
  } else if (strcmp(topic, "cmnd/matrix/countdown/action") == 0) {  
    int whichAction = message.toInt();
    switch(whichAction) {
      case 0:       // Start/resume countdown
        if (!timerRunning && remCountdownMillis > 0) {
          endCountDownMillis = millis() + remCountdownMillis;
          timerRunning = true;
          updateMqttCountdownAction(0);
          break;
        }
      case 1:       // Pause/stop countdown
        timerRunning = false;
        updateMqttCountdownAction(1);
        break;
      case 2:       // Stop (if running) and reset to last start time
        timerRunning = false;
        countdownMilliSeconds = initCountdownMillis;
        remCountdownMillis = initCountdownMillis;
        endCountDownMillis = countdownMilliSeconds + millis();
        updateMqttCountdownAction(2);
        break;
      case 3:       // Stop (if running), set to 00:00 and clear init start time
        timerRunning = false;
        countdownMilliSeconds = 0;
        endCountDownMillis = 0;
        remCountdownMillis = 0;
        initCountdownMillis = 0;
        updateMqttCountdownStartTime(initCountdownMillis);
        updateMqttCountdownAction(3);
        break; 
    }
    updateMqttCountdownStatus(timerRunning);

  // Temperature
    // Color
  } else if (strcmp(topic, "cmnd/matrix/temperature/color") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    temperatureColor = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(4, mqqt_r_val, mqqt_g_val, mqqt_b_val);
    if (clockMode == 0) {
      tempUpdateCount = 0;  
    }
    //Source - internal/external
  } else if (strcmp(topic, "cmnd/matrix/temperature/external") == 0) {
    externalTemperature = message.toInt();
    if (clockMode == 0) {
      tempUpdateCount = 0;  
    }
    updateMqttTemperature();
    // Symbol (12 = C, 13 = F)
  } else if (strcmp(topic, "cmnd/matrix/temperature/symbol") == 0) {
    int testVal = message.toInt();
    if ((testVal == 12) || (testVal == 13)) {
      temperatureSymbol = message.toInt();
      clockMode = 0;
      tempUpdateCount = 0;
      updateMqttTempSymbol(temperatureSymbol);
      updateMqttTemperature();  
    }
    // Correction
  } else if (strcmp(topic, "cmnd/matrix/temperature/correction") == 0) {
    temperatureCorrection = message.toInt();
    clockMode = 0;
    tempUpdateCount = 0;
    updateMqttTempCorrection();
    updateMqttTemperature();
  
  // Scoreboard
    // Left Color
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/colorleft") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    scoreboardColorLeft = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(5, mqqt_r_val, mqqt_g_val, mqqt_b_val);
    // Right Color
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/colorright") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    scoreboardColorRight = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(6, mqqt_r_val, mqqt_g_val, mqqt_b_val);
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/teamleft") == 0) {
    scoreboardTeamLeft = message.substring(0,4); 
    clockMode = 2;
    updateMqttScoreboardScores();
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/teamright") == 0) {
    scoreboardTeamRight = message.substring(0,4);
    updateMqttScoreboardScores();
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/scoreleft") == 0) {
    scoreboardLeft = message.toInt();
    if ((scoreboardLeft > 99) || (scoreboardLeft < 0))
      scoreboardLeft = 0;
    updateMqttScoreboardScores();
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/scoreright") == 0) {
    scoreboardRight = message.toInt();
    if ((scoreboardRight > 99) || (scoreboardRight < 0))
      scoreboardRight = 0;
    updateMqttScoreboardScores();
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/scoreup") == 0) {
    unsigned int whichScore = message.toInt();     // 0=left score, 1=right score, 2=both
    switch(whichScore) {
      case 0:
        scoreboardLeft = scoreboardLeft + 1;
        if (scoreboardLeft > 99)
          scoreboardLeft = 0;
          break;
      case 1:
        scoreboardRight = scoreboardRight + 1;
        if (scoreboardRight > 99)
          scoreboardRight = 0;
          break;
      case 2:
        scoreboardLeft = scoreboardLeft + 1;
        if (scoreboardLeft > 99)
          scoreboardLeft = 0;
        scoreboardRight = scoreboardRight + 1;
        if (scoreboardRight > 99)
          scoreboardRight = 0;
          break;
    }
    updateMqttScoreboardScores();
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/scoredown") == 0) {
    unsigned int whichScore = message.toInt();     // 0=left score, 1=right score, 2=both
    switch(whichScore) {
      case 0:
        if (scoreboardLeft > 0)
          scoreboardLeft = scoreboardLeft - 1;
          break;
      case 1:
        if (scoreboardRight > 0)
          scoreboardRight = scoreboardRight - 1;
          break;
      case 2:
        if (scoreboardLeft > 0)
          scoreboardLeft = scoreboardLeft - 1;
        if (scoreboardRight > 0)
          scoreboardRight = scoreboardRight - 1;
          break;
    }
    updateMqttScoreboardScores();
  } else if (strcmp(topic, "cmnd/matrix/scoreboard/reset") == 0) {
    unsigned int whichScore = message.toInt();     // 0=left score, 1=right score, 2=both
    switch(whichScore) {
      case 0:
        scoreboardLeft = 0;
        break;
      case 1:
        scoreboardRight = 0;
        break;
      case 2:
        scoreboardLeft = 0;
        scoreboardRight = 0;
        break;
    }
    updateMqttScoreboardScores();

  // Text display
    // Top Color
  } else if (strcmp(topic, "cmnd/matrix/text/colortop") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    textColorTop = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(7, mqqt_r_val, mqqt_g_val, mqqt_b_val);
    // Bottom Color
  } else if (strcmp(topic, "cmnd/matrix/text/colorbottom") == 0) {
    int firstDelim = message.indexOf(",");
    int secondDelim = message.indexOf(",", firstDelim+1);
    int mqqt_r_val = message.substring(0, firstDelim).toInt();
    int mqqt_g_val = message.substring(firstDelim+1, secondDelim).toInt();
    int mqqt_b_val = message.substring(secondDelim+1).toInt();
    textColorBottom = CRGB(mqqt_r_val, mqqt_g_val, mqqt_b_val);
    updateMqttColor(8, mqqt_r_val, mqqt_g_val, mqqt_b_val);
    // Text Message
  } else if (strcmp(topic, "cmnd/matrix/text/message") == 0){
    int textDelim = message.indexOf("|");
    textFull = message;
    textTop = message.substring(0, textDelim);
    if (textDelim > -1) {
      textBottom = message.substring(textDelim+1);
    } else {
      textBottom = "";
    }
    updateMqttTextMessage();
  } else if (strcmp(topic, "cmnd/matrix/text/effect") == 0) {
    textEffect = message.toInt();
    updateMqttTextMessage();
  } else if (strcmp(topic, "cmnd/matrix/text/speed") == 0) {
    textEffectSpeed = message.toInt();
    if ((textEffectSpeed > 0) && (textEffectSpeed <= 10)) {
      textEffectPeriod = int((11-textEffectSpeed) * 200);
    } else {
      textEffectPeriod = 1000;    //default to 1 sec
    }
    updateMqttTextMessage();
  }
};

// ===============================================================
//   MAIN LOOP
// ===============================================================
void loop() {
  // When OTA flag set via HTML call, time to upload set at 20 sec. via server callback above.  Alter there if more time desired.
  if (ota_flag) {
    displayOTA();
    uint16_t ota_time_start = millis();
    while (ota_time_elapsed < ota_time_window) {
      ArduinoOTA.handle();  
      ota_time_elapsed = millis()-ota_time_start;   
      delay(10); 
    }
    ota_flag = false;
    tempUpdateCount = 0;
    allBlank();
  }

  server.handleClient();

  if (MQTTMODE == 1) {
    if (!client.connected()) {
      long now = millis();
      if (now - lastReconnectAttempt > 60000) {   //attempt reconnect once per minute. Drop usually a result of Home Assistant/broker server restart
        lastReconnectAttempt = now;
        // Attempt to reconnect
        if (reconnect()) {
          lastReconnectAttempt = 0;
          mqttConnected = true;
        }
      }
    } else {
      // Client connected
      client.loop();
    }
  }

  int modeReading = digitalRead(V0_PIN);  //Blue
  int v1Reading = digitalRead(H1_PIN);    //Green
  int h1_Reading = digitalRead(H0_PIN);   //Red

  // Mode Button (Blue)
  if (modeReading == LOW) { 
    clockMode = clockMode + 1; 
    delay(500);
  }
  if (clockMode > 3) {         
    clockMode = 0;
    tempUpdateCount = 0;
  }

//V+ Button (Green): Increase visitor score / Toggle timer (run/stop) / Next text effect
  if (v1Reading == LOW && h1_Reading == !LOW) {
    if (clockMode == 2) {
      scoreboardLeft = scoreboardLeft + 1;
      if (scoreboardLeft > 99)
        scoreboardLeft = 0;
      if (mqttConnected)
        updateMqttScoreboardScores();
    } else if (clockMode == 1) {
      if (!timerRunning && remCountdownMillis > 0) {
        endCountDownMillis = millis() + remCountdownMillis;
        timerRunning = true;
        if (mqttConnected)
          updateMqttCountdownAction(0);
      } else if (timerRunning) {
        timerRunning = false;  
        if (mqttConnected)
          updateMqttCountdownAction(1);
      }  
      if (mqttConnected)
        updateMqttCountdownStatus(timerRunning);
    } else if (clockMode == 3) {
       textEffect = textEffect + 1;
       if (textEffect > 7) {
         textEffect = 0;
         oldTextEffect = textEffect; 
       }
    }
    delay(500);
  }
  //H+ Button (Red): Increase home score / Reset timer to starting value / Clear notification (e.g. YouTube)
  if (h1_Reading == LOW && v1Reading == !LOW) {
    if (clockMode == 2) {
      scoreboardRight = scoreboardRight + 1;
      if (scoreboardRight > 99) 
        scoreboardRight = 0;
      if (mqttConnected)
        updateMqttScoreboardScores();
    } else if (clockMode == 1) {
      if (!timerRunning) {
        // Reset timer to last start value
        countdownMilliSeconds = initCountdownMillis;
        remCountdownMillis = initCountdownMillis;
        endCountDownMillis = countdownMilliSeconds + millis();
        if (mqttConnected)
          updateMqttCountdownAction(2);
      } else {
        timerRunning = false;
        if (mqttConnected) {
          updateMqttCountdownStatus(timerRunning);
          updateMqttCountdownAction(1);
        }
      }
    } else if (clockMode == 3) {
      textEffect = 0;
      oldTextEffect = 0;
      allBlank();
      tempUpdateCount = 0;
      clockMode = 0;
    }
    delay(500);
  }

  //V+ and H+ buttons (green + red): Reset scoreboard scores / clear starting time
   if (v1Reading == LOW && h1_Reading == LOW) {
     if (clockMode == 2) {
       scoreboardLeft = 0;
       scoreboardRight = 0;
       if (mqttConnected)
         updateMqttScoreboardScores();
     } else if (clockMode == 1  && !timerRunning) {
       countdownMilliSeconds = 0;
       endCountDownMillis = 0;
       remCountdownMillis = 0;
       initCountdownMillis = 0;
       if (mqttConnected)
         updateMqttCountdownStartTime(initCountdownMillis);
         updateMqttCountdownAction(3);
     }
   delay(500);
   }
  
  unsigned long currentMillis = millis(); 
  // Text Effect processing
  //Flash, Flash Alternate, Fade In and Fade Out
  if ((clockMode == 3) && (textEffect > 0)) {
    yield();
    if ((currentMillis - prevTime) >= textEffectPeriod) {
      prevTime = currentMillis;
      switch (textEffect) {
        case 1:   //Flash
          if (textEffect != oldTextEffect){
            oldTextEffect = textEffect;
          }
          updateTextFlash();
          FastLED.setBrightness(brightness);
          break;
        case 2:   //FlashAlternate
          if (textEffect != oldTextEffect){
            oldTextEffect = textEffect;
          }
          updateTextFlashAlternate();
          FastLED.setBrightness(brightness);
          break;
        case 3:  //Fade In
          if (textEffect != oldTextEffect) {
            effectBrightness = 0;
            oldTextEffect = textEffect;
          }
          updateTextFadeIn();
          FastLED.setBrightness(effectBrightness);
          break;
        case 4:  //Fade Out
          if (textEffect != oldTextEffect) {
            effectBrightness = brightness;
            oldTextEffect = textEffect;
          }
          updateTextFadeOut();
          FastLED.setBrightness(effectBrightness);
          break;
        case 5:  //Appear
          if (textEffect != oldTextEffect) {
            appearCount = 99;
            oldTextEffect = textEffect;
          }
          updateTextAppear(false);
          FastLED.setBrightness(brightness);
          break;
        case 6:  //Appear Flash
          if (textEffect != oldTextEffect) {
            appearCount = 99;
            oldTextEffect = textEffect;
          }
          updateTextAppear(true);
          FastLED.setBrightness(brightness);
          break;
        case 7:  //Rainbow (random letter colors)
          if (textEffect != oldTextEffect){
            oldTextEffect = textEffect;
          }
          updateTextRainbow();
          FastLED.setBrightness(brightness);
          break;
        default:
          textEffect = 0;
          break;
      }
      FastLED.show();
    }
  } else if (currentMillis - prevTime >= 1000) {
    prevTime = currentMillis;
    
    if (oldMode != clockMode) {  //If mode has changed, clear the display first
      allBlank();
      oldMode = clockMode;
    }

    if (clockMode == 0) {
      updateClock();
      if (tempUpdateCount <= 0) {
        updateTemperature(); 
        tempUpdateCount = tempUpdatePeriod;      
      }
      tempUpdateCount -= 1;
    } else if (clockMode == 1) {
      updateCountdown();
    } else if (clockMode == 2) {
      updateScoreboard();            
    } else if (clockMode == 3) {
      updateText();
    }


    FastLED.setBrightness(brightness);
    FastLED.show();
    // Update MQTT here if enabled and connected
    if (mqttConnected) {
      unsigned long currTempMillis = millis();
      // Mode
      if (clockMode != oldMQTTMode) {
        updateMqttMode();
        oldMQTTMode = clockMode;
      }
      if ((temperatureUpdatePeriod > 0) && (currTempMillis - prevTempTime >= temperatureUpdatePeriod)) { 
        prevTempTime = currTempMillis;     
        updateMqttTemperature();
      }
    }
  }
}

// ===============================================================
//   UPDATE FUNCTIONS
// ===============================================================
void updateClock() {
  RtcDateTime now = Rtc.GetDateTime();
  // printDateTime(now);    

  int hour = now.Hour();
  int mins = now.Minute();
  int secs = now.Second();
  byte AmPm = 16;   // Default to no symbol

  //Get Am/Pm symbol if 12 hour mode
  if (hourFormat == 12 && hour < 12) {
    AmPm = 10;
  } else if (hourFormat == 12 && hour >= 12) {
    AmPm = 11;
  }
  
  if (hourFormat == 12 && hour > 12) {
    hour = hour - 12;
  } else if (hourFormat == 12 && hour == 0) {     //fix for midnight - 1 am, where previously showed "0" for hour
    hour = 12;
  }
 
  byte h1 = hour / 10;
  byte h2 = hour % 10;
  byte m1 = mins / 10;
  byte m2 = mins % 10;  
  byte s1 = secs / 10;
  byte s2 = secs % 10;
  
  CRGB color = clockColor;     //CRGB(r_val, g_val, b_val);

  if (h1 > 0 || (hourFormat == 24 && hour == 0))  //display leading zero for midnight when 24-hour display
    displayNumber(h1,3,color);
  else 
    displayNumber(10,3,color);  // Blank
    
  displayNumber(h2,2,color);
  displayNumber(m1,1,color);
  displayNumber(m2,0,color); 
  //Display A/P symbol if in 12-hour mode
  displaySmallNum(AmPm, 3, color);
  displayDots(color);  
}

void updateCountdown() {
  if (countdownMilliSeconds == 0 && endCountDownMillis == 0) {
    displayNumber(0,7,countdownColorPaused);
    displayNumber(0,6,countdownColorPaused);
    displayNumber(0,5,countdownColorPaused);
    displayNumber(0,4,countdownColorPaused);  
    LEDs[237] = countdownColorPaused;
    LEDs[187] = countdownColorPaused;
    return;    
  }

  if (!timerRunning) {
    unsigned long restMillis = remCountdownMillis;
    unsigned long hours = ((restMillis / 1000) / 60) / 60;
    unsigned long minutes = (restMillis / 1000) / 60;
    unsigned long seconds = restMillis / 1000;
    int remSeconds = seconds - (minutes * 60);
    int remMinutes = minutes - (hours * 60); 
    byte h1 = hours / 10;
    byte h2 = hours % 10;
    byte m1 = remMinutes / 10;
    byte m2 = remMinutes % 10;  
    byte s1 = remSeconds / 10;
    byte s2 = remSeconds % 10;
    if (hours > 0) {
      // hh:mm
      displayNumber(h1,7,countdownColorPaused); 
      displayNumber(h2,6,countdownColorPaused);
      displayNumber(m1,5,countdownColorPaused);
      displayNumber(m2,4,countdownColorPaused);  
    } else {
      // mm:ss   
      displayNumber(m1,7,countdownColorPaused);
      displayNumber(m2,6,countdownColorPaused);
      displayNumber(s1,5,countdownColorPaused);
      displayNumber(s2,4,countdownColorPaused);  
    }
    LEDs[237] = countdownColorPaused;
    LEDs[187] = countdownColorPaused;
    return;
  }
    
  unsigned long restMillis = endCountDownMillis - millis();
  unsigned long hours   = ((restMillis / 1000) / 60) / 60;
  unsigned long minutes = (restMillis / 1000) / 60;
  unsigned long seconds = restMillis / 1000;
  int remSeconds = seconds - (minutes * 60);
  int remMinutes = minutes - (hours * 60); 
  
  byte h1 = hours / 10;
  byte h2 = hours % 10;
  byte m1 = remMinutes / 10;
  byte m2 = remMinutes % 10;  
  byte s1 = remSeconds / 10;
  byte s2 = remSeconds % 10;

  remCountdownMillis = restMillis;        //Store current remaining in event of pause

  CRGB color = countdownColor;
  if (restMillis <= 60000) {
    color = countdownColorFinalMin;             
  }

  if (hours > 0) {
    // hh:mm
    displayNumber(h1,7,color); 
    displayNumber(h2,6,color);
    displayNumber(m1,5,color);
    displayNumber(m2,4,color);  
  } else {
    // mm:ss   
    displayNumber(m1,7,color);
    displayNumber(m2,6,color);
    displayNumber(s1,5,color);
    displayNumber(s2,4,color);  
  }

  displayDots(color);  

  if (hours <= 0 && remMinutes <= 0 && remSeconds <= 0) {
    //endCountdown();
    countdownMilliSeconds = 0;
    endCountDownMillis = 0;
    remCountdownMillis = 0;
    timerRunning = false;
    FastLED.setBrightness(brightness);
    FastLED.show();
    digitalWrite(COUNTDOWN_OUTPUT, HIGH);
    delay(2000);  //sound for 2 seconds
    digitalWrite(COUNTDOWN_OUTPUT, LOW);
    updateMqttCountdownStatus(timerRunning);
    return;
  }  
}

void updateTemperature() {
  float ctemp = 0;
  bool isNegative = false;
  if (temperatureSource == 0) {
    RtcTemperature temp = Rtc.GetTemperature();
    float ftemp = temp.AsFloatDegC();

    if (temperatureSymbol == 13)
      ftemp = (ftemp * 1.8000) + 32;

    float ctemp = ftemp + temperatureCorrection;
    
  } else if (temperatureSource == 1) {
    ctemp = (externalTemperature * 1.0);
  }

  if (ctemp < 0) {      //Flip sign and set isNegative to true since byte cannot contain negative num
    ctemp = ctemp * -1;
    isNegative = true;
  }
  byte t1 = int(ctemp) / 10;
  byte t2 = int(ctemp) % 10;

  //Hide leading zero
  if (t1 == 0)
    t1 = 16;  //blank
  displaySmallNum(t1,1,temperatureColor);
  displaySmallNum(t2,0,temperatureColor);
  displaySmallNum(temperatureSymbol,2,temperatureColor);
  LEDs[110] = temperatureColor;   //degree dot
  if (isNegative) {
    //show negative
    LEDs[50] = temperatureColor;
    LEDs[51] = temperatureColor;
  } else {
    LEDs[50] = CRGB::Black;
    LEDs[51] = CRGB::Black;
  }
}

void updateScoreboard() {
  byte sl1 = scoreboardLeft / 10;
  byte sl2 = scoreboardLeft % 10;
  byte sr1 = scoreboardRight / 10;
  byte sr2 = scoreboardRight % 10;
  byte vLen = scoreboardTeamLeft.length() + 1;
  byte hLen = scoreboardTeamRight.length() + 1;
  char v_array[vLen];
  char h_array[hLen];
  byte letternum = 0;   //Default to blank space
  scoreboardTeamLeft.toCharArray(v_array, vLen);
  scoreboardTeamRight.toCharArray(h_array, hLen);

  displayNumber(sl1,11,scoreboardColorLeft);
  displayNumber(sl2,10,scoreboardColorLeft);
  displayNumber(sr1,9,scoreboardColorRight);
  displayNumber(sr2,8,scoreboardColorRight);

  //Show Team Names
  // Left - visitor
  for (byte i=0; i<3; i++) {
    yield();
    if (i <= (vLen-1)) {
      letternum = getLetterIndex(v_array[i]);
    } else {
      letternum = 0;
    }
    displayLetter(letternum, (17-i), scoreboardColorLeft); 
  }
  // Right - home
  for (byte i=0; i<3; i++) {
    yield();
    if (i <= (hLen-1)) {
      letternum = getLetterIndex(h_array[i]);
    } else {
      letternum = 0;
    }
    displayLetter(letternum, (14-i), scoreboardColorRight); 
  }
  
  hideDots();
}

void updateText() {
  byte topLen = textTop.length() + 1;
  byte botLen = textBottom.length() + 1;
  char top_array[topLen];
  char bottom_array[botLen];
  textTop.toCharArray(top_array, topLen);
  textBottom.toCharArray(bottom_array, botLen);
  byte letternum = 0;    //Default to blank space

  //Top row
  for (byte i=0; i<6; i++){
    yield();
    if (i <= (topLen-1)) {
      letternum = getLetterIndex(top_array[i]);
    } else {
      letternum = 0;
    }
    displayLetter(letternum, (5-i), textColorTop);
  }
  //Bottom row
  for (byte i=0; i<6; i++){
    yield();
    if (i <= (botLen-1)) {
      letternum = getLetterIndex(bottom_array[i]);
    } else {
      letternum = 0;
    }
    displayLetter(letternum, (11-i), textColorBottom);
  }
  
}

//Text Effect Mathods
//Flash (both rows on/off together)
void updateTextFlash() {
  yield();
  if (effectTextHide) {
      allBlank();
    } else {
    updateText();
  }
  effectTextHide = !effectTextHide;
}

//Flash Alternate (one row on, one row off)
void updateTextFlashAlternate() {
  byte letternum = 0;
  allBlank();
  //Show 1st row when effectTextHide false
  if (effectTextHide) {
    byte topLen = textTop.length() + 1;
  char top_array[topLen];
  textTop.toCharArray(top_array, topLen);
    for (byte i=0; i<6; i++){
      yield();
      if (i <= (topLen-1)) {
        letternum = getLetterIndex(top_array[i]);
      } else {
        letternum = 0;
      }
    displayLetter(letternum, (5-i), textColorTop);
    }
  } else {
    byte botLen = textBottom.length() + 1;
    char bottom_array[botLen];
    textBottom.toCharArray(bottom_array, botLen);
    for (byte i=0; i<6; i++){
      yield();
      if (i <= (botLen-1)) {
        letternum = getLetterIndex(bottom_array[i]);
      } else {
        letternum = 0;
      }
      displayLetter(letternum, (11-i), textColorBottom);
    }
  }
  effectTextHide = !effectTextHide;
}

void updateTextFadeIn(){
  if (effectBrightness == 0) {
    allBlank();
    effectBrightness += int(brightness / 5);
  } else if (effectBrightness > brightness) {
    effectBrightness = 0;
  } else {
    updateText(); 
    effectBrightness += int(brightness / 5);
  }
}
//Fade Out
void updateTextFadeOut() {
  if (effectBrightness <= 0) {
    allBlank();
    effectBrightness = brightness;
  } else {
    updateText();
    effectBrightness = (effectBrightness - int((brightness / 5)));
  }
}

//Appear and Appear Flash
void updateTextAppear(bool flash) {
  byte topLen = textTop.length() + 1;
  byte botLen = textBottom.length() + 1;
  byte letternum = 0;
  char top_array[topLen];
  char bottom_array[botLen];
  textTop.toCharArray(top_array, topLen);
  textBottom.toCharArray(bottom_array, botLen);
  yield();
  if (appearCount >= 99) {
    allBlank();
    appearCount = 5;
  } else if ((appearCount >= 14) && (appearCount <= 20)) {
    // just skip and hold or start flash
    if (flash) {      //Start flash for 6 loops (3 on 3 off)
      updateTextFlash();
      appearCount = appearCount-1;
    } else {
      appearCount = 99;
    }
  } else if ((appearCount == 12) || (appearCount == 13)) {
    appearCount = 99;
  } else if ((appearCount >= 6) && (appearCount <= 11)) {  //Bottom row
    letternum = getLetterIndex(bottom_array[11 - appearCount]);
    displayLetter(letternum, appearCount, textColorBottom);
    if (appearCount == 6) {
      appearCount = 20;   //finished display
    } else {
      appearCount = appearCount-1;
    }
  
  } else if ((appearCount >= 0) && (appearCount <= 5)) {   //Top row
    letternum = getLetterIndex(top_array[5 - appearCount]);
    displayLetter(letternum, appearCount, textColorTop);
    if (appearCount == 0) {
      appearCount = 11;   //Move to bottom row
    } else {
      appearCount = appearCount - 1;
    }
  }
  return; 
}

//Rainbow (random colors)
void updateTextRainbow() {
  byte topLen = textTop.length() + 1;
  byte botLen = textBottom.length() + 1;
  char top_array[topLen];
  char bottom_array[botLen];
  textTop.toCharArray(top_array, topLen);
  textBottom.toCharArray(bottom_array, botLen);
  byte letternum = 0;    //Default to blank space
  byte r_val = 255;
  byte g_val = 255;
  byte b_val = 255;
  //Top row
  for (byte i=0; i<6; i++){
    yield();
    if (i <= (topLen-1)) {
      letternum = getLetterIndex(top_array[i]);
    } else {
      letternum = 0;
    }
    r_val = getRandomColor();
    g_val = getRandomColor();
    b_val = getRandomColor();
    displayLetter(letternum, (5-i), CRGB(r_val, g_val, b_val));
  }
  //Bottom row
  for (byte i=0; i<6; i++){
    yield();
    if (i <= (botLen-1)) {
      letternum = getLetterIndex(bottom_array[i]);
    } else {
      letternum = 0;
    }
    r_val = getRandomColor();
    g_val = getRandomColor();
    b_val = getRandomColor();
    displayLetter(letternum, (11-i), CRGB(r_val, g_val, b_val));
  }
  return;
}


byte getLetterIndex(byte letterval) {
  byte index = 0;
  if ((letterval >= 32) && (letterval <=96)) {            //Symbols, numbers & uppercase letters
    index = letterval - 32;
  } else if ((letterval >= 97) && (letterval <= 122)) {   //lowercase letters
    index = letterval - 64; 
  }
  return index;
}

byte getRandomColor() {
  byte colorVal = 0;
  colorVal = random(0,256);
  return colorVal;
}
// ===============================================================
//   DISPLAY FUNCTIONS
//   For all LED[x] calls, subtract 1 since array is 0-based,
//   but segments are based on actual pixel number which is 1-based.
// ===============================================================
void displayNumber(byte number, byte segment, CRGB color) {
  /*
    number passed from numbers[x][y] array.  [x] defines 'font' number. [y] is the digit/character
    Segment is defined in the fullnumPixelPos[] array 
   */
  unsigned int pixelPos = 0;
  for (byte i=0; i<32; i++){
    yield();
    pixelPos = (fullnumPixelPos[segment][i] - 1);   
    LEDs[pixelPos] = ((numbers[numFont][number] & 1 << i) == 1 << i) ? color : alternateColor;
  } 
}

void displaySmallNum(byte number, byte segment, CRGB color) {
  /*
   *      7  8  9  Segments 0,1 Temperature digits
   *      6    10  Segment  2 Temperature Symbol (F/C)
   *      5 12 11  Segment  3 A/P time symbol
   *      4     0  Segment  4 V(istor) symbol
   *      3  2  1  Segment  5 H(ome) symbol
   */
  unsigned int pixelPos = 0;
  for (byte i=0; i<13; i++) {
    yield();
    pixelPos = (smallPixelPos[segment][i] - 1);
    LEDs[pixelPos] = ((smallNums[number] & 1 << i) == 1 << i) ? color : alternateColor;
  }
}

void displayLetter(byte number, byte segment, CRGB color) {
  /*
   *      7  8  9  Segments 0-5 Top row, right to left
   *      6 13 10  Segments 6-11 Bottom row, right to left    
   *      5 12 11  
   *      4 14  0  
   *      3  2  1  
   */
  unsigned int pixelPos = 0;
  for (byte i=0; i<15; i++) {
    yield();
    pixelPos = (letterPixelPos[segment][i] - 1);
    LEDs[pixelPos] = ((letters[number] & 1 << i) == 1 << i) ? color : alternateColor;
  }
}

void allBlank() {
  for (int i=0; i<NUM_LEDS; i++) {
    yield();
    LEDs[i] = CRGB::Black;
  }
  FastLED.show();
}

void displayOTA() {
  // Hardcoded for now, but in order from 0-12 for each latter
  CRGB color = CRGB::Red;
  allBlank();
  // Subtract 1 from physical pixel number 
  // U - 0b0111011111111
  LEDs[196] = color;
  LEDs[153] = color;
  LEDs[152] = color;
  LEDs[151] = color;
  LEDs[198] = color;
  LEDs[201] = color;
  LEDs[248] = color;
  LEDs[251] = color;
  LEDs[253] = color;
  LEDs[246] = color;
  LEDs[203] = color;
  //P - 0b1111111111000
  LEDs[155] = color;
  LEDs[194] = color;
  LEDs[205] = color;
  LEDs[244] = color;
  LEDs[255] = color;
  LEDs[256] = color;
  LEDs[257] = color;
  LEDs[242] = color;
  LEDs[207] = color;
  LEDs[206] = color;
  //L - 0b0000011111110
  LEDs[161] = color;
  LEDs[160] = color;
  LEDs[159] = color;
  LEDs[190] = color;
  LEDs[209] = color;
  LEDs[240] = color;
  LEDs[259] = color;
  //O - 0b0111111111111
  LEDs[184] = color;
  LEDs[165] = color;
  LEDs[164] = color;
  LEDs[163] = color;
  LEDs[186] = color;
  LEDs[213] = color;
  LEDs[236] = color;
  LEDs[263] = color;
  LEDs[264] = color;
  LEDs[265] = color;
  LEDs[234] = color;
  LEDs[215] = color;
  //A - 0b1111111111011
  LEDs[180] = color;
  LEDs[169] = color;
  LEDs[167] = color;
  LEDs[182] = color;
  LEDs[217] = color;
  LEDs[232] = color;
  LEDs[267] = color;
  LEDs[268] = color;
  LEDs[269] = color;
  LEDs[230] = color;
  LEDs[219] = color;
  LEDs[218] = color;
  //D - 0b0110111111101
  LEDs[176] = color;
  LEDs[172] = color;
  LEDs[171] = color;
  LEDs[178] = color;
  LEDs[221] = color;
  LEDs[228] = color;
  LEDs[271] = color;
  LEDs[272] = color;
  LEDs[226] = color;
  LEDs[223] = color;
  
  FastLED.setBrightness(brightness);
  FastLED.show();
}

void displayDots(CRGB color) {
  if (dotsOn) {
    LEDs[237] = color;
    if (clockMode == 0) {
      LEDs[287] = color;
    } else if (clockMode = 1) {
      LEDs[187] = color;
    }
  } else {
    LEDs[187] = CRGB::Black;
    LEDs[237] = CRGB::Black;
    LEDs[287] = CRGB::Black;
  }
  dotsOn = !dotsOn;  
}

void hideDots() {
  LEDs[187] = CRGB::Black;
  LEDs[237] = CRGB::Black;
  LEDs[287] = CRGB::Black;
}

void soundBuzzer(unsigned int buzzLength) {
    digitalWrite(COUNTDOWN_OUTPUT, HIGH);
    delay(buzzLength);  //time in millis -  do not exceed about 3000 millis
    digitalWrite(COUNTDOWN_OUTPUT, LOW);
  
}
// ==================================================
//   MQTT UPDATING
// ==================================================
void updateMqttMode() {
  switch (clockMode) {
    case 0:
      client.publish("stat/matrix/mode", "Clock", true);
      break;
    case 1:
      client.publish("stat/matrix/mode", "Countdown", true);
      break;
    case 2:
      client.publish("stat/matrix/mode", "Scoreboard", true);
      break;
    case 3:
      client.publish("stat/matrix/mode", "Text", true);
      break;
  }
}

void updateMQTTTime () {
  // v0.53 - sends MQTT request to get time from local MQTT system (or other automation source)
  // Receiving system should send MQTT command back with current date-time (see MQTT commands for format)
  client.publish("stat/matrix/clock/requesttime", "1", true);
}

void updateMqttBrightness(unsigned int bright_val) {
  char outBright[5];
  sprintf(outBright, "%3u", bright_val);
  client.publish("stat/matrix/brightness", outBright, true);
}

void updateMqttFont() {
  char outFont[5];
  sprintf(outFont, "%3u", numFont);
  client.publish("stat/matrix/font", outFont, true);
}

void updateMqttClockDisplay(unsigned int display_val) {
  char outDisplay[3];
  sprintf(outDisplay, "%3u", display_val);
  client.publish("stat/matrix/clock/display", outDisplay, true);
}

void updateMqttColor(unsigned int whichMode, unsigned int r_val, unsigned int g_val, unsigned int b_val) {
  // whichmode: 0 Clock, 1 Countdown, 2 CountdownPaused, 3 CountdownFinalMin, 4 Temp, 5 ScoreboardLeft, 6 ScoreboardRight, 7 TextTop, 8 TextBottom
  char outRed[5];
  char outGreen[5];
  char outBlue[5];
  sprintf(outRed, "%3u", r_val);
  sprintf(outGreen, "%3u", g_val);
  sprintf(outBlue, "%3u", b_val);
  switch (whichMode) {
    case 0:   //clock
      client.publish("stat/matrix/clock/color/red", outRed, true);
      client.publish("stat/matrix/clock/color/green", outGreen, true);
      client.publish("stat/matrix/clock/color/blue", outBlue, true);
      break;
    case 1:   //countdown
      client.publish("stat/matrix/countdown/color/red", outRed, true);
      client.publish("stat/matrix/countdown/color/green", outGreen, true);
      client.publish("stat/matrix/countdown/color/blue", outBlue, true);
      break;
    case 2:   //countdownPaused
      client.publish("stat/matrix/countdown/colorpaused/red", outRed, true);
      client.publish("stat/matrix/countdown/colorpaused/green", outGreen, true);
      client.publish("stat/matrix/countdown/colorpaused/blue", outBlue, true);
      break;
    case 3:   //countdownFinalMin
      client.publish("stat/matrix/countdown/colorfinalmin/red", outRed, true);
      client.publish("stat/matrix/countdown/colorfinalmin/green", outGreen, true);
      client.publish("stat/matrix/countdown/colorfinalmin/blue", outBlue, true);
      break;
    case 4:   //temp
      client.publish("stat/matrix/temperature/color/red", outRed, true);
      client.publish("stat/matrix/temperature/color/green", outGreen, true);
      client.publish("stat/matrix/temperature/color/blue", outBlue, true);
      break;
    case 5:   //scoreboard left
      client.publish("stat/matrix/scoreboard/colorleft/red", outRed, true);
      client.publish("stat/matrix/scoreboard/colorleft/green", outGreen, true);
      client.publish("stat/matrix/scoreboard/colorleft/blue", outBlue, true);
      break;
    case 6:   //scoreboard right
      client.publish("stat/matrix/scoreboard/colorright/red", outRed, true);
      client.publish("stat/matrix/scoreboard/colorright/green", outGreen, true);
      client.publish("stat/matrix/scoreboard/colorright/blue", outBlue, true);
      break;
    case 7:  //text top
      client.publish("stat/matrix/text/colortop/red", outRed, true);
      client.publish("stat/matrix/text/colortop/green", outGreen, true);
      client.publish("stat/matrix/text/colortop/blue", outBlue, true);
      break;
    case 8:  //text bottom
      client.publish("stat/matrix/text/colorbottom/red", outRed, true);
      client.publish("stat/matrix/text/colorbottom/green", outGreen, true);
      client.publish("stat/matrix/text/colorbottom/blue", outBlue, true);
      break;
    
  }
}

void updateMqttCountdownStartTime(unsigned long initMillis) {
  //  Break into hh, mm and ss and post as hh:mm:ss
  unsigned long hours   = ((initMillis / 1000) / 60) / 60;
  unsigned long minutes = (initMillis / 1000) / 60;
  unsigned long seconds = initMillis / 1000;
  int remSeconds = seconds - (minutes * 60);
  int remMinutes = minutes - (hours * 60); 
  byte h1 = hours / 10;
  byte h2 = hours % 10;
  byte m1 = remMinutes / 10;
  byte m2 = remMinutes % 10;  
  byte s1 = remSeconds / 10;
  byte s2 = remSeconds % 10;
  char outTime[10];
  sprintf(outTime, "%1u%1u:%1u%1u:%1u%1u", h1, h2, m1, m2, s1, s2);
  client.publish("stat/matrix/countdown/starttime", outTime, true); 
}
void updateMqttCountdownStatus(bool curStatus) {
  if (curStatus) {
    client.publish("stat/matrix/countdown/status", "Running", true);
   } else {
    client.publish("stat/matrix/countdown/status", "Stopped", true);
  }
}

void updateMqttCountdownAction(byte action) {
  // This is needed for LED clock sync
  char outAction[3];
  sprintf(outAction, "%3u", action);
  client.publish("stat/matrix/countdown/action", outAction, true);
  
}

void updateMqttTempSymbol(unsigned int symbolVal) {
  switch(symbolVal) {
    case 12:
      client.publish("stat/matrix/temperature/symbol", "C", true);
      break;
    case 13:
      client.publish("stat/matrix/temperature/symbol", "F", true);
      break;
    default:
      client.publish("stat/matrix/temperature/symbol", "?", true);
      break;
  }
}

void updateMqttTempCorrection() {
  char outCorr[10];
  sprintf(outCorr, "%4.1f", temperatureCorrection);
  client.publish("stat/matrix/temperature/correction", outCorr, true);
}

void updateMqttTemperature() {
  float ctemp = 0;
  if (temperatureSource == 0) {
    RtcTemperature temp = Rtc.GetTemperature();
    float ftemp = temp.AsFloatDegC();

    if (temperatureSymbol == 13) {
      ftemp = (ftemp * 1.8000) + 32;   
    }
    ctemp = ftemp + temperatureCorrection;
  } else if (temperatureSource == 1) {
    ctemp = (externalTemperature * 1.0);
  }
  char outTemp[10];
  sprintf(outTemp, "%4.1f", ctemp);
  client.publish("stat/matrix/temperature", outTemp, true);
}

void updateMqttScoreboardScores() {
  char outLeft[5];
  char outRight[5];
  byte leftLen = scoreboardTeamLeft.length() + 1;
  byte rightLen = scoreboardTeamRight.length() + 1;
  char outTeamLeft[leftLen];
  char outTeamRight[rightLen];
  scoreboardTeamLeft.toCharArray(outTeamLeft, leftLen);
  scoreboardTeamRight.toCharArray(outTeamRight, rightLen);
  sprintf(outLeft, "%3u", scoreboardLeft);
  sprintf(outRight, "%3u", scoreboardRight);
  client.publish("stat/matrix/scoreboard/scoreleft", outLeft, true);
  client.publish("stat/matrix/scoreboard/scoreright", outRight, true);
  client.publish("stat/matrix/scoreboard/teamleft", outTeamLeft, true);
  client.publish("stat/matrix/scoreboard/teamright", outTeamRight, true);
}

void updateMqttTextMessage() {
  byte msgLen = textFull.length() + 1;
  char outMsg[msgLen];
  char outEffect[4];
  char outSpeed[4];
  textFull.toCharArray(outMsg, msgLen);
  sprintf(outEffect, "%3u", textEffect);
  sprintf(outSpeed, "%3u", textEffectSpeed);
  client.publish("stat/matrix/text/message", outMsg, true);
  client.publish("stat/matrix/text/effect", outEffect, true);
  client.publish("stat/matrix/text/speed", outSpeed, true);
  
}
// **********************************
