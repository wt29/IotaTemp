#include "IotaTemp.h"

String formatHex(uint32_t data);
void dropDead(void);
void dropDead(const char*);
void setLedCycle(const char* pattern);
void endLedCycle();
void ledBlink();
void setLedState();

void setup()
{
  //*************************************** Start Serial connection (if any)***************************
   
  Serial.begin(115200);
  delay(250);
  //Serial.println(F("\r\n\n\n** Restart **\r\n\n"));
  //Serial.println(F("Serial Initialized"));
  
  //*************************************** Start SPI *************************************************
  /*  
  No ADC Chip so no pins
  pinMode(pin_CS_ADC0,OUTPUT);                    // Make sure all the CS pins are HIGH
  digitalWrite(pin_CS_ADC0,HIGH);
  pinMode(pin_CS_ADC1,OUTPUT);
  digitalWrite(pin_CS_ADC1,HIGH);
  */
  // pinMode(pin_CS_SDcard,OUTPUT);
  // digitalWrite(pin_CS_SDcard,HIGH);
  
  pinMode(redLed,OUTPUT);
  digitalWrite(redLed,LOW);

  //*************************************** Initialize SPI *******************************************
  
  SPI.begin();
  SPI.beginTransaction(SPISettings(2000000,MSBFIRST,SPI_MODE0));
  Serial.println("\r\nSPI started.");
   
  //*************************************** Initialize the SD card ************************************

  if(!SD.begin(pin_CS_SDcard)) {
    log("SD initiatization failed. Retrying.");
    setLedCycle("G.R.R...");
    while(!SD.begin(pin_CS_SDcard, SPI_FULL_SPEED)){ 
      yield();
    }
    endLedCycle();
    digitalWrite(greenLed,HIGH); 
  }
  hasSD = true;
  log("SD initialized.");

  //*************************************** Check RTC   *****************************

  Wire.begin(pin_I2C_SDA, pin_I2C_SCL);
  rtc.begin();

  Wire.beginTransmission(PCF8523_ADDRESS);            // Read Control_3
  Wire.write((byte)2);
  Wire.endTransmission();
  Wire.requestFrom(PCF8523_ADDRESS, 1);
  uint8_t Control_3 = Wire.read();
  
//  if(rtc.initialized()){
  if(rtc.isrunning()){
    timeRefNTP = rtc.now().unixtime() + SEVENTY_YEAR_SECONDS;
    timeRefMs = millis();
    RTCrunning = true;
    log("Real Time Clock is running. Unix time %d ", UNIXtime());
    if((Control_3 & 0x08) != 0){
      log("Power failure detected.");
      Wire.beginTransmission(PCF8523_ADDRESS);            
      Wire.write((byte)PCF8523_CONTROL_3);
      Wire.write((byte)0);
      Wire.endTransmission();
    }
    SdFile::dateTimeCallback(dateTime);
  }
  else {
    log("Real Time Clock not initialized.");
  }
  programStartTime = UNIXtime();
  
  Wire.beginTransmission(PCF8523_ADDRESS);            // Set crystal load capacitance
  Wire.write((byte)0);
  Wire.write((byte)0x80);
  Wire.endTransmission();

  //**************************************** Display software version *********************************

  log("Version %s", IOTATEMP_VERSION);

  copyUpdate(String(IOTATEMP_VERSION));
  
  //**************************************** Display the trace ****************************************

  log("Reset reason: %s", ESP.getResetReason().c_str());
  logTrace();
  log("ESP8266 ChipID: %d",ESP.getChipId());

  // ****************************************** Flush the trace ************************************

  traceEntry.seq = 0;
  for(int i=0; i<32; i++) trace(0,0);

//************************************* Process Config file *****************************************
  if(!getConfig()) {
    log("Configuration failed");
    dropDead();
  }
  log("Local time zone: %d", localTimeDiff);
  log("device name: %s, version: %d", deviceName, deviceVersion);

//************************************* Load passwords *******************************************

authLoadPwds();  
lastCrossMs = millis() - 10000;
log("LastCrossMs %f",lastCrossMs);

//******************  Setup the TFT 
tft.initR(INITR_144GREENTAB);
  tft.setTextWrap(false); 		// Allow text to run off right edge
  tft.setRotation( 1 );			// Portrait mode
  tft.fillScreen(ST7735_BLACK);

//*************************************** Start the WiFi  connection *****************************
  
  WiFi.setAutoConnect(true);
  WiFi.hostname();
  WiFi.begin();
  tft.setTextSize(2);
  tft.setCursor(0,0);
  tft.setTextColor(ST7735_BLUE);
  tft.println( "Connecting" );
  uint32_t autoConnectTimeout = millis() + 3000UL;
  while(WiFi.status() != WL_CONNECTED){
    if(millis() > autoConnectTimeout){
      setLedCycle("R.G.G...");
      tft.print(".");    			// Show that it is trying
      wifiManager.setDebugOutput(false);
      wifiManager.setConfigPortalTimeout(180);
      String ssid = "iota" + String(ESP.getChipId());
      String pwd = deviceName;
      log("Connecting with WiFiManager.");
      wifiManager.autoConnect(ssid.c_str(), deviceName);
      endLedCycle();
      while(WiFi.status() != WL_CONNECTED && RTCrunning == false){
        log("RTC not running, waiting for WiFi.");
        setLedCycle("R.R.G...");
        wifiManager.setConfigPortalTimeout(3600);
        wifiManager.autoConnect(ssid.c_str(), pwd.c_str());
        endLedCycle();
      }
      break;
    }
    yield();
  }
  if(WiFi.status() != WL_CONNECTED){
    log("No WiFi connection.");
  }
    
  //*************************************** Start the local DNS service ****************************

  if (MDNS.begin(deviceName)) {
      MDNS.addService("http", "tcp", 80);
      log("MDNS responder started");
      log("You can now connect to http://%s.local", deviceName);
  }
   
 //*************************************** Start the web server ****************************

  SdFile::dateTimeCallback(dateTime);
  server.on(F("/edit"), HTTP_POST, returnOK, handleFileUpload);
  server.onNotFound(handleRequest);
  const char * headerkeys[] = {"X-configSHA256"};
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize );
  server.begin();
  log("HTTP server started");
  WiFi.mode(WIFI_STA);
  
 //*************************************** Start the logging services *********************************
   
  NewService(timeSync, T_timeSync);
  NewService(statService, T_stats);
  NewService(WiFiService, T_WiFi);
  
 // NewService(updater, T_UPDATE);   // No updates happening here
  NewService(dataLog, T_datalog);
  NewService(historyLog, T_history);
  log("Services started");
 
  loopTime = 30000;
  timeNow = millis() - loopTime + 1000 ;     // Should force it to poll and display on init
  
}  // setup()
/***************************************** End of Setup **********************************************/

void dropDead(void){dropDead("R.R.R...");}
void dropDead(const char* pattern){
  log("Program halted.");
  setLedCycle(pattern);
  while(1){
    delay(1000);   
  }  
}

void setLedCycle(const char* pattern){
  ledCount = 0;
  for(int i=0; i<13; i++){
    ledColor[i] = pattern[i];
    if(pattern[i] == 0) break;
  }
  ticker.attach(0.5, ledBlink);
}

void endLedCycle(){
  ticker.detach();
  setLedState();
}

void ledBlink(){
  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, LOW);
  if(ledColor[ledCount] == 0) ledCount = 0;
  if(ledColor[ledCount] == 'R')digitalWrite(redLed, HIGH);
  else if(ledColor[ledCount] == 'G')digitalWrite(greenLed, HIGH);
  ledCount++;
}

void setLedState(){
  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed, LOW);
  if( !RTCrunning || WiFi.status() != WL_CONNECTED){
    digitalWrite(redLed, HIGH);
  }
}

      



