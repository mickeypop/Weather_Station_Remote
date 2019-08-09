#include "defines.h"           // many defined vars here
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "FS.h"

// SET THESE 2
const char *ssid     = "mickeysplace";    // YOUR AP
const char *password = "Jesus loves me";  // YOUR AP PASSWORD/PHRASE

ESP8266WebServer server ( 80 ); 


void setup() 
{
  Serial.begin ( 115200 );

  initDisplay();  // Initialize OLED Display 


  initSPIFFS();   // Store
  openConfig();   // load vars
   
  initWiFi();     // connect to wifi
  initMDNS();     // setup mDNS,  RT_<MAC>.local, only last 4 char of MAC address
  initWEB();      // WEB server setup

  bootDisplay();  // show IP and RT_<MAC>.local
  delay(5000);
  pinMode(LED,OUTPUT);

  //Initialize Ticker every 0.1s
  blinker.attach(0.1, LedChangeState);                    // Use attach_ms if you need time in ms
  OLED_Update.attach( LED_TICKER_TIME, OLED_Update_ISR ); // SECONDS, time between OLED updates 

  initSensor();  // setup Si7021
} // END setup() 


void loop() 
{
 if (WiFi.status() ==  WL_CONNECTED)   // WiFi UP 
  {
     if( BLINK_STATUS_LAST == 1 )  // change flash rate ?
     {
        BLINK_STATUS = 0;
        BLINK_STATUS_LAST = BLINK_STATUS;
        LED_COUNT = 0;
     }
    
    if ( OLED_Update_Stat) // update on tick
    {
      buildDisplay();
    }

    server.handleClient();
    delay(1000);           // wait 1s between readings
  } else
  {
    // WiFi DOWN
    if( BLINK_STATUS_LAST == 0 )  // change flash rate
    {
      BLINK_STATUS = 1;
      BLINK_STATUS_LAST = BLINK_STATUS;
      LED_COUNT = 0;
    }
    OLED.clearDisplay();
    OLED.setCursor(5, 12);
    OLED.print("WiFi DOWN");
    OLED.display();       //output 'display buffer' to screen 

    WiFi.begin( ssid , password );
    int WLcount = 0;

   // try till reconnected or drop out at 200, usually < 30 once AP is up
    while (WiFi.status() != WL_CONNECTED && WLcount < 200 )
    {
      delay( 100 );
      Serial.printf(".");
         if (UpCount >= 50)  // just keep terminal from scrolling sideways
         {
            UpCount = 0;
            Serial.printf("\n");
         }
         ++UpCount;
      ++WLcount;
    }    
  }
}  // END loop()

//// Service Functions by type

/// Sensor
void initSensor()
{
  if (!sensor.begin()) 
  {
    Serial.println("Did not find Si7021 sensor!");
    while (true)
      ;
  }
  Serial.print("Found model ");
  switch( sensor.getModel() ) 
  {
    case SI_Engineering_Samples:
      Serial.print("SI engineering samples"); break;
    case SI_7013:
      Serial.print("Si7013"); break;
    case SI_7020:
      Serial.print("Si7020"); break;
    case SI_7021:
      Serial.print("Si7021"); break;
    case SI_UNKNOWN:
    default:
      Serial.print("Unknown");
  }
  Serial.print(" Rev(");
  Serial.print(sensor.getRevision());
  Serial.print(")");
  Serial.print(" Serial #"); Serial.print(sensor.sernum_a, HEX); Serial.println(sensor.sernum_b, HEX);
  
} // END initSensor()


//boolean isFahrenheit: True == Fahrenheit; False == Celcius
float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit) 
{ // both Rothfusz and Steadman's equations
  // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
  float hi;

  if (!isFahrenheit)      temperature = convertC_to_F(temperature);

  hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

  if (hi > 79) 
  {
    hi = -42.379 +
             2.04901523 * temperature +
            10.14333127 * percentHumidity +
            -0.22475541 * temperature*percentHumidity +
            -0.00683783 * pow(temperature, 2) +
            -0.05481717 * pow(percentHumidity, 2) +
             0.00122874 * pow(temperature, 2) * percentHumidity +
             0.00085282 * temperature*pow(percentHumidity, 2) +
            -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

    else if((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  return isFahrenheit ? hi : convertF_to_C(hi);
}


float convertC_to_F(float c) {
  return c * 1.8 + 32;
}


float convertF_to_C(float f) {
  return (f - 32) * 0.55555;
}
//// END sensor functions


/// Storage ///
void initSPIFFS()
{
  SPIFFS.begin();
  
    // test if format SPIFFS if needed
  if (!SPIFFS.exists("/formatComplete.txt")) 
  {
    Serial.println("\n\nPlease wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();  // comment out for safty once reading files?
    
    File f = SPIFFS.open("/formatComplete.txt", "w");   
    if (!f) 
    {
        Serial.println("file open failed");
    } else 
    {
        f.println("Format Complete");
        f.close();  // save and close file
    }
  } else 
  {
      Serial.println("\n\nSPIFFS is Already formatted...");
  }
} // END initSPIFFS(

void openConfig()
{
  if (!SPIFFS.exists("/config.txt")) 
  {
    Serial.println("Building Missing Config file");
    
    File f = SPIFFS.open("/config.txt", "w");
    if (!f) 
    {
        Serial.println("file open failed");
    } else 
    {
        String tmp = "F,";
        tmp += RemoteID;
        f.println( tmp );  // set default till web set changed
        f.close();
    }
  } else 
  {
    Serial.println("Opening Config...");

    if (!loadConfig()) 
    {
        Serial.println("Failed to load config");
    } else 
    {
        Serial.print("\tConfig loaded: \n\t");
        Serial.printf( "F_C_PREF = %s\n\t" , F_C_PREF.c_str());
        Serial.printf( "RemoteID = %s\n" , RemoteID.c_str());
    }
  }
}  // END openConfig()

void saveWebSettings()
{
    Serial.println("Saving Config...");

    if (!saveConfig() ) 
    {
        Serial.println("Failed to save config");
    } else 
    {
        Serial.print("Unit Config Saved: ");
        Serial.printf( "F_C_PREF = %s\n" , F_C_PREF.c_str());    
    }
}

bool loadConfig() 
{
  String openString = "";
  File configFile = SPIFFS.open("/config.txt", "r");
  if (!configFile) {    // did it open??
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }
    openString = configFile.readStringUntil('\n');  // Read string from file
    openString.trim();  // get rid of \r\n
    F_C_PREF = getStringValue( openString, ',', 0); // if  a,4,D,r  would return a
    F_C_HOLD = F_C_PREF;

    RemoteID = getStringValue( openString, ',', 1); // if  c,Bedrm,D,r  would return Bedrm

  configFile.close();
  return true;
}  // END loadConfig()



bool saveConfig() 
{
  File configFile = SPIFFS.open("/config.txt", "w");
  if (!configFile) 
  {
    Serial.println("Failed opening config for writing");
    return false;
  } else
  {
   // stored <temp>,<remoteID>
   String store = F_C_PREF;
   store += ",";
   store += RemoteID;
    
    configFile.println( store );
    configFile.close();
    return true;
  }
}  // END saveConfig()
/// END Storage ///


/// OLED DISPLAY STUFF ///
void initDisplay()
{
  OLED.begin();
  OLED.clearDisplay();
  OLED.setTextWrap(false);
  OLED.setTextSize(1);
  OLED.setTextColor(WHITE);
} // END initDisplay()

void bootDisplay()  // only at boot
{
 // build the output
  OLED_line1 = "IP ";       // clear lines
  OLED_line1 += WiFi.localIP().toString();
  OLED_line2 += "mDNS ";
  OLED_line2 += hostString;
  OLED_line2 += ".local";
  OLED_line3 += "Remote ID ";
  OLED_line3 += RemoteID;

  // write to OLED buffer
  OLED.clearDisplay();
  OLED.setCursor(1, 1);
  OLED.print( OLED_line1 );
  OLED.setCursor(1, 12 );
  OLED.print( OLED_line2 );
  OLED.setCursor(1, 24 );
  OLED.print( OLED_line3 );    
  OLED.display();             //output 'display buffer' to OLED 
} // END bootDisplay()

void buildDisplay()
{
  Humidity = sensor.readHumidity();                          // Read humidity
  TempC     = sensor.readTemperature();                       // Read temperature in degree Celsius
  TempF = convertC_to_F( TempC );
  hif      = computeHeatIndex(TempF, Humidity, true  ); // Compute heat index in Fahrenheit (the default)
  hic      = computeHeatIndex(TempC, Humidity, false ); // Compute heat index in Celsius (isFahreheit = false)
  buff[6];                                              // place holder 

  OLED_Update_Stat = false;  // don't run again till next tick
   
  if(isnan(Humidity) || isnan(TempC))   // Check if any reads failed and exit early (to try again)
  {
    OLED.clearDisplay();
    OLED.setCursor(10, 12);
    OLED.print("ERROR");
    OLED.display(); //output 'display buffer' to screen 
    return;
  }
  // serial out
  Serial.print( "\nTemperature: " );
    if( F_C_PREF == "C")
    {
      Serial.print(TempC);
      Serial.print( "°C\t Heat index: ");
      Serial.print(hic);
      Serial.print( "°C\n\t");
    } else
    {
      Serial.print(TempF);
      Serial.print( "°F\t Heat index: ");
      Serial.print(hif);
      Serial.print( "°F\n\t" );
    }
               
  Serial.print( "Humidity: " );
  Serial.print(Humidity);
  Serial.print( " %\n" );

  // build the output
  OLED_line1 = ""; // clear lines
  OLED_line2 = "";
  OLED_line3 = "";


  // line 1
  if( OLED_count == 0 )
  {
      OLED_line1 += "IP ";
      OLED_line1 += WiFi.localIP().toString();
  }
  if( OLED_count == 1 )
  {
      OLED_line1 += "mDNS ";
      OLED_line1 += hostString;
      OLED_line1 += ".local";
  }
  if( OLED_count == 2 )
  {
      OLED_line1 += "Remote ID ";
      OLED_line1 += RemoteID;
  }

  // LINE 2
  OLED_line2 += "T:";

    if( F_C_PREF == "C" )  
    {
      dtostrf(TempC, 3, 1, buff);  // Celcius, 3 is mininum width, 6 is precision/ load to buff
      OLED_line2 += buff;
      OLED_line2 += " C  H:";
    } else
    {
      dtostrf(TempF, 3, 1, buff); // Fahrenheit, 3 is mininum width, 6 is precision/ load to buff
      OLED_line2 += buff;
      OLED_line2 += " F  H:";
    }
  dtostrf(Humidity, 2, 0, buff);  // 3 is mininum width, 6 is precision/ load to buff
  OLED_line2 += buff;
  OLED_line2 += " %";

  // LINE 3
  OLED_line3 = "TempIndex: ";
    if( F_C_PREF == "C" )
    {
      dtostrf( hic, 3, 1, buff);  // Celcius, 3 is mininum width, 1 is precision/ load to buff
      OLED_line3 += buff;
      OLED_line3 += " C";
    }else
    {
      dtostrf( hif, 3, 1, buff); // Fahrenheit, 3 is mininum width, 1 is precision/ load to buff
      OLED_line3 += buff;
      OLED_line3 += " F";
    }
  // write to OLED buffer
  OLED.clearDisplay();
  OLED.setCursor(1, 1);
  OLED.print( OLED_line1 );
  OLED.setCursor(1, 12 );
  OLED.print( OLED_line2 );
  OLED.setCursor(1, 24 );
  OLED.print( OLED_line3 );    
  OLED.display();             //output 'display buffer' to OLED 
} // END buildDisplay()


void LedChangeState()       // LED ISR
{
  if( BLINK_STATUS == 0)    // NORMAL
  {
    if( LED_COUNT == 0 )    
    {
      digitalWrite(LED, HIGH );  // ON
    } else
    {
      digitalWrite(LED, LOW );  // OFF
    }
    LED_COUNT++;
    if( LED_COUNT >= NORMAL_STATE )
    {
      LED_COUNT = 0;
    }
  }

  if( BLINK_STATUS == 1)          // WIFI DOWN
  {
    if( LED_COUNT == 0 )    
    {
      digitalWrite(LED, HIGH );  // ON
    } else
    {
      digitalWrite(LED, LOW );  // OFF
    }
    LED_COUNT++;
    if( LED_COUNT >= WIFI_DWN_STATE )
    {
      LED_COUNT = 0;
    }
  }
} // END LedChangeState()


void OLED_Update_ISR()
{
  OLED_Update_Stat = true;
  OLED_count++;
  if( OLED_count > 2 )  OLED_count = 0;
} // END OLED_Update_ISR()

/// END OLED DISPLAY STUFF ///


/// WIFI STUFF ///
void initWiFi()
{
  Serial.print("\nStarting WiFi..");
  WiFi.begin ( ssid, password );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) 
  {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "\tIP address: " );
  Serial.println ( WiFi.localIP() );
  Serial.println("");
  IP_String = WiFi.localIP().toString();  
}  // END initWiFi()


void  initMDNS()
{
  String MACString = GetMyMacAddress();
  sprintf( hostString, "RT_%s", MACString.c_str() );  // RT_BEBC.local, last 4 char of MAC
  WiFi.hostname( hostString );
  if ( MDNS.begin( hostString )) {
    Serial.println( "mDNS responder started" );
    Serial.print( "\tHostname: " );
    Serial.print( hostString ); 
    Serial.println( ".local\n" );
  }
  MDNS.addService("http", "tcp", 80); // Announce esp tcp service on port 80
}  // END initMDNS()


// gets MAC, returns String with ":"s removed
// ie:   String var = GetMyMacAddress();
String GetMyMacAddress(){
  uint8_t mac[6];
  char macStr[18] = {0};
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X%02X", mac[4], mac[5]); // only 4 char
  //sprintf(macStr, "%02X%02X%02X", mac[3], mac[4], mac[5]); // only 6 char
  //sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0],  mac[1], mac[2], mac[3], mac[4], mac[5]); // no :'s 12 char
  return  String(macStr);
} // END GetMyMacAddress()

/// END WIFI STUFF ///


/// WEB STUFF ///
void  initWEB()
{
  server.on( "/", handleRoot );
  server.on( "/set", handleSetting );
  server.onNotFound( handleNotFound );
  server.begin();
  Serial.println( "HTTP server started" );
} // END initWEB()


void handleRoot() 
{
  WebText = "";
  WebText = server.arg( "temp" );
  WebText.toUpperCase();

  server.send ( 200, "text/html", getPage() );
}  // END handleRoot()


// call remote as http://192.168.1.120/set?temp=c&id=bedRm
// if http://192.168.1.120/set  you get a web page
void handleSetting() 
{  // http://192.168.1.120/set?temp=c&id=bedRm  for Celcius 
      // TEMP = "F" or "C" NO QUOTES, upper or lower case
      // ID     will store 1st char uppercase all rest lower
    bool changed = false;
   WebText      = "";
   WebText      = server.arg( "temp" );
   String webID = server.arg( "id" );
   WebText.trim();
   WebText.toUpperCase();
   webID.trim();
   webID.toLowerCase();
    webID[0] = toUpperCase( webID[0]);  // "bedRm" becomes "Bedrm"

   if( WebText == "C" || WebText == "F"  )
   {
       F_C_PREF = WebText;

      if( F_C_PREF != F_C_HOLD )  // did stored setting change ?
      {
        changed = true;
        F_C_HOLD = F_C_PREF;
      }
   }

   if( webID != RemoteID )  // did it change?
   {
     changed = true;
     RemoteID = webID;
   }

  if( changed )
  {
     saveWebSettings();       // save to SPIFFS here
     Serial.println("\nSaving Config...");

     if (!saveConfig() ) 
     {
        Serial.println("Failed to save config");
     } else 
     {
        F_C_HOLD = F_C_PREF;
        Serial.print("Unit Config Saved: ");
        Serial.printf( "F_C_PREF = %s\n" , F_C_PREF.c_str());    
     }
  }
   server.send ( 200, "text/html",  settingsPage() );
   // server.send ( 200, "text/html",  F_C_PREF );   // return F_C_PREF , DEBUGGING ONLY comment out when done
}  // END handleSetting()


void handleNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
} // END handleNotFound()


// builds the web page
// RETURNS  <remoteID >,<C/F>,<TEMP>,<HeatIndex>,<Humidity>
String getPage()
{
  String page = RemoteID;
  page += ",";
  page += F_C_PREF;
  page += ",";

  // <temp>
  if( F_C_PREF == "C")
  {
     dtostrf(TempC, 3, 1, buff); // Fahrenheit, 3 is mininum width, 6 is precision/ load to buff
     page += buff;
     page += ",";

    // <tempIndex>
     dtostrf( hic, 3, 1, buff); // Fahrenheit, 3 is mininum width, 1 is precision/ load to buff
     page += buff;
     page += ",";
  } else
  {
     dtostrf(TempF, 3, 1, buff); // Fahrenheit, 3 is mininum width, 1 is precision/ load to buff
     page += buff;
     page += ",";

     // <tempIndex>
     dtostrf( hif, 3, 1, buff); // Fahrenheit, 3 is mininum width, 1 is precision/ load to buff
     page += buff;
     page += ",";
  }

   // Humidity, Format output
     dtostrf( Humidity, 3, 1, buff); // Fahrenheit, 3 is mininum width, 1 is precision/ load to buff
     page += buff;

  return page;
} // END getPage()


String settingsPage()
{
  String page = "<html>\n<head><meta name=\"viewport\" content=\"initial-scale=1,maximum-scale=1,user-scalable=no\">\n";
  page += "<title>ESP Remote Temp/Humidity Sensor</title>\n";
  page += "\n<style>\nhtml,body{font-size:13px;padding:3px;margin:0;width:100%;line-height:2em;box-sizing:border-box;}\nsection{width:350px;margin:0 auto;}\nfieldset>legend{font-weight:bolder;}\nlabel>span,div>span{display:inline-block;margin-right:2px;}\ndiv.h{margin-left:22px;}\ndiv>span{width:52px;text-align:center;}\nlabel>span{width:50px;height:25px;border:1px solid black;vertical-align:bottom;}\n#l a{margin-left:5px;}table{font-size:13px;}\n#id,#d,#V{width:60px;}\n.tab{margin-left: 2.5em}\n</style>\n";
  page += "\n</head>\n<body>\n";
  page += "<section>\n";
  page += "<fieldset><legend>Settings Page: </legend>\n";
  
  page += "<form action='/set' method='POST'>\n";
  page += "Network Name: <b>";
  page += hostString;

   page += ".local</b>\n<br><br>\nSet units and ID name you prefer to show on main display<br>\nThey will save as master settings.\n<br><br>\n ";

   page += "<b>Temperature Units:</b> \n";
   page += " <input type=\"radio\" name=\"temp\" value=\"C\" ";
   if( F_C_PREF == "C" )
   {
      page += "checked=\"checked\"  ";
   }
   page += "> Celsius \n";
   page += " <input type=\"radio\" name=\"temp\" value=\"F\" ";
   if( F_C_PREF == "F" )
   {
      page += "checked=\"checked\"  ";
   }
   page += ">  Fahrenheit\n<br><br>\n ";
   page += "<b>Remote ID Name:</b> ";
   page += "<input type=\"text\" id=\"id\" name=\"id\" value=\"";
      page +=   RemoteID;
   page += "\" >\n<br>\n";
   page += "ID should stay between 4 and 6 characters to fit on the Main Station LCD \n<br><br>";

  //// SUBMIT button ////
  page += "<input type=\"submit\" value=\"Submit\">\n"; 
  page += "</form>\n</fieldset>\n</section>\n</body>\n</html>";

  return page;
} // END settingsPage()

/// END WEB STUFF ///

/// MISC STUFF ///

// REF var = getStringValue( StringVar, ',', 2); // if  a,4,D,r  would return D
String getStringValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length();

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}  // END getValue()
