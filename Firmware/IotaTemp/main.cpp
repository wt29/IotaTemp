#include "IotaTemp.h"
/***********************************************************************************
    IotaTemp - stolen (with full attribution) from Bob Lemaire's fabulous IotaTemp.
    all I'm trying to achieve is an extendable temperature/humidity and possibly wind speed (for bush fires) system
    that will log to an SD Card and upload to EmonCMS when WiFi is unavailable
    
    Goto to https://iotawatt.com  for all the details including the Git links to Bob's project 
    
    IotaWatt Electric Power Monitor System
    Copyright (C) <2017>  <Bob Lemaire, IoTaWatt, Inc.>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.  
   
***********************************************************************************/
/*********************************** Change log ****************************************************
 *  
 *   IotaTemp Changes
 *   16/03/19     00.00.01    Started to change references from IotaWatt to IotaTemp - makes me less confused and 
 *                            somewhat differenciates the project
 * 
 *****************************************************************************************************/

// Define instances of major classes to be used

WiFiClient WifiClient;
WiFiManager wifiManager;
DNSServer dnsServer;    
IotaLog currLog(5,400);                     // current data log  (1.1 years) 
IotaLog histLog(60,4000);                   // history data log  (11 years)  
RTC_DS1307 rtc;                             // Instance of RTC_PCF8523
Ticker ticker;
messageLog msglog;
DHT12 dht12;
Adafruit_ST7735 tft = Adafruit_ST7735( D4, D3, -1);

// Define filename Strings of system files.          
char* deviceName;             
const char* IotaLogFile = "/IotaWatt/IotaLog";
const char* historyLogFile = "/IotaWatt/histLog";
const char* IotaMsgLog = "/IotaWatt/IotaMsgs.txt";
const char* ntpServerName = "pool.ntp.org";
                       
/*
Not required
uint8_t ADC_selectPin[2] = {pin_CS_ADC0,    // indexable reference for ADC select pins
                            pin_CS_ADC1};  
                            
*/

// Trace context and work area

traceUnion traceEntry;

/**************************************************************************************************
* Core dispatching parameters - There's a lot going on, but the steady rhythm is sampling the
* power channels, and that has to be done on their schedule - the AC frequency.  During sampling,
* the time (in ms) of the last zero crossing is saved here.  Upon return to "Loop", the estimated
* time just before the next crossing is computed.  That's when samplePower will be called again.
* We try to run everything else during the half-wave intervals between power sampling.  The next 
* channel to be sampled is also kept here to complete the picture.  
**************************************************************************************************/
       
uint32_t lastCrossMs = 0;             // Timestamp at last zero crossing (ms) (set in samplePower)
uint32_t nextCrossMs = 0;             // Time just before next zero crossing (ms) (computed in Loop)

// Various queues and lists of resources.

serviceBlock* serviceQueue;           // Head of active services list in order of dispatch time.       
IotaInputChannel* *inputChannel;      // -->s to incidences of input channels (maxInputs entries) 
uint8_t maxInputs = 0;                        // channel limit based on configured hardware (set in Config)      
ScriptSet* outputs;                   // -> scriptSet for output channels

uint16_t  deviceVersion = 0;
float     VrefVolts = 2.5;            // Temperature reference shunt value used to calibrate

// ****************************************************************************
// statService maintains current averages of the channel values
// so that current values can be displayed by web clients
// statService runs at low frequency but is reved up by the web server 
// handlers if the statistics are used.

float   frequency = 55;                  // Split the difference to start
float   samplesPerCycle = 550;           // Here as well
float   cycleSampleRate = 0;
int16_t cycleSamples = 0;
float    heapMs = 0;                      // heap size * milliseconds for weighted average heap
uint32_t heapMsPeriod = 0;                // total ms measured above.
IotaLogRecord statRecord;                 // Maintained by statService with real-time values

// ****************************** SDWebServer stuff ****************************

#define DBG_OUTPUT_PORT Serial
ESP8266WebServer server(80);
bool    hasSD = false;
File    uploadFile;
SHA256* uploadSHA;
boolean serverAvailable = true;   // Set false when asynchronous handler active to avoid new requests
boolean wifiConnected = false;
uint8_t configSHA256[32];         // Hash of config file last time read or written

uint8_t*          adminH1 = nullptr;      // H1 digest md5("admin":"admin":password) 
uint8_t*          userH1 = nullptr;       // H1 digest md5("user":"user":password)
authSession*      authSessions = nullptr; // authSessions list head;
uint16_t          authTimeout = 600;      // Timeout interval of authSession in seconds;   
 
// ************************** HTTP concurrent request semaphore *************************

int16_t  HTTPrequestMax = 2;      // Maximum number of concurrent HTTP requests        
int16_t  HTTPrequestFree = 2;     // Request semaphore 

// ****************************** Timing and time data **********************************

int      localTimeDiff = 0;                  // Hours from UTC 
uint32_t programStartTime = 0;               // Time program started (UnixTime)
uint32_t timeRefNTP = SEVENTY_YEAR_SECONDS;  // Last time from NTP server (NTPtime)
uint32_t timeRefMs = 0;                      // Internal MS clock corresponding to timeRefNTP
uint32_t timeSynchInterval = 3600;           // Interval (sec) to roll NTP forward and try to refresh
uint32_t EmonCMSInterval = 10;               // Interval (sec) to invoke EmonCMS
uint32_t influxDBInterval = 10;              // Interval (sec) to invoke inflexDB 
uint32_t statServiceInterval = 1;            // Interval (sec) to invoke statService
uint32_t updaterServiceInterval = 60*60;     // Interval (sec) to check for software updates 

bool     hasRTC = false;
bool     RTCrunning = false;

char     ledColor[12];                       // Pattern to display led, each char is 500ms color - R, G, Blank
uint8_t  ledCount;                           // Current index into cycle

// ****************************** Firmware update ****************************
      
/*  

Don't want it updating from Bob's site

const char* updateURL = "iotawatt.com";
const char* updatePath = "/firmware/iotaupdt.php";
char*    updateClass = nullptr;              // NONE, MAJOR, MINOR, BETA, ALPHA, TEST    
const uint8_t publicKey[32] PROGMEM = {
                        0x7b, 0x36, 0x2a, 0xc7, 0x74, 0x72, 0xdc, 0x54,
                        0xcc, 0x2c, 0xea, 0x2e, 0x88, 0x9c, 0xe0, 0xea,
                        0x3f, 0x20, 0x5a, 0x78, 0x22, 0x0c, 0xbc, 0x78,
                        0x2b, 0xe6, 0x28, 0x5a, 0x21, 0x9c, 0xb7, 0xf3
                        }; 
*/

const char hexcodes_P[] PROGMEM = "0123456789abcdef";
const char base64codes_P[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";  

// ************************ ADC sample pairs ************************************
 
int16_t   samples = 0;                              // Number of samples taken in last sampling
int16_t   Vsample [MAX_SAMPLES];                    // Temperature/current pairs during sampling
int16_t   Isample [MAX_SAMPLES];

uint32_t loopTime;
uint32_t timeNow;

float _temp ;
float _humidity ;