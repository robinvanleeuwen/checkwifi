#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPIFFSIniFile.h>
#include "FS.h"

int getWiFiStatus(bool debug=false);
int getUsedStackSize(bool debug=false);
int getFreeHeapSize(bool debug=false);
String read_ini_file(char* key, bool debug=false);
String getWiFiIP(bool debug=false);
void wifi_connect(String ssid, String password, bool debug=false);

// The URL to perform the WiFi check on
const char* http_url = "http://www.zaaksysteem.nl/wifi/ping";

// Global variable to remeber stack position
char *stack_start;

/* read_ini_file read the ini to lookup a given key
 * ags: char *key:  The key to look for
 *      bool debug: Should there be debug logging
 *                  !! This possibly log sensitve information
 */                  
String read_ini_file(char* key, bool debug) {
  
  const size_t buffer_length = 80;
  char buffer[buffer_length];

  const char *ini_file = "/wifi.ini";

  if(!SPIFFS.begin()) {
    Serial.println("SPIFFS mount failed. Cannot continue.");
    while(true);
  }

  SPIFFSIniFile ini(ini_file);
  if(!ini.open()){
    Serial.println("Could not open .ini file. Cannot continue");
    while(true);
  }

  if(!ini.validate(buffer, buffer_length)){
    Serial.println("Not a valid .ini file. Cannot continue.");
    while(true);  
  }

  // Get the key from the ini file and copy it to heap as static char[].
  static char value[buffer_length]; 
  Serial.print("Getting value for "); Serial.println(key);
  if(ini.getValue("network", key, buffer, buffer_length)){
    strcpy(value, buffer);
  }

  // Will convert static char[] to String
  return value;  
}

void wifi_connect(String ssid, String password, bool debug) {
  
  if(debug) {
    Serial.println();
    Serial.println("Setting client mode...");
  }
  WiFi.mode(WIFI_STA);

  String ip_address = getWiFiIP(debug);

  if(debug) {
    Serial.print("Connecting to SSID: ");
    Serial.println(ssid);
    Serial.print("With password: ");
    Serial.println("***************");
  }

  while(ip_address == "(IP unset)") {
    
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("Connecting...");
    WiFi.begin(ssid, password);
    delay(15000);
   
    ip_address = getWiFiIP(debug);

  }
  Serial.println();
  getWiFiStatus(true);
  
}

void startup_blinks() {
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  for(int c=0; c<5; c++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
  }
  
}

int getUsedStackSize(bool debug) {
  char stack;
  int used_stack = stack_start - &stack;
  if(debug) {
    Serial.print(F("Used stack size "));
    Serial.println(used_stack);
  }
  return used_stack;
}

int getFreeHeapSize(bool debug) {
  int free_heap_size = ESP.getFreeHeap();
  if(debug) {
    Serial.print("Free heap size: ");
    Serial.println(free_heap_size);
  }
  return free_heap_size;
}


int getWiFiStatus(bool debug) {
  int wifi_status = WiFi.status();
  if(debug){
    Serial.print("WiFi.status == ");
    Serial.println(wifi_status);
  }
  return wifi_status;
}

String getWiFiIP(bool debug) {
  IPAddress ip = WiFi.localIP();
  if(debug) {
    Serial.print("IP address: ");
    Serial.println(ip);
  }
  return ip.toString();
}

void setup() {
  
  bool debug = true;
  
  // Get the address of the beginning of the stack to calculate used stack size
  char stack;
  stack_start = &stack;

  Serial.begin(115200);
  delay(500);

  Serial.println("--- This is the setup function ---");  
  startup_blinks();
  wifi_connect(read_ini_file("ssid"), read_ini_file("password"), debug);
  Serial.println("--- End of the setup function  ---");  
}

void loop() {
  bool debug = true;
  int minimal_heap_size = 4096;
  delay(5000);

  getWiFiIP(debug);
  
  if(getWiFiIP(false) == "(IP unset)") {
    Serial.println("No WiFi IP, reconnecting...");
    wifi_connect(read_ini_file("ssid"), read_ini_file("password"), debug);
  }
  
  if(getWiFiIP(false) != "(IP unset)") {
    HTTPClient http;
    http.begin(http_url);
    int http_code = http.GET();
    if (debug) {
      Serial.print("http.GET() return code = ");
      Serial.println(http_code);      
    }
    http.end();
  }  

  //  Yeahhh sure this is ugly, but it's better than
  //  having to unscrew the ESP8266 Board from the
  //  ceiling to restart it when HEAP runs low...
  //
  if (getFreeHeapSize(debug) < minimal_heap_size) {
    if (debug == true) {
      Serial.print("Heap Size < ");
      Serial.print(minimal_heap_size);
      Serial.println(", Restarting ESP8266 Board");
    }
    delay(5000);
    ESP.restart();
  }
  getUsedStackSize(debug);
}
