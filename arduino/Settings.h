// Setting.h version 0.53
// ===============================================================================
// Update these values to match your build and board type if not using D1 Mini
#define NUM_LEDS 400                           // Total of 400 LED's if matrix built as shown    
#define MILLI_AMPS 30000                       // Update to match max milliamp output of your power suppy (or about 500 milliamp lower than max to be safe)
#define COUNTDOWN_OUTPUT D5                   // Output pin to drive buzzer or other device upon countdown expiration
#define WIFIMODE 2                            // 0 = Only Soft Access Point, 1 = Only connect to local WiFi network with UN/PW, 2 = Both
#define MQTTMODE 1                            // 0 = Disable MQTT, 1 = Enable (will only be enabled if WiFi mode = 1 or 2 - broker must be on same network)
#define MQTTSERVER "192.168.1.108"            // IP Address (or url) of MQTT Broker. Use '0.0.0.0' if not enabling MQTT
#define MQTTPORT 1883                         // Port of MQTT Broker.  This is usually 1883
// Optional pushbuttons (colors listed match ones used in build instructions)
// All pins need to be LOW when button open or code modifications will be required in main sketch
#define DATA_PIN D6                           // Change this if you are using another type of ESP board than a WeMos D1 Mini
#define MODE_PIN D3                           // Push button: - not used -
#define V1_PIN D4                             // Push button: - not used -
#define V0_PIN 3                              // Push button (blue): Mode button (RX pin)
#define H1_PIN D7                             // Push button (green): Visitor+/Countdown start/stop button
#define H0_PIN 1                              // Push button (red): Home+/Countdown reset button (TX pin)
// Default state topic prefix: stat/matrix
// Default command topci prefix: cmnd/matrix
// ---------------------------------------------------------------------------------------------------------------
// Options - Defaults upon boot-up.  Most values can be updated via web app or via Home Assistant/MQTT after boot
// ---------------------------------------------------------------------------------------------------------------

byte clockMode = 0;                             // Default mode at boot: 0=Clock, 1=Countdown, 2=Temperature, 3=Scoreboard
byte brightness = 30;                          // Default starting brightness at boot. 255=max brightness based on milliamp rating of power supply
byte numFont = 1;                               // Default Large number font: 0-Original 7-segment (21 pixels), 1-modern (31 pixels), 2-hybrid (28 pixels)
byte temperatureSymbol = 13;                    // Default temp display: 12=Celcius, 13=Fahrenheit
byte temperatureSource = 1;                     // 0 Interior temp (from RTC module), 1 Exterior temp (provided via MQTT - MQTT use required/no conversion or correction applied)
float temperatureCorrection = -3.0;             // Temp from RTC module.  Generally runs "hot" due to heat from chip.  Adjust as needed. Does not apply if external source selected.
unsigned long temperatureUpdatePeriod = 60000; // How often, in milliseconds to update MQTT time. Recommend minimum of 60000 (one minute) or greater. Set to 0 to disable updates.
byte hourFormat = 12;                           // Change this to 24 if you want default 24 hours format instead of 12     
byte scoreboardLeft = 0;                        // Starting "Visitor" (left) score on scoreboard
byte scoreboardRight = 0;                       // Starting "Home" (right) score on scoreboard
String scoreboardTeamLeft = "&V&";             // Default visitor (left) team name on scoreboard
String scoreboardTeamRight = "&H&";            // Default home (right) team name on scoreboard
byte textEffect = 0;                           // Text effect to supply (0 = none, see documentation for others)
byte textEffectSpeed = 5;                      // 1 = slow (1 sec) to max of 10 (.1 second)
bool autoSetTimeOnBoot = true;                 // v0.53: Make MQTT call to request current time on boot - call must be handled via automation system (e.g. Home Assistant, NodeRed, etc.)

// Default starting colors for modes
// Named colors can be used.  Valid values found at: http://fastled.io/docs/3.1/struct_c_r_g_b.html
// Alternatively, you can specify each of the following as rgb values.  Example: CRGB clockColor = CRBG(0,0,255);

CRGB clockColor = CRGB:: Blue;
CRGB countdownColor = CRGB::Green;
CRGB countdownColorPaused = CRGB::Orange;     // If different from countdownColor, countdown color will change to this when paused/stopped.
CRGB countdownColorFinalMin = CRGB::Red;      // If different from countdownColor, countdown color will change to this for final 60 seconds.
CRGB temperatureColor = CRGB::Green;
CRGB scoreboardColorLeft = CRGB::Red;
CRGB scoreboardColorRight = CRGB::Green;
CRGB textColorTop = CRGB::Red;
CRGB textColorBottom = CRGB::Green;
CRGB alternateColor = CRGB::Black;            // Recommend to leave as Black. Otherwise unused pixels will be lit in digits
