#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 0        // NO interrrupt pin? set to 0
Adafruit_SSD1306 OLED(OLED_RESET);

#include "Adafruit_Si7021.h"
Adafruit_Si7021 sensor = Adafruit_Si7021();


#include <Ticker.h>         // Ticker Library
Ticker blinker;             // setable blinker
Ticker OLED_Update;         // sets when to update OLED display
bool   OLED_Update_Stat;
float  LED_TICKER_TIME = 10.0;

/// GLOBAL VARS
  char hostString[16] = {0}; // mDNS name, built part string part MAC ie; RT_AE2D

  // OLED return vars
  String IP_String = "";
  String OLED_line1 = "";
  String OLED_line2 = "";
  String OLED_line3 = "";
  int    OLED_count = 0;    // 0 ip, 1 hostname, 2 unit ID, will cycle on each OLED display update 

  String RemoteID = "loc";  // http://<your IP>/set and store 
  String WebText  = "F";    // web text passed
  String F_C_PREF = "F";    // will be updated from loadConfig
  String F_C_HOLD = "F";    // place holder to test for changes
  float  Humidity = 0;      // humidity
  float  TempC    = 0;      // temp in Celsius
  float  TempF    = 0;      // temp as Fahrenheit (isFahrenheit = true)

  float  hif = 0;           // Compute heat index in Fahrenheit (the default)
  float  hic = 0;           // Compute heat index in Celsius (isFahreheit = false)
  char   buff[10];          // passing var 
  int    UpCount = 0;

#define LED D6                          //  LED pin, NOTE some esp8266 boards have different headers and pin names may not be the same 
int  BLINK_STATUS      = 0;             // 0 NORMAL, 1 WiFi DOWN
int  BLINK_STATUS_LAST = BLINK_STATUS;  // place holder for change detection
int  LED_COUNT         = 0;
// settable staus blink rates
int  NORMAL_STATE   = 100;  // BLINK_STATUS 0, NORMAL,    .1 pulse every 10 sec / HeartBeat
int  WIFI_DWN_STATE = 10;   // BLINK_STATUS 1, WIFI DOWN, .1 pulse every 1.0
