//Reference: https://www.arduino.cc/en/Reference/HomePage
//See: http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
//Librairies et cartes ESP8266 sur IDE Arduino: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//http://arduino-esp8266.readthedocs.io/en/latest/
//JSon lib: see https://github.com/bblanchon/ArduinoJson.git
//peychart@netcourrier.com 20171021
// Licence: GNU v3
#include "common.h"

//Avoid to change the following:
String                            hostname(DEFAULTHOSTNAME);    //Can be change by interface
String                            ssid[SSIDCount()];            //Identifiants WiFi /Wifi idents
String                            password[SSIDCount()];        //Mots de passe WiFi /Wifi passwords

bool                              WiFiAP(false);
bool                              mustResto(false);
ulong                             next_reconnect(0UL);
#ifdef DEFAULTWIFIPASS
  static ushort                   nbWifiAttempts(MAXWIFIRETRY), WifiAPTimeout;
#endif
bool                              powerLedOn(true), wifiLedOn(false);

struct ntpConf{
  String                          source;
  short                           zone;
  bool                            dayLight;
} ntp;

extern String                     defaultScript;

struct pinConf{
  std::vector<String>             gpio;
  std::map<String,String>         name, gpioVar;
  std::map<String,bool>           state;
  std::map<String,ushort>         mode;
} pin;

String                            serialInputString;
bool                              isSlave;

extern ESP8266WebServer           ESPWebServer;

WiFiClient                        ethClient;
PubSubClient                      mqttClient(ethClient);
struct mqttConf{
  String                          broker, idPrefix, user, pwd, topic;
  ushort                          port;
} mqtt;

ESP8266HTTPUpdateServer           httpUpdater;

#define WIFI_STA_Connected()     (WiFi.status()==WL_CONNECTED)

#ifdef DEBUG
  WiFiServer                      telnetServer(23);
  WiFiClient                      telnetClient;
#endif

bool WiFiHost(){
#ifdef DEFAULTWIFIPASS
  if(String(DEFAULTWIFIPASS).length()){
    DEBUG_print("\nNo custom SSID found: setting soft-AP configuration ... \n");
    WifiAPTimeout=(WIFIAPDELAYRETRY/WIFISTADELAYRETRY); nbWifiAttempts=MAXWIFIRETRY;
    WiFi.forceSleepWake(); delay(1L); WiFi.mode(WIFI_AP);
  //WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,254), IPAddress(255,255,255,0));
    WiFiAP=WiFi.softAP(String(DEFAULTHOSTNAME)+"-"+String(ESP.getChipId()), DEFAULTWIFIPASS);
    DEBUG_print(
      WiFiAP
      ?(String("Connecting \"" + hostname+ "\" [") + WiFi.softAPIP().toString() + "] from: " + DEFAULTHOSTNAME + "-" + String(ESP.getChipId()) + "/" + DEFAULTWIFIPASS + "\n\n").c_str()
      :"WiFi Timeout.\n\n");
    return WiFiAP;
  }
#endif
  return false;
}

void WiFiDisconnect(ulong duration=0L){
  next_reconnect=millis()+WIFISTADELAYRETRY;
  if(WiFiAP || WIFI_STA_Connected())
    DEBUG_print("Wifi disconnected!...\n");
  WiFi.softAPdisconnect(); WiFi.disconnect(); WiFiAP=false; wifiLedOn=false;
  WiFi.mode(WIFI_OFF);
  if(duration){
    WiFi.forceSleepBegin(); delay(1L);
    next_reconnect=millis()+duration;
} }

bool WiFiConnect(){
#ifdef DEFAULTWIFIPASS
  WiFiDisconnect(); WiFi.forceSleepWake(); delay(1L);
  DEBUG_print("\n");
  for(ushort i(0); i<SSIDCount(); i++) if(ssid[i].length()){

    //Connection au reseau Wifi /Connect to WiFi network
    WiFi.mode(WIFI_STA);
    DEBUG_print(String("Connecting \"" + hostname+ "\" [") + String(WiFi.macAddress()) + "] to: " + ssid[i]);
    WiFi.begin(ssid[i].c_str(), password[i].c_str());

    //Attendre la connexion /Wait for connection
    for(ushort j(0); j<12 && !WIFI_STA_Connected(); j++){
      delay(500L);
      DEBUG_print(".");
    }DEBUG_print("\n");

    if(WIFI_STA_Connected()){
      nbWifiAttempts=MAXWIFIRETRY;
      //Affichage de l'adresse IP /print IP address:
      DEBUG_print("WiFi connected\n");
      DEBUG_print("IP address: "); DEBUG_print(WiFi.localIP()); DEBUG_print(", dns: "); DEBUG_print(WiFi.dnsIP()); DEBUG_print("\n\n");
      return true;
    } WiFi.disconnect();
  }
  nbWifiAttempts--;
  if(ssid[0].length()){
    DEBUG_print("WiFi Timeout ("); DEBUG_print(nbWifiAttempts);
    DEBUG_print((nbWifiAttempts>1) ?" more attempts)." :" more attempt).\n");
  }else nbWifiAttempts=0;

  if(!nbWifiAttempts)
    return WiFiHost();
#endif
  return false;
}

void reboot(){
  if(!isSlave){
    DEBUG_print("Restart needed!...\n");
    Serial_print("S(.)\n"); delay(1500L);
    mustResto=true; writeConfig();
  }ESP.restart();
}

void memoryTest(){
#ifdef MEMORYLEAKS     //oberved on DNS server (bind9/NTP) errors -> reboot each ~30mn
  if(!isSlave){        //on WiFi(TCP) errors...
    ulong f=ESP.getFreeHeap();
    if(f<MEMORYLEAKS) reboot();
    DEBUG_print("FreeMem: " + String(f, DEC) + "\n");
  }
#endif
}

inline void onConnect(){
  MDNS.begin(hostname.c_str());
  MDNS.addService("http", "tcp", 80);
#ifdef WIFISTA_LED
  if(!WiFiAP)
    wifiLedOn=true;
#endif
#ifdef DEBUG
  if(!WiFiAP){ //bool b=true;
//    for(ushort i(0); b&&i<mode.size(); i++)
//      b=notifyProxy(i, hostname + " connected.");
    telnetServer.begin();
    telnetServer.setNoDelay(true);
  }
#endif
}

inline void ifConnected(){
  MDNS.update();
  if(ntp.source.length() && !isTimeSynchronized()){
    DEBUG_print("Retry NTP synchro...\n");
    NTP.getTime();
  }
#ifdef DEBUG
if(telnetServer.hasClient()){                   //Telnet client connection:
    if (!telnetClient || !telnetClient.connected()){
      if(telnetClient){
        telnetClient.stop();
        DEBUG_print("Telnet Client Stop\n");
      }telnetClient=telnetServer.available();
      telnetClient.flush();
      DEBUG_print("New Telnet client connected...\n");
    }
  }
#endif
}

void connectionTreatment(){                              //Test connexion/Check WiFi every mn:
  if(isNow(next_reconnect)){
    next_reconnect=millis()+WIFISTADELAYRETRY;
    memoryTest();

    if(isSlave){
      if(WiFiAP) WiFiDisconnect();
      return;
    }else if(Serial && !isMaster())
      Serial_print("M(?)\n");                            //Is there a Master here?...

#ifdef DEFAULTWIFIPASS
    if( (!WiFiAP && !WIFI_STA_Connected()) || (WiFiAP && ssid[0].length() && !WifiAPTimeout--) ){
      if(WiFiConnect())
        onConnect();
    }else ifConnected();
#endif
} }

void BlinkingLED(){
#ifdef POWER_LED
  static bool power=false;
 #ifdef BLINKING_POWERLED
  static ulong powerLed[2]={BLINKING_POWERLED};
 #else
  static ulong powerLed[2]={500UL,1000UL};
 #endif
  static ulong next_powerLed_up=powerLed[0], next_powerLed_down=powerLed[1];
  if(power){
    if(next_powerLed_down && isNow(next_powerLed_down))
      digitalWrite(POWER_LED, (power=false));
    next_powerLed_up=millis()+powerLed[0];
  }else if(powerLedOn){
    if(next_powerLed_up && isNow(next_powerLed_up))
      digitalWrite(POWER_LED, (power=true));
    next_powerLed_down=millis()+powerLed[1];
  }
#endif
#ifdef WIFISTA_LED
  static bool wifi=false;
 #ifdef BLINKING_WIFILED
  static ulong wifiLed[2]={BLINKING_WIFILED};
 #else
  static ulong wifiLed[2]={500UL,1000UL};
 #endif
  static ulong next_wifiLed_up=wifiLed[0], next_wifiLed_down=wifiLed[1];
  if(wifi){
    if(next_wifiLed_down && isNow(next_wifiLed_down))
      digitalWrite(WIFISTA_LED, (wifi=0));
    next_wifiLed_up=millis()+wifiLed[0];
  }else if(wifiLedOn){
    if(next_wifiLed_up && isNow(next_wifiLed_up))
      digitalWrite(WIFISTA_LED, (wifi=1));
    next_wifiLed_down=millis()+wifiLed[1];
  }
#endif
}

void shiftSSID(){
  for(ushort i(0); i<SSIDCount(); i++){
    if(!ssid[i].length() || !password[i].length()) ssid[i]=password[i]="";
    for(ushort n(SSIDCount()-i-1);!ssid[i].length() && n; n--){
      for(ushort j(i+1); j<SSIDCount(); j++){
        ssid[i]=ssid[j]; password[i]=password[j]; ssid[j]="";
} } } }

void writeConfig(){                        //Save current config:
  if(!readConfig(false))
    return;
  if( !SPIFFS.begin() ){
    DEBUG_print("Cannot open SPIFFS!...\n");
    return;
  }File f=SPIFFS.open("/config.txt", "w+");
  DEBUG_print("Writing SPIFFS.\n");
  if(f){
    f.println(String(VERSION).substring(0, String(VERSION).indexOf(".")));
    f.println(hostname);
    shiftSSID();for(ushort i(0); i<SSIDCount(); i++){  //Save SSIDs
      f.println(ssid[i]);
      f.println(password[i]);
    }f.println(mustResto);
    f.println(ntp.source);
    f.println(ntp.zone);
    f.println(ntp.dayLight);
    f.println(pin.state.size()); for(auto const& x : pin.state){
      f.println(x.first);
      f.println(x.second);
    }f.println(mqtt.broker);
    f.println(mqtt.port);
    f.println(mqtt.idPrefix);
    f.println(mqtt.user);
    f.println(mqtt.pwd);
    f.println(mqtt.topic);
    f.close(); SPIFFS.end();
    DEBUG_print("SPIFFS writed.\n");
} }

String readString(File f){ String ret=f.readStringUntil('\n'); ret.remove(ret.indexOf('\r')); return ret; }
template<typename T> bool getConf(T& v, File& f, bool wr=true){T r(atol(readString(f).c_str())); if(v==r) return false; if(wr)v=r; return true;}
template<> bool getConf(String& v, File& f, bool wr){String r(readString(f).c_str()); if(v==r) return false; if(wr)v=r; return true;}
template<typename T,typename U> bool getConf(std::map<T,U>& m, File& file, bool wr=true){
    ushort n=m.size(); bool isNew(getConf(n, file));
    if(isNew && wr) m.erase(m.begin(), m.end());
    for(ushort i=0; i<n; i++){
    T first (getConf(first,  file));
    U second(getConf(second, file));
    if( wr || !(isNew|=(m.find(first)==m.end())) )
      if( (isNew|=(m[first]!=second)) && wr) m[first]=second;
  }return isNew;
}bool readConfig(bool wr){
  bool isNew(false);
  if( !SPIFFS.begin() ){
    DEBUG_print("Cannot open SPIFFS!...\n");
    return false;
  }File f(SPIFFS.open("/config.txt", "r"));
  if(f && String(VERSION).substring(0, String(VERSION).indexOf("."))!=readString(f)){
    f.close();
    if(wr) DEBUG_print("New configFile version...\n");
  }if(!f){
    if(wr){    //Write default config:
#ifdef DEFAULT_MQTT_BROKER
      mqtt.broker   =DEFAULT_MQTT_BROKER;    mqtt.port=DEFAULT_MQTT_PORT;
      mqtt.idPrefix =DEFAULT_MQTT_IDPREFIX;  mqtt.user=DEFAULT_MQTT_USER; mqtt.pwd=DEFAULT_MQTT_PWD;
      mqtt.topic    =DEFAULT_MQTT_TOPIC;
#endif
      SPIFFS.format(); SPIFFS.end(); writeConfig();
      DEBUG_print("SPIFFS initialized.\n");
    }return true;
  }isNew|=getConf(hostname, f, wr);
  for(ushort i(0); i<SSIDCount(); i++){
    isNew|=getConf(ssid[i], f, wr);
    isNew|=getConf(password[i], f, wr);
  }isNew|=getConf(mustResto, f, wr);
  isNew|=getConf(ntp.source, f, wr);
  isNew|=getConf(ntp.zone, f, wr);
  isNew|=getConf(ntp.dayLight, f, wr);
  isNew|=getConf<String, bool>(pin.state, f, wr);
  isNew|=getConf(mqtt.broker, f, wr);
  isNew|=getConf(mqtt.port, f, wr);
  isNew|=getConf(mqtt.idPrefix, f, wr);
  isNew|=getConf(mqtt.user, f, wr);
  isNew|=getConf(mqtt.pwd, f, wr);
  isNew|=getConf(mqtt.topic, f, wr);
  f.close(); SPIFFS.end();
  if(wr) DEBUG_print("Config restored.\n");
  return isNew;
}

bool mqttSend(String s){ //Send MQTT OUTPUT string:
  if(mqtt.broker.length()){
    mqttClient.setServer(mqtt.broker.c_str(), mqtt.port);
    if(!mqttClient.connected())
      mqttClient.connect(mqtt.idPrefix.c_str(), mqtt.user.c_str(), mqtt.pwd.c_str());
    if(mqttClient.connected()){
      if(s.length())
      mqttClient.publish(mqtt.topic.c_str(), s.c_str());
      DEBUG_print("'" + s + "' published to '" + mqtt.broker + "(" + mqtt.topic + ")'.\n");
      return true;
    }else DEBUG_print("Cannot connect MQTT broker '" + mqtt.broker + "'!\n");
  }return false;
}

// ***********************************************************************************************
// **************************************** SETUP ************************************************
void setup(){
  Serial.begin(115200);
  while(!Serial);
  Serial_print("\n\nChipID(" + String(ESP.getChipId(), DEC) + ") says:\nHello World!\n\n");

  ntp.source=""; ntp.dayLight=(ntp.zone=0);
#ifdef DEFAULT_NTPSERVER
  if(String(DEFAULT_NTPSERVER).length()){
    ntp.source=DEFAULT_NTPSERVER;
    ntp.zone=DEFAULT_TIMEZONE;
    ntp.dayLight=DEFAULT_DAYLIGHT;
  }
#endif

  readConfig();

  // Servers:
  WiFi.softAPdisconnect(); WiFi.disconnect();
  setupWebServer();
  httpUpdater.setup(&ESPWebServer);  //Adds OnTheAir updates:

#ifdef POWER_LED
  pinMode(POWER_LED,   OUTPUT);
  if(POWER_LED==3 || POWER_LED==1) Serial.end();
#endif
#ifdef WIFISTA_LED
  if(POWER_LED==3 || POWER_LED==1) Serial.end();
  pinMode(WIFISTA_LED, OUTPUT);
#endif

  if(Serial){
    serialInputString.reserve(32);
    if(isMaster())
      setAllPinsOnSlave();
  }

  if(ntp.source.length()){
    NTP.begin(ntp.source, ntp.zone, ntp.dayLight);
  #ifdef NTP_INTERVAL
    NTP.setInterval(NTP_INTERVAL);
  #else
    NTP.setInterval(3600);
  #endif
#ifdef DEBUG
    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
      if (error) {
        DEBUG_print("Time Sync error: ");
        if (error == noResponse){
          DEBUG_print("NTP server not reachable\n");
        }else if (error == invalidAddress){
          DEBUG_print("Invalid NTP server address\n");
        }else{
          DEBUG_print(error);DEBUG_print("\n");
      } }
      else {
        DEBUG_print("Got NTP time: ");
        DEBUG_print(NTP.getTimeDateString(NTP.getLastNTPSync()));
        DEBUG_print("\n");
    } });
#endif
  }
}

// **************************************** LOOP *************************************************
void loop(){
  ESPWebServer.handleClient(); delay(1L);

  connectionTreatment();                //WiFi watcher
  treatment(defaultScript);             //Gestion des switchs/Switchs management
  BlinkingLED();
}
// ***********************************************************************************************
