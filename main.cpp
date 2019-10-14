//Reference: https://www.arduino.cc/en/Reference/HomePage
//See: http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
//Librairies et cartes ESP8266 sur IDE Arduino: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//http://arduino-esp8266.readthedocs.io/en/latest/
//JSon lib: see https://github.com/bblanchon/ArduinoJson.git
//peychart@netcourrier.com 20171021
// Licence: GNU v3
#include <string.h>
#include "FS.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
//#include <Ethernet.h>
#include <uart.h>

#include "setting.h"   //Can be adjusted according to the project...

//Avoid to change the following:
#define DEBOUNCE_TIME             100L
#define INFINY                    60000L
String                            hostname(DEFAULTHOSTNAME);    //Can be change by interface
String                            ssid[SSIDCount()];            //Identifiants WiFi /Wifi idents
String                            password[SSIDCount()];        //Mots de passe WiFi /Wifi passwords
std::vector<String>              _outputName;
std::vector<ushort>               outputValue, outputReverse;   //No bool/char because of this fucking compilator
static bool                       WiFiAP(false), mustResto(false);
#ifdef DEFAULTWIFIPASS
  static ushort                   nbWifiAttempts(MAXWIFIRETRY), WifiAPTimeout;
#endif
static unsigned long              next_reconnect(0L);
static std::vector<unsigned long> maxDuration, timerOn;
volatile short                    intr(0);
volatile unsigned long            rebounds_completed;
volatile bool                     master(false), slave(false);
static String                     serialInputString;
ESP8266WebServer                  ESPWebServer(80);
ESP8266HTTPUpdateServer           httpUpdater;

WiFiClient                        ethClient;
PubSubClient                      mqttClient(ethClient);
String                            mqttBroker, mqttIdent, mqttUser, mqttPwd, mqttQueue;
ushort                            mqttPort;
std::vector<std::vector<String>>  mqttFieldName, mqttOnValue, mqttOffValue;
std::vector<std::vector<ushort>>  mqttNature, mqttType;
std::vector<ushort>               mqttEnable;

#define Serial_print(m)          {if(Serial) Serial.print(m);}
#define Serial_printf(m,n)       {if(Serial) Serial.printf(m,n);}
#ifdef DEBUG
  WiFiServer                      telnetServer(23);
  WiFiClient                      telnetClient;
  #ifdef DEFAULTWIFIPASS
    #define DEBUG_print(m)       {if(telnetClient && telnetClient.connected()) telnetClient.print(m);    Serial_print(m);}
    #define DEBUG_printf(m,n)    {if(telnetClient && telnetClient.connected()) telnetClient.printf(m,n); Serial_printf(m,n);}
  #else
    #define DEBUG_print(m)        Serial_print(m);
    #define DEBUG_printf(m,n)     Serial_printf(m,n);
  #endif
#else
  #define DEBUG_print(m)          ;
  #define DEBUG_printf(m,n)       ;
#endif

#define WIFI_STA_Connected()     (WiFi.status()==WL_CONNECTED)
#define isMaster()                master
#define isSlave()                 slave
#define inputCount()              _inputPin.size()
#define outputCount()             atoi(outputName(-1).c_str())
#define clearOutputCount()        outputName(-2)

bool notifyHTTPProxy(ushort, String="");
bool readConfig(bool=true);
void writeConfig();

inline bool isNow(unsigned long v) {unsigned long ms(millis()); return((v<ms) && (ms-v)<INFINY);}  //Because of millis() rollover:
inline void setTimer    (ushort i) {timerOn[i] = ( ((long)maxDuration[i]==(-1L)) ?maxDuration[i] :(millis() + (1000L*maxDuration[i])) );}
inline void unsetTimer  (ushort i) {timerOn[i] = (unsigned long)(-1L);}
inline bool isTimer     (ushort i) {return((timerOn[i]!=(unsigned long)(-1L)));}

inline String getHostname()        {return hostname;}

bool addMQTT(ushort i, ushort j){bool isNew(false);
  std::vector<String> s;
  std::vector<ushort> n;
  while(mqttFieldName.size()<=i){isNew=true;
    mqttFieldName.push_back(s);
    mqttNature.push_back(n);
    mqttType.push_back(n);
    mqttOnValue.push_back(s);
    mqttOffValue.push_back(s);
  }while(mqttFieldName[i].size()<=j){isNew=true;
    mqttFieldName[i].push_back("");mqttNature[i].push_back(0);mqttType[i].push_back(0);mqttOnValue[i].push_back("");mqttOffValue[i].push_back("");
  }return isNew;
}

void addSwitch(){
  ushort i(_outputName.size());
  if(!i){
    _outputName.reserve(i); outputValue.reserve(i); maxDuration.reserve(i); timerOn.reserve(i);
    mqttFieldName.reserve(i); mqttNature.reserve(i); mqttType.reserve(i); mqttOnValue.reserve(i); mqttOffValue.reserve(i);
  };if(i<_outputPin.size())
        _outputName.push_back("Switch" + String(i+1, DEC));
  else  _outputName.push_back("Serial" + String(i-_outputPin.size()+1, DEC));
  outputValue.push_back(0);
  outputReverse.push_back((ushort)REVERSE_OUTPUT);
  maxDuration.push_back((unsigned long)(-1L));
  timerOn.push_back((unsigned long)(-1L));
  mqttEnable.push_back(0);
}

String& outputName(ushort n){
  static ushort serialPinCount(0);
  static String pinCount;
  if(n >= ushort(-2)){                                        //return count
    if(n==ushort(-2)) serialPinCount=0;                       //reset serial pins
    return(pinCount=(_outputPin.size()+serialPinCount));
  }
  if(n>=_outputPin.size()+serialPinCount){
    for(ushort i=_outputPin.size()+serialPinCount; i<=n; i++){    //New serial pin(s)
      addSwitch();
      DEBUG_print("SerialPin '" + _outputName[i] + "' created.\n");
    }serialPinCount=n-_outputPin.size()+1;
  }return(_outputName[n]);
}

#define WEB_S(n) ESPWebServer.sendContent(n)
#define WEB_F(n) ESPWebServer.sendContent(F(n))
void sendHTML_input(String name, String type, String more){
  WEB_S( "<input"+ (name.length() ?(" id='" + name + "' name='" + name + "'") : String("")) + " type='" + type + (more.length()?"' ":"'") + more + ">\n" );
}void sendHTML_inputText   (String name, String val, String more=""){sendHTML_input(name, F("text"),    "value='" + val + (more.length()?"' ":"'") + more);}
void  sendHTML_inputNumber (String name, String val, String more=""){sendHTML_input(name, F("number"),  "value='" + val + (more.length()?"' ":"'") + more);}
void  sendHTML_checkbox    (String name, bool   val, String more=""){sendHTML_input(name, F("checkbox"), more + (val ?" checked" :""));}
void  sendHTML_button      (String name, String val, String more=""){sendHTML_input(name, F("button"),  "value='" + val + (more.length()?"' ":"'") + more);}
void  sendHTML_optionSelect(String lib,  String val, String more=""){WEB_S("<option value='" + val + (more.length()?"' ":"'") + more + ">" + lib + "</option>\n");}

void sendHTML(bool blankPage=false){
  ESPWebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  ESPWebServer.send(200, "text/html", F("<!DOCTYPE HTML>\n<html lang='us-US'>\n<head><meta charset='utf-8'/>\n"));
  if(!blankPage){
    WEB_F("<title>");
    WEB_S(getHostname());
    WEB_F("</title>\n\
<style>\n\
body{background-color: #fff7e6;font-family: Arial,Helvetica,Sans-Serif;Color: #000088;}\n\
 .modal {display: none;position: fixed;z-index: 1;left: 0%;top: 0%;height: 100%;width: 100%;overflow: scroll;background-color: #000000;}\n\
 .modal-content {background-color: #fff7e6;margin: 5% auto;padding: 15px;border: 2px solid #888;height: 90%;width: 90%;min-height: 755px;}\n\
 .close {color: #aaa;float: right;font-size: 30px;font-weight: bold;}\n\
 td.switches {text-align: left;min-height: 60px;vertical-align: middle;display: inline-block;overflow: hidden;text-overflow: ellipsis;white-space: nowrap;}\n\
 .bulle {border: none;height: 20px;width: 20px;cursor: pointer;background-repeat: no-repeat;background: transparent;background-position: center;vertical-align: center;background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA+pJREFUeNrsV01sG0UUnvV6bdrY2fVPcmkStw60NFYaYlIEoogIeuBSWb2CkLggwg0pByr1XCWghBNCvVWoEgeEiJBA0HAgURRDmuCgFEd1owSnCWkSx/tjr3/W3h/eLONqFaWx143US2b1aXbfzpv3vTfz3uxShmGgZ9kc6Bm3YwLHBJxHMcn4+HjDY4eHh5+OABhzQ9cGOAXwAxhAFcAD/gVkAMqRRwAM4+UKAgY21/8ZkvjsRU1XW1wMQyNE6Zqmy6w/MN8ROnMTxiwA9gD6kRAA4y7owpIovL96P/lRwMcFIpHzqKWlBVFwGXBVlIpne/vRlXt/zr3e1X32Jsv5boPOGqDyVASI5+GH6bUbm+nVWG/vBbqtLYhoB40cFGWOwSRcLhc61dGBOF/An1xOfsoF2l7sOh2+Dq8fHBaJRrLAz2f33l1ZTsb6XuqnW70sqlZUpCgKKpXKj4GfsdxJO1FPTw+9t/0ohvXIPmkuAuA9dvHc0mJiKBQO0xR4XVTKdRnj8+V0dzedWk4OvfbGm3dAlMViM1oQNev5U28JTggC/2omsxsEr1ChUGw4W7ARTdeDWN/n8y+CqNjMHvA8XF+/dPKkhyoUS7brg9fjpbA+ELjdFAFVVZ8TBDHESxLiRQG53W5bBDRADvShc9veA7BW1MjISBXCqN1PpVAKwLIs4jjOXEev14tomjZTkWEYJIqiqZfL5ZCu60iWZcQLAhocHNSq1aqKx9iOwM7OThEMrSlKZQA/l3czaGc3Y3cZViVJKoyOjlJjY2OGnTSkZmdnlWAg8GtXZ2dTZwTWa29vv5NIJMpN1YH5+XkjL8vxl6P9WYoUnUYbHj8QjWZL5XJ8cnLSaLYQOaampni21fPF5bffskUAj/f5Wj+fnp7eM4ulHQLU/+6a2NjYUKdm4j88H+qcuBq7Ypbcwxp+j8edPRP6biY+N7GysqLaroRQRAzgYJDqpafT6cL3P/78Weydy/lPPv7wvbuLS/S9ZBJls/xjnUDAj3ojEfRK/wUNMuHriZ9++XJra0uudyI6G0hl7IECGSHe+ubbr6J9vb+BkQ8uXeyPOhnGK0p5xLFepFar+Vw+vzDz+x+3FhaX/obUE8l3gnYYicMIGEQRE8C72AGT6nMLib8A13C0OZY90Rc598LScuqBIEoFcvQWLVBsE9i322sEaoRUMimuKowoSfR0/O4meacRjxULahGwnwWkaNSMVkgUsJd5gAQQyGeYtcdy2eK9WvP+oCJUNw0tJKzeFQkRmZDJkXuZyEuEsFrz/knGzYjv/zd8QsGhDrin9skNC6yyA49qux+ldSc9/jNqtv0nwAAyrctSAHsdjQAAAABJRU5ErkJggg==');}\n\
 .outputName {width: 150px;font-size: 25px;color: #000088;border: none;background: transparent;}\n\
 .delayConf {float: left;vertical-align: middle;display: inline-block;overflow: hidden;text-overflow: ellipsis;white-space: nowrap;}\n\
 .duration {width: 50px;} .sDuration {width: 50px;}\n\
/*see: https://proto.io/freebies/onoff/: */\n\
 .onoffswitch {position: relative;width: 90px;-webkit-user-select: none;-moz-user-select: none;-ms-user-select: none;}\n\
 .onoffswitch-checkbox {display: none;}\n\
 .onoffswitch-label {display: block;overflow: hidden;cursor: pointer;border: 2px solid #999999;border-radius: 20px;}\n\
 .onoffswitch-inner {display: block;width: 200%;margin-left: -100%;transition: margin 0.3s ease-in 0s;}\n\
 .onoffswitch-inner:before, .onoffswitch-inner:after {display: block;float: left;width: 50%;height: 30px;padding: 0;line-height: 30px;font-size: 14px;color: white;font-family: Trebuchet, Arial, sans-serif;font-weight: bold;box-sizing: border-box;}\n\
 .onoffswitch-inner:before{content: 'ON';padding-left: 10px;background-color: #34C247;color: #FFFFFF;}\n\
 .onoffswitch-inner:after {content: 'OFF';padding-right: 10px;background-color: #EEEEEE;color: #999999;text-align: right;}\n\
 .onoffswitch-switch{display: block;width: 18px;margin: 6px;background: #FFFFFF;position: absolute;top: 0;bottom: 0;right: 56px;border: 2px solid #999999;border-radius: 20px;transition: all 0.3s ease-in 0s;}\n\
 .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-inner {margin-left: 0;}\n\
 .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-switch {right: 0px;}\n\
 .onofftimer {right: 0px;vertical-align: middle;}\n\
 .confPopup {position: relative;opacity: 0;display: none;-webkit-transition: opacity 400ms ease-in;-moz-transition: opacity 400ms ease-in;transition: opacity 400ms ease-in;}\n\
 .confPopup:target {opacity: 1;display: block;}\n\
 .confPopup > div {width: 600px;position: fixed;top: 25px;left: 25px;margin: 10% auto;padding: 5px 20px 13px 20px;border-radius: 10px;background: #71a6fc;background: -moz-linear-gradient(#71a6fc, #fff);background: -webkit-linear-gradient(#71a6fc, #999);}\n\
 .closeconfPopup {background: #606061;color: #FFFFFF;line-height: 25px;position: absolute;right: -12px;text-align: center;top: -10px;width: 24px;text-decoration: none;-webkit-border-radius: 12px;-moz-box-shadow: 1px 1px 3px #000;}\n\
 .closeconfPopup:hover {background: #00d9ff;}\n\
</style></head>\n");
  }if(blankPage){
    WEB_F("<body>\n");
  }else{
    WEB_F("<body onload='init();'>\n\
<div id='about' class='modal'><div class='modal-content'><span class='close' onClick='refresh();'>&times;</span><h1>About</h1>\
This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domoticz or Jeedom.<br><br>\
In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). \
Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs, like this:\
<a id='example1' style='padding:0 0 0 5px;'></a><br><br>\
The state of the electrical outlets can also be requested from the following URL: <a id='example2' style='padding:0 0 0 5px;'></a>.\
 In addition, a gateway (ip=NOTYFYPROXY & port=NOTIFYPort in \"settingX.h\" file) can be notified of each of the state changes in order, for example, to relay the state of the switches (on manual action) to the centralized home automation interface.<br><br>\
The status of the power strip can be retained when the power is turned off and restored when it is turned on ; a power-on duration can be set on each output: (-1) no delay, (0) to disable an output and (number of s) to configure the power-on duration.<br><br>\
A second slave module (without any declared WiFi SSID) can be connected to the first (which thus becomes master by automatically detecting a slave) through its UART interface in order to increase the number of outputs to a maximum of 12 on the same management interface. \
The manual action of these additional switches adds them automatically to the web interface (on refresh). The \"clear\" button can be used to remove them on slave disconnection.<br><br>\
The following allows you to configure some parameters of the Wifi Power Strip (as long as no SSID is set and it is not connected to a master, the device acts as an access point with its own SSID and default password.: \"");
    WEB_S(String(DEFAULTHOSTNAME)); WEB_F("-nnn/");
#ifdef DEFAULTWIFIPASS
    WEB_S(String(DEFAULTWIFIPASS).length() ?F(DEFAULTWIFIPASS) :F("none"));
#else
    WEB_F("none");
#endif
    WEB_F("\" on 192.168.4.1).<br><br>\n<table width='100%'><tr style='white-space: nowrap;'><td>\n<form method='POST'>\n<h2>Network name: ");
    sendHTML_inputText(F("hostname"), getHostname(), "size='10'");
    sendHTML_button("", F("Submit"), F("onclick='submit();'"));
    WEB_F("</h2></form>\n</td><td style='text-align: right;'>\n<form method='POST'><h2>Clear serial devices : ");
    sendHTML_button(F("Clear"), F("Clear"), F("onclick='submit();'"));
    WEB_F("</h2>");
//    sendHTML_checkbox(F("Reboot"), false, F("checked style='display:none;'"));
    WEB_F("&nbsp;</form>\n</td></tr>\n</table>\n<h2>Network connection:</h2><table><tr>");
    for(ushort i(0); i<SSIDCount(); i++){
      WEB_F("<td>\n<form method='POST'><table>\n<tr><td>SSID ");
      WEB_S(String(i+1, DEC));
      WEB_F(":</td></tr>\n<tr><td><input type='text' name='SSID' value='");
      WEB_S(ssid[i] + (ssid[i].length() ?F("' readonly><br>\n"): F("'></td></tr>\n")));
      WEB_F("<tr><td>Password:</td></tr>\n<tr><td><input type='password' name='password' value='");
      if(password[i].length()) WEB_S(password[i]);
      WEB_F("'></td></tr>\n<tr><td>Confirm password:</td></tr>\n<tr><td><input type='password' name='confirm' value='");
      if(password[i].length()) WEB_S(password[i]);
      WEB_F("'></td></tr>\n<tr><td><input type='button' value='Submit' onclick='saveSSID(this);'><input type='button' value='Remove' onclick='deleteSSID(this);'></td></tr>\n</table></form>\n</td>\n");
    }WEB_F("</tr></table>\n\
<h6><a href='update' onclick='javascript:event.target.port=80'>Firmware update</a> - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>\n\
</div></div>\n");
//                             -----------------------------------------------------------------
    WEB_F("\n<!MAIN FORM>\n<table id='main' width='100%'><tr style='height:75px;'>\n<td style='width:800px;'><h1>");
    WEB_S(getHostname()); WEB_F(" - ");WEB_S(WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString()); WEB_F(" ["); WEB_S(WiFi.macAddress());
    WEB_F("]</h1></td>\n<td style='text-align:right;vertical-align:top;'><div style='text-align:right;white-space: nowrap;'><p><span class='close' onclick='showHelp();'>?</span></p></div></td>\n");
    WEB_F("</tr></table>\n<h3>Status :</h3>\n<table width='100%'>\n");
    for (ushort i=0; i<outputCount(); i++){ bool display;
      //Bullet:
      WEB_F("<tr><td class='switches'><form method='POST'><table>\n<!");
      WEB_S(outputName(i));
      WEB_F(":><tr>\n<td style='width:30px;'></td>\n<td style='width:30px;'><div><button id='");
      WEB_S(String(i, DEC));
      WEB_F("' class='bulle' title=\"'");
      WEB_S(outputName(i));
      WEB_F("' configuration\" onclick='initConfPopup(this);'></button></div></td>\n");
      // Tittle:
      //WEB_S(outputName(i));
      WEB_F("<td style='width:150px;'><div class='outputName'>");
      WEB_S(outputName(i));
      WEB_F("</div></td>\n");
      // Switch:
      WEB_F("<td><div class='onoffswitch delayConf'>\n");
      sendHTML_checkbox(outputName(i), outputValue[i], F("class='onoffswitch-checkbox' onClick='switchSubmit(this);'"));
      WEB_F("<label class='onoffswitch-label' for='");
      WEB_S(outputName(i));
      WEB_F("'><span class='onoffswitch-inner'></span><span class='onoffswitch-switch'></span></label>\n</div>\n<div class='delayConf'>&nbsp;&nbsp;&nbsp;\n");
      //Timer:
      WEB_F("(Timer&nbsp;");
      WEB_F("<input type='checkbox' name='");
      WEB_S(outputName(i));
      WEB_F("-timer' ");
      WEB_S((outputValue[i] || ((signed)maxDuration[i]==(-1L))) ?F("uncheck"): F("checked"));
      if((signed)maxDuration[i]==(-1L) || !maxDuration[i]) WEB_F(" disabled");
      WEB_F(" class='onofftimer'>:&nbsp;\n");
      // Days duration:
      display=( (signed)maxDuration[i]!=(-1L) && (maxDuration[i]/86400L)>0L );
      WEB_F("<input type='number' name='"); WEB_S(outputName(i)); WEB_F("-max-duration-d' value='"); WEB_S(String(display ?(maxDuration[i]/86400L) :0L, DEC));
      WEB_F("' min='0' max='31' data-unit=86400 class='duration' style='width:60px;display:");
      WEB_S(display ?F("inline-block") :F("none")); WEB_F(";' onChange='checkDelay(this);'>"); WEB_S(display ?F("d &nbsp;\n") :F("\n"));
      // Hours duration:
      display|=( (signed)maxDuration[i]!=(-1L) && (maxDuration[i]%86400L/3600L)>0L );
      WEB_F("<input type='number' name='"); WEB_S(outputName(i)); WEB_F("-max-duration-h' value='"); WEB_S(String(display ?(maxDuration[i]%86400L/3600L) :0L, DEC));
      WEB_F("' min='0' max='24' data-unit=3600 class='duration' style='display:");
      WEB_S(display ?F("inline-block") :F("none"));
      WEB_F(";' onChange='checkDelay(this);'>");
      WEB_S(display ?F("h &nbsp;\n") :F("\n"));
      // Minutes duration:
      display|=( (signed)maxDuration[i]!=(-1L) && (maxDuration[i]%86400L%3600L/60L)>0L );
      WEB_S("<input type='number' name='"); WEB_S(outputName(i)); WEB_S("-max-duration-mn' value='"); WEB_S(String(display ?(maxDuration[i]%86400L%3600L/60L) :0L, DEC));
      WEB_F("' min='0' max='60' data-unit=60 class='duration' style='display:");
      WEB_S(display ?F("inline-block") :F("none"));
      WEB_F(";' onChange='checkDelay(this);'>"); WEB_S(display ?F("mn &nbsp;\n") :F("\n"));
      // Secondes duration:
      WEB_F("<input type='number' name='"); WEB_S(outputName(i)); WEB_F("-max-duration-s' value='");
      WEB_S( ((signed)maxDuration[i]!=(-1L)) ? String(maxDuration[i]%86400L%3600L%60L, DEC) : String(-1L, DEC) );
      WEB_F("' min='-1' max='60' data-unit=1 class='duration sDuration' onChange='checkDelay(this);'>");
      WEB_S( ((signed)maxDuration[i]!=(-1L)) ?F("s") :F("-") );
      WEB_F(")</div>\n</td></tr>\n</table><div style='display: none;'>\n");
      sendHTML_checkbox("newValue"+String(i, DEC), true);

      //Parameters:
      WEB_F("</div></form>\n<!");
      WEB_S(outputName(i));
      WEB_F(" parameters:>\n<div style='display: none;'>\n");
      sendHTML_checkbox ("outputReverse"   +String(i, DEC),                    outputReverse[i]);
      sendHTML_checkbox ("mqttEnable"      +String(i, DEC),                    mqttEnable[i]);
      for(ushort j(0); j<mqttEnable[i]; j++){
        sendHTML_inputText ("mqttFieldName"+String(i, DEC)+"."+String(j, DEC), mqttFieldName[i][j]);
        sendHTML_inputText ("mqttNature"   +String(i, DEC)+"."+String(j, DEC), String(mqttNature[i][j], DEC));
        sendHTML_inputText ("mqttType"     +String(i, DEC)+"."+String(j, DEC), String(mqttType[i][j], DEC));
        sendHTML_inputText ("mqttOnValue"  +String(i, DEC)+"."+String(j, DEC), mqttOnValue[i][j]);
        sendHTML_inputText ("mqttOffValue" +String(i, DEC)+"."+String(j, DEC), mqttOffValue[i][j]);
      }WEB_F("</div></td>\n</tr>");
    }WEB_F("</table>\n<h6>(V"); WEB_S(String(ResetConfig,DEC));
    WEB_F(", Uptime: ");
    unsigned long sec=millis()/1000L;
    WEB_S(String(sec/(24L*3600L)) + "d-");
    WEB_S(String((sec%=24L*3600L)/3600L) + "h-");
    WEB_S(String((sec%=3600L)/60L) + "mn)</h6>\n\n");

    WEB_F("<!Configuration popup:>\n<div id='confPopup' class='confPopup'><div>\n<form id='mqttConf' method='POST'>");
    sendHTML_inputText(F("plugNum"), "", F("style='display:none;'"));
    WEB_F("<a title='Save configuration' class='closeconfPopup' onclick='closeConfPopup();'>X</a>\n");
    WEB_F("<table width=100%><col width=90%><tr><td><div style='text-align:center;'><h2>configuration:</h2></div></td></tr></table>\n\
<h3 title='for this switch'>Output parameters</h3>\n\
<table title='for this switch' width=100%>\n<col width='50%'>\n<tr>\n\
<td style='text-align:center;'>Plug Name<br>");
    sendHTML_inputText(F("plugName"), "", F("style='width:150px;'"));
    WEB_F("</td>\n<td align=center>Reverse level<br>");
    sendHTML_checkbox(F("outputReverse"), false, F("style='vertical-align:middle;'"));
    WEB_F("</td>\n</tr></table>\n<h3 title='for this switch'>MQTT parameters \n");
    //MQTT configuration:
    sendHTML_checkbox(F("mqttEnable"), false, "style='vertical-align:right;' onclick='refreshConfPopup();'");
    WEB_F("</h3>\n<div id='mqttParams'><p align=center title='for all switches'>Broker: ");
    sendHTML_inputText(F("mqttBroker"), mqttBroker, F("style='width:65%;'"));
    WEB_S(":");
    sendHTML_inputNumber(F("mqttPort"), String(mqttPort,DEC), F("min='0' max='-1' style='width:10%;'"));
    WEB_F("</p>\n<table width=100%>\n<col width='42%'><col width='30%'><tr title='for all switches' style='white-space: nowrap;'><td>\nIdentification: ");
    sendHTML_inputText(F("mqttIdent"), mqttIdent, F("style='width:120px;'"));
    WEB_F("</td><td>\nUser: ");
    sendHTML_inputText(F("mqttUser"), mqttUser, F("style='width:120px;'"));
    WEB_F("</td><td>\nPassword: ");
    sendHTML_inputText(F("mqttPwd"), mqttPwd, F("style='width:75px;'"));
    WEB_F("</td></tr></table>\n<p align=center title='for all switches'>Topic: ");
    sendHTML_inputText(F("mqttTopic"), mqttQueue, F("style='width:80%;'"));
    WEB_F("</p>\n<table id='mqttRaws' title='for this switch' border='1' cellpadding='10' cellspacing='1' width='100%'>\n\
<col width='16%'><col width='22%'><col width='22%'><col width='17%'><col width='17%'>\n\
<tr>\n<th>FieldName</th><th>Nature</th><th>Type</th><th>On value</th><th>Off value</th><th>");
    sendHTML_button("mqttPlus", "+", "title='Add a field name' style='background-color: rgba(0, 0, 0, 0);' onclick='mqttRawAdd();'");
    WEB_F("</th></tr>\n");
    WEB_F("</table>\n</div></form></div></div>\n\n\
<!FRAME /dev/null><iframe name='blankFrame' height='0' width='0' frameborder='0'></iframe>\n\
<script>\n\
this.timer=0;\n\
function init(){\n document.getElementById('");
    WEB_S(outputName(0));
    WEB_F("').focus();\n refresh(1);}\n\
function refresh(v=30){\n\
 clearTimeout(this.timer);document.getElementById('about').style.display='none';\n\
 if(v>0)this.timer=setTimeout(function(){RequestStatus();refresh();},v*1000);}\n\
function RequestStatus(){var j,ret,e,f,g,req=new XMLHttpRequest();\n\
 req.open('GET',location.protocol+'//'+location.host+'/plugValues',false);req.send(null);ret=req.responseText;\n\
 if((j=ret.indexOf('['))>=0)\n\
  if((e=document.getElementsByClassName('onoffswitch-checkbox')).length && (f=document.getElementsByClassName('onofftimer')).length && (g=document.getElementsByClassName('sDuration')).length)\n\
   for(var v,i=0,r=ret.substr(j+1);r[0] && r[0]!=']';i++){\n\
    if((j=r.indexOf(','))<0)j=r.indexOf(']');\n\
    if(j>1 && ((r[0]=='\"'&&r[j-1]=='\"')||(r[0]=='\\\''&&r[j-1]=='\\\'')))v=parseInt(r.substr(1,j-2));else v=parseInt(r.substr(0,j));\n\
    e[i].checked=v;f[i].checked=!v;if(g[i].value=='-1')f[i].disabled=!(f[i].checked=false);\n\
    r=r.substr(j+1);\n\
}  }\n\
function showHelp(){var e=document.getElementById('example1');\n\
 e.innerHTML=location.protocol+'//'+location.host+'/plugValues?");
    WEB_S(outputName(0)); WEB_F("="); WEB_S(outputValue[0]?"true":"false");
    for(ushort i(1); i<3 && i<outputCount(); i++){
      WEB_F("&"); WEB_S(outputName(i)); WEB_F("="); WEB_S(outputValue[i]?"true":"false");
    }WEB_F("';e.href=e.innerHTML;\n\
 e=document.getElementById('example2');e.innerHTML=location.protocol+'//'+location.host+'/plugValues';e.href=e.innerHTML;\n\
 refresh(120);document.getElementById('about').style.display='block';\n}\n\
function saveSSID(e){var f,s;\n\
 for(f=e;f.tagName!='FORM';)f=f.parentNode;\n\
 if((s=f.querySelectorAll('input[type=text]')).length && s[0]==''){alert('Empty SSID...');f.reset();s.focus();}\n\
 else{var p=f.querySelectorAll('input[type=password]');\n\
  if(p[0].value!=p[1].value || p[0].value==''){\n\
   var ssid=s[0].value;s[0].value=ssid;\n\
   alert('Incorrect password...');p[0].focus();\n\
  }else f.submit();\n\
}}\n\
function deleteSSID(e){var f,s;\n\
 for(f=e;f.tagName!='FORM';)f=f.parentNode;\n\
 if((s=f.querySelectorAll('input[type=text]')).length && s[0].value!=''){\n\
  if(confirm('Are you sure to remove this SSID?')){\n\
   f.reset();s=f.getElementsByTagName('input');\n\
   for(var i=0; i<s.length; i++)if(s[i].type=='password')s[i].value='';\n\
   f.submit();\n\
 }}else alert('Empty SSID...');\n\
}\n\
function switchSubmit(e){var b=false,f,l;\n\
 for(f=e;f.tagName!='FORM';)f=f.parentNode;f.setAttribute('target', 'blankFrame');l=f.getElementsByTagName('input');\n\
 for(var i=0;i<l.length;i++)if(l[i].type=='number')b|=(l[i].value!='0');\n\
 e.checked&=b;refresh(1);\n f.submit();\n}\n\
var checkDelaySubmit=0;\n\
function checkDelay(e){var f,l;refresh();\n\
 for(f=e;f.tagName!='FORM';)f=f.parentNode;f.setAttribute('target', '_self');\n\
 if(e.value=='-1'){\n\
  l=e.parentNode.getElementsByTagName('input');\n\
  for(var i=0;i<l.length;i++) if(l[i].className=='duration'){\n\
   if(l[i].getAttribute('data-unit')!=1)l[i].style.display='none';\n\
   else l[i].style.display='inline-block';\n\
 }}\n\
 clearTimeout(this.checkDelaySubmit);\n\
 this.checkDelaySubmit=setTimeout(function(v=f){this.checkDelaySubmit=0;v.submit();}, 1000);\n\
}\n\
function setDisabled(v, b){for(var i=0;v[i];i++)v[i].disabled=b;}\n\
function mqttAllRawsRemove(){var t,r;for(t=document.getElementById('mqttPlus');t.tagName!='TR';)t=t.parentNode;t=t.parentNode;\n\
 r=t.getElementsByTagName('TR');while(r[1])t.removeChild(r[1]);\n\
}\n\
function mqttRawRemove(e){var t;for(t=e;t.tagName!='TR';)t=t.parentNode;t.parentNode.removeChild(t);}\n\
function mqttRawAdd(){var t;for(t=document.getElementById('mqttPlus');t.tagName!='TR';)t=t.parentNode;t=t.parentNode;\n\
 var i,tr,td,j=t.getElementsByTagName('TR'),n=j.length-1;\n\
 for(i=1;i<=n;i++)if(j[i].querySelectorAll('input[type=text]')[0].value==='')return false;\n\
 t.appendChild(tr=document.createElement('tr'));\n\
 tr.appendChild(td=document.createElement('td'));\n\
 td.appendChild(i=document.createElement('input'));i.id=i.name='mqttFieldName'+n;i.type='text';i.style='width:90%;';\n\
 tr.appendChild(td=document.createElement('td'));\n\
 td.appendChild(i=document.createElement('select'));i.id=i.name='mqttNature'+n;i.setAttribute('onchange','refreshConfPopup();');i.style='width:95%;text-align:center;';\n\
 i.appendChild(j=document.createElement('option'));j.value='0';j.innerHTML='Pin-stat';\n\
 i.appendChild(j=document.createElement('option'));j.value='1';j.innerHTML='Constant';\n\
 tr.appendChild(td=document.createElement('td'));\n\
 td.appendChild(i=document.createElement('select'));i.id=i.name='mqttType'+n;i.setAttribute('onchange','refreshConfPopup();');i.style='width:95%;text-align:center;';\n\
 i.appendChild(j=document.createElement('option'));j.value='0';j.innerHTML='String';\n\
 i.appendChild(j=document.createElement('option'));j.value='1';j.innerHTML='Number';\n\
 tr.appendChild(td=document.createElement('td'));td.style='text-align:center;';\n\
 td.appendChild(i=document.createElement('b'));i.innerHTML='\"';\n\
 td.appendChild(i=document.createElement('input'));i.id=i.name='mqttOnValue'+n;i.value='1';i.type='text';i.style='width:45%;text-align:center;';\n\
 td.appendChild(i=document.createElement('b'));i.innerHTML='\"';\n\
 tr.appendChild(td=document.createElement('td'));td.style='text-align:center;';\n\
 td.appendChild(i=document.createElement('b'));i.innerHTML='\"';\n\
 td.appendChild(i=document.createElement('input'));i.id=i.name='mqttOffValue'+n;i.value='0';i.type='text';i.style='width:45%;text-align:center;';\n\
 td.appendChild(i=document.createElement('b'));i.innerHTML='\"';\n\
 tr.appendChild(td=document.createElement('td'));td.style='text-align:center;';\n\
 td.appendChild(i=document.createElement('input'));i.id='mqttMinus'+n;i.type='button';i.value='-';i.style='background-color: rgba(0, 0, 0, 0);';i.setAttribute('onclick','mqttRawRemove(this);');\n\
}\n\
function checkConfPopup(){var r;\n\
 if(document.getElementById('plugName').value==='')return false;\n\
 if(!document.getElementById('mqttEnable').checked)return true;\n\
 if(document.getElementById('mqttBroker').value==='')return false;\n\
 if((r=document.getElementById('mqttRaws').getElementsByTagName('TR').length)<2)return false;\n\
 for(var i=0;i<r-1;i++){\n\
  if(document.getElementById('mqttFieldName'+i).value===''||document.getElementById('mqttOnValue'+i).value===''||document.getElementById('mqttOffValue'+i).value==='')return false;\n\
 }return true;\n\
}\n\
function refreshConfPopup(e=document.getElementById(document.getElementById('plugNum').value)){var t;for(t=e;t&&t.tagName!='TR';)t=t.parentNode;\n\
 for(var b,v,i=0,n=0,r=document.getElementById('mqttRaws').getElementsByTagName('TR');i<r.length;i++)\n\
  if((b=r[i].getElementsByTagName('B')).length){\n\
   b[0].innerHTML=b[1].innerHTML=b[2].innerHTML=b[3].innerHTML=(document.getElementById('mqttType'+n).value==='0'?'\"':'');\n\
   b[2].style.display=b[3].style.display=document.getElementById('mqttOffValue'+n).style.display=(document.getElementById('mqttNature'+n).value==='0'?'inline-block':'none');\n\
   n++;}\n\
 setDisabled(document.getElementById('mqttParams').getElementsByTagName('input'),!document.getElementById('mqttEnable').checked);\n\
 setDisabled(document.getElementById('mqttParams').getElementsByTagName('select'),!document.getElementById('mqttEnable').checked);\n\
}\n\
function initConfPopup(e){var f;for(f=e;f.tagName!='FORM';)f=f.parentNode;\n\
 f.setAttribute('target','blankFrame'); window.location.href='#confPopup';\n\
 var v=document.getElementById('plugNum').value=e.id;\n\
 document.getElementById('plugName').value=f.getElementsByClassName('onoffswitch-checkbox')[0].name\n\
 document.getElementById('confPopup').getElementsByTagName('H2')[0].innerHTML=\"'\"+document.getElementById('plugName').value+\"' configuration:\";\n\
 document.getElementById('outputReverse').checked=document.getElementById('outputReverse'+e.id).checked;\n\
 document.getElementById('mqttEnable').checked=document.getElementById('mqttEnable'+e.id).checked;\n\
 mqttAllRawsRemove(); for(var i=0;document.getElementById('mqttFieldName'+e.id+'.'+i);i++){ mqttRawAdd();\n\
  document.getElementById('mqttFieldName'+i).value=document.getElementById('mqttFieldName'+e.id+'.'+i).value;\n\
  document.getElementById('mqttNature'+i).value=document.getElementById('mqttNature'+e.id+'.'+i).value;\n\
  document.getElementById('mqttType'+i).value=document.getElementById('mqttType'+e.id+'.'+i).value;\n\
  document.getElementById('mqttOnValue' +i).value=document.getElementById('mqttOnValue' +e.id+'.'+i).value;\n\
  document.getElementById('mqttOffValue'+i).value=document.getElementById('mqttOffValue'+e.id+'.'+i).value;\n\
 }refreshConfPopup(e);\n\
}\n\
function closeConfPopup(){\n\
 if(checkConfPopup()){\n\
  var f=document.getElementById('mqttConf');f.setAttribute('target','blankFrame');\n\
  setTimeout(function(){window.location.href='';}, 1000);f.submit();\n\
}}\n\
</script>\n\
"); }
  WEB_F("</body>\n</html>\n\n");
  ESPWebServer.sendContent("");
  ESPWebServer.client().stop();
}

void shiftSSID(){
  for(ushort i(0); i<SSIDCount(); i++){
    if(!ssid[i].length() || !password[i].length()) ssid[i]=password[i]="";
    if(!ssid[i].length()) for(ushort j(i+1); j<SSIDCount(); j++)
      if(ssid[j].length() && password[j].length()){
        ssid[i]=ssid[j]; password[i]=password[j]; password[j]="";
        break;
      }else j++;
} }

void writeConfig(){                                     //Save current config:
  if(!readConfig(false))
    return;
  if( !SPIFFS.begin() ){
    DEBUG_print("Cannot open SPIFFS!...\n");
    return;
  }File f=SPIFFS.open("/config.txt", "w+");
  DEBUG_print("Writing SPIFFS.\n");
  if(f){
    f.println(ResetConfig);
    f.println(getHostname());                           //Save hostname
    shiftSSID(); for(ushort i(0); i<SSIDCount(); i++){  //Save SSIDs
      f.println(ssid[i]);
      f.println(password[i]);
    }f.println(mustResto);
    f.println(mqttBroker);
    f.println(mqttPort);
    f.println(mqttIdent);
    f.println(mqttUser);
    f.println(mqttPwd);
    f.println(mqttQueue);
    f.println(outputCount());
    unsigned long m=millis();
    for(ushort i(0); i<outputCount(); i++){      //Save output states
      f.println(outputName(i));
      f.println(outputReverse[i]);
      f.println(outputValue[i]);
      f.println((long)maxDuration[i]);
      f.println( (!isTimer(i) || (long)maxDuration[i]==(-1L)) ?(-1L) :(long)((timerOn[i]<m) ?(~m+timerOn[i]) :(timerOn[i]-m)) );
      f.println(mqttEnable[i]);
      for(ushort j(0); j<mqttEnable[i]; j++){
        f.println(mqttFieldName[i][j]);
        f.println(mqttNature[i][j]);
        f.println(mqttType[i][j]);
        f.println(mqttOnValue[i][j]);
        f.println(mqttOffValue[i][j]);
    } }
    f.close(); SPIFFS.end();
    DEBUG_print("SPIFFS writed.\n");
} }

String readString(File f){ String ret=f.readStringUntil('\n'); ret.remove(ret.indexOf('\r')); return ret; }
inline bool getConfig(std::vector<String>::iterator v, File& f, bool w=true){String r(readString(f).c_str());      if(r==*v) return false; if(w)*v=r; return true;}
inline bool getConfig(std::vector<ushort>::iterator v, File& f, bool w=true){ushort r(atoi(readString(f).c_str()));if(r==*v) return false; if(w)*v=r; return true;}
inline bool getConfig(String& v, File& f, bool w=true){String r(readString(f).c_str());       if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(bool&   v, File& f, bool w=true){bool   r(atoi(readString(f).c_str())); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(ushort& v, File& f, bool w=true){ushort r(atoi(readString(f).c_str())); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(long&   v, File& f, bool w=true){long   r(atol(readString(f).c_str())); if(r==v) return false; if(w)v=r; return true;}
bool readConfig(bool w){                                //Get config (return false if config is not modified):
  bool isNew(false);
  if( !SPIFFS.begin() ){
    DEBUG_print("Cannot open SPIFFS!...\n");
    return false;
  }File f(SPIFFS.open("/config.txt", "r"));
  if(f && ResetConfig!=atoi(readString(f).c_str())){
    f.close();
    if(w) DEBUG_print("New configFile version...\n");
  }if(!f){
    if(w){    //Write default config:
#ifdef DEFAULT_MQTT_SERVER
      mqttBroker=DEFAULT_MQTT_SERVER; mqttPort=DEFAULT_MQTT_PORT;
      mqttIdent=DEFAULT_MQTT_IDENT; mqttUser=DEFAULT_MQTT_USER; mqttPwd=DEFAULT_MQTT_PWD;
      mqttQueue=DEFAULT_MQTT_QUEUE;
#endif
      SPIFFS.format(); SPIFFS.end(); writeConfig();
      DEBUG_print("SPIFFS initialized.\n");
    }return true;
  }isNew|=getConfig(hostname, f, w);
  for(ushort i(0); i<SSIDCount(); i++){
    isNew|=getConfig(ssid[i], f, w);
    isNew|=getConfig(password[i], f, w);
  }isNew|=getConfig(mustResto, f, w);
  isNew|=getConfig(mqttBroker, f, w);
  isNew|=getConfig(mqttPort, f, w);
  isNew|=getConfig(mqttIdent, f, w);
  isNew|=getConfig(mqttUser, f, w);
  isNew|=getConfig(mqttPwd, f, w);
  isNew|=getConfig(mqttQueue, f, w);
  ushort n; getConfig(n, f, true); isNew|=(n!=outputCount());
  for(ushort i(0); (!isNew||w) && i<n ; i++){
    isNew|=getConfig(outputName(i), f, w);
    isNew|=getConfig(outputReverse[i], f, w);
    isNew|=getConfig(outputValue[i], f, w);
    isNew|=getConfig((long&)maxDuration[i], f, w);
    getConfig((long&)timerOn[i], f);
    if((long)maxDuration[i]==(-1L)) unsetTimer(i);
    else if(isTimer(i))     timerOn[i]+=millis();
    isNew|=getConfig(mqttEnable[i], f, w);
    for(ushort j(0); j<mqttEnable[i] && (!isNew||w); j++){
      isNew|=addMQTT(i, j);
      isNew|=getConfig(mqttFieldName[i][j], f, w);
      isNew|=getConfig(mqttNature[i][j], f, w);
      isNew|=getConfig(mqttType[i][j], f, w);
      isNew|=getConfig(mqttOnValue[i][j], f, w);
      isNew|=getConfig(mqttOffValue[i][j], f, w);
  }  }//MQTT parameters:
  f.close(); SPIFFS.end();
  if(w) DEBUG_print("Config restored.\n");
  return isNew;
}

bool WiFiHost(){
#ifdef DEFAULTWIFIPASS
  if(String(DEFAULTWIFIPASS).length()){
    DEBUG_print("\nNo custom SSID found: setting soft-AP configuration ... \n");
    WifiAPTimeout=(WIFIAPDELAYRETRY/WIFISTADELAYRETRY); nbWifiAttempts=MAXWIFIRETRY;
    WiFi.mode(WIFI_AP);
  //WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,254), IPAddress(255,255,255,0));
    WiFiAP=WiFi.softAP(String(DEFAULTHOSTNAME)+"-"+String(ESP.getChipId()), DEFAULTWIFIPASS);
    DEBUG_print(
      WiFiAP
      ?(String("Connecting \"" + getHostname()+ "\" [") + WiFi.softAPIP().toString() + "] from: " + DEFAULTHOSTNAME + "-" + String(ESP.getChipId()) + "/" + DEFAULTWIFIPASS + "\n\n").c_str()
      :"WiFi Timeout.\n\n");
    return WiFiAP;
  }
#endif
  return false;
}

void WiFiDisconnect(){
  next_reconnect=millis()+WIFISTADELAYRETRY;
  if(WiFiAP || WIFI_STA_Connected())
    DEBUG_print("Wifi disconnected!...\n");
  WiFi.softAPdisconnect(); WiFi.disconnect(); WiFiAP=false;
}

bool WiFiConnect(){
#ifdef DEFAULTWIFIPASS
  WiFiDisconnect();
  DEBUG_print("\n");
  for(ushort i(0); i<SSIDCount(); i++) if(ssid[i].length()){

    //Connection au reseau Wifi /Connect to WiFi network
    WiFi.mode(WIFI_STA);
    DEBUG_print(String("Connecting \"" + getHostname()+ "\" [") + String(WiFi.macAddress()) + "] to: " + ssid[i]);
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
  if(!isSlave()){
    DEBUG_print("Restart needed!...\n");
    Serial_print("S(.)\n"); delay(1500L);
    mustResto=true; writeConfig();
  }ESP.restart();
}

void setPin(ushort i, bool v, bool withNoTimer){
  if(i<outputCount() && outputValue[i]!=v){
    unsetTimer(i);
    if(i<_outputPin.size()){
//      if(!ssid[0].length())
      if(isSlave())
        Serial_print("M(" + String(i, DEC) + "):" + (v ?"1" :"0") + "\n");
        //Serial_print("M(" + String(i, DEC) + "):" + (v ?"1" :"0") + (withNoTimer ?":-1\n" :"\n"));
      digitalWrite( _outputPin[i], (outputReverse[i] xor (outputValue[i]=v)) );
      DEBUG_print( "Set GPIO " + String(_outputPin[i], DEC) + "(" + outputName(i) + ") to " + (outputValue[i] ?"true\n" :"false\n") );
    }else if(isMaster()) Serial_print("S(" + String(i-_outputPin.size(), DEC) + "):" + (v ?"1\n" :"0\n"));
    if(isMaster()){
      if(!withNoTimer)
        setTimer(i);
      if(RESTO_VALUES_ON_BOOT)
        writeConfig();
      notifyHTTPProxy(i, "Status-changed");
} } }

void setAllPinSlave(){
  for(ushort i(_outputPin.size()); i<outputCount(); i++)
    Serial_print("S(" + String(i-_outputPin.size(), DEC) + "):" + ((outputValue[i]=((RESTO_VALUES_ON_BOOT || mustResto) ?outputValue[i] :false)) ?"1\n" :"0\n"));
}

void serialSwitchsTreatment(unsigned int serialBufferLen=serialInputString.length()){
  if(serialBufferLen>4){
    if(isMaster()){
      if(serialInputString.startsWith("M(") && serialInputString[3]==')'){        //Setting Master from Slave:
        if(serialInputString[2]=='?'){
          setAllPinSlave();
          DEBUG_print("Slave detected...\n");
        }else if(serialInputString[4]==':'){
          if(serialInputString[5]=='-'){ ushort i=_outputPin.size()+serialInputString[2]-'0';
            unsetTimer(i);
            DEBUG_print( "Timer removed on uart(" + outputName(i) + ")\n");
          }else{ ushort i=_outputPin.size()+serialInputString[2]-'0';
            if(i>=outputCount() && outputName(i).length()) writeConfig();           //Create and save new serial pin(s)
            outputValue[i] = (serialBufferLen>5 && serialInputString[5]=='1'); setTimer(i);
            //if(serialBufferLen>6 && serialInputString[6]==':') unsetTimer(i); else if(outputValue[i]) setTimer(i);
            DEBUG_print( "Set GPIO uart(" + outputName(i) + ") to " + (outputValue[i] ?"true\n" :"false\n") );
        } }
      }else DEBUG_print("Slave says: " + serialInputString + "\n");

    }else if(serialInputString.startsWith("S(") && serialInputString[3]==')'){    //Setting Slave from Master:
      if(serialInputString[2]=='.')
        reboot();
      else if(serialInputString[4]==':' && (ushort)(serialInputString[2]-'0')<_outputPin.size())
        setPin(serialInputString[2]-'0', (serialInputString[5]=='1'), true);
      if(!isSlave()) DEBUG_print("I'm now the Slave.\n");
      isSlave()=true;

    }serialInputString=serialInputString.substring(serialBufferLen);
} }

void mySerialEvent(){char inChar;
  if(Serial) while(Serial.available()){
    serialInputString += (inChar=(char)Serial.read());
    if(inChar=='\n')
         serialSwitchsTreatment();
} }

void timersTreatment(){
  for(ushort i(0); i<outputCount(); i++){
    if(outputValue[i] && isTimer(i) && isNow(timerOn[i])){
      DEBUG_print("Timeout(" + String(maxDuration[i], DEC) + "s) on GPIO " + ((i<_outputPin.size()) ?String(_outputPin[i], DEC) :"uart") + "(" + outputName(i) + "):\n");
      setPin(i, !outputValue[i], outputValue[i]);
      //notifyHTTPProxy(i, "Status-changed on timeout");
} } }

void memoryTest(){
#ifdef MEMORYLEAKS
  if(!isSlave()){        //on WiFi(TCP) errors...
    unsigned long f=ESP.getFreeHeap();
    if(f<MEMORYLEAKS) reboot();
    DEBUG_print("FreeMem: " + String(f, DEC) + "\n");
  }
#endif
}

void connectionTreatment(){                              //Test connexion/Check WiFi every mn:
  if(isNow(next_reconnect)){
    next_reconnect=millis()+WIFISTADELAYRETRY;
    memoryTest();

    if(isSlave()){
      if(WiFiAP) WiFiDisconnect();
      return;
    }else if(Serial && !isMaster())
      Serial_print("M(?)\n");                            //Is there a Master here?...

#ifdef DEFAULTWIFIPASS
    if( (!WiFiAP && !WIFI_STA_Connected()) || (WiFiAP && ssid[0].length() && !WifiAPTimeout--) ){
      if(WiFiConnect())
#ifdef DEBUG
      { telnetServer.begin();
        telnetServer.setNoDelay(true);
        if(!WiFiAP){ bool b=true;
          for(ushort i(0); b&&i<outputCount(); i++)
            b=notifyHTTPProxy(i, getHostname() + " connected.");
    } } }
    else if(telnetServer.hasClient()){                   //Telnet client connection:
      if (!telnetClient || !telnetClient.connected()){
        if(telnetClient){
          telnetClient.stop();
          DEBUG_print("Telnet Client Stop\n");
        }telnetClient=telnetServer.available();
        telnetClient.flush();
        DEBUG_print("New Telnet client connected...\n");
      }
#endif
  ;}MDNS.update();
#endif
} }

void handleSubmitSSIDConf(){                              //Setting:
  ushort count=0;
  for(ushort i(0); i<SSIDCount(); i++) if(ssid[i].length()) count++;
  for(ushort i(0); i<count;     i++)
    if(ssid[i]==ESPWebServer.arg("SSID")){                      //Modify password if SSID exist
      password[i]=ESPWebServer.arg("password");
      if(!password[i].length())                           //Delete this ssid if no more password
        ssid[i]=="";
      return;
    }
  if(count<SSIDCount()){                                  //Add ssid:
    ssid[count]=ESPWebServer.arg("SSID");
    password[count]=ESPWebServer.arg("password");
} }

inline bool handlePlugIdentSubmit(ushort i){               //Set outputs names:
  if(ESPWebServer.hasArg("plugIdent"+(String)i) && ESPWebServer.arg("plugIdent"+String(i, DEC)))
    return true;
  return false;
}

inline bool handlePlugNameSubmit(ushort i){               //Set outputs names:
  if(ESPWebServer.hasArg("plugName"+String(i, DEC)) && ESPWebServer.arg("plugName"+String(i, DEC)))
    if(outputName(i)!=ESPWebServer.arg("plugName"+String(i, DEC)))
      return(outputName(i)=ESPWebServer.arg("plugName"+String(i, DEC)));
  return false;
}

inline bool handleDurationOnSubmit(ushort i){ unsigned int v;        //Set outputs durations:
  if(!ESPWebServer.hasArg(outputName(i)+"-max-duration-s"))
    return false;
  v   =atoi((ESPWebServer.arg(outputName(i)+"-max-duration-s")).c_str());
  if(ESPWebServer.hasArg(outputName(i)+"-max-duration-mn"))
    v+=atoi((ESPWebServer.arg(outputName(i)+"-max-duration-mn")).c_str())*60;
  if(ESPWebServer.hasArg(outputName(i)+"-max-duration-h"))
    v+=atoi((ESPWebServer.arg(outputName(i)+"-max-duration-h")).c_str())*3600;
  if(ESPWebServer.hasArg(outputName(i)+"-max-duration-d"))
    v+=atoi((ESPWebServer.arg(outputName(i)+"-max-duration-d")).c_str())*86400;
  if(maxDuration[i]==(unsigned long)v)
    return false;
  maxDuration[i] = (unsigned long)v;
  return true;
}

inline bool handleValueSubmit(ushort i){//Set outputs values: if param -> 1; else -> 0
  if(ESPWebServer.hasArg(outputName(i)) && outputValue[i]) //already set
    return true;
  if(!ESPWebServer.hasArg("newValue" + String(i, DEC)))
    return false;
  setPin(i, ESPWebServer.hasArg(outputName(i)), !ESPWebServer.hasArg(outputName(i)+"-timer")); // not arg if unchecked...
  return true;
}

#define setMQTT_S(n,m) if(     ESPWebServer.arg(n)         !=m){m=     ESPWebServer.arg(n);         isNew=true;};
#define setMQTT_N(n,m) if(atoi(ESPWebServer.arg(n).c_str())!=m){m=atoi(ESPWebServer.arg(n).c_str());isNew=true;};
bool handleSubmitMQTTConf(ushort n){
  bool isNew(false);
  isNew=(ESPWebServer.hasArg("outputReverse") != outputReverse[n]); outputReverse[n]=ESPWebServer.hasArg("outputReverse");
  setMQTT_S("plugName", outputName(n));
  if((mqttEnable[n]=ESPWebServer.hasArg("mqttEnable"))){ushort i;
    setMQTT_S("mqttBroker", mqttBroker);
    setMQTT_N("mqttPort",   mqttPort  );
    setMQTT_S("mqttIdent",  mqttIdent );
    setMQTT_S("mqttUser",   mqttUser  );
    setMQTT_S("mqttPwd",    mqttPwd   );
    for(i=0; ESPWebServer.hasArg("mqttFieldName"+String(i,DEC)); i++){
      isNew|=addMQTT(n, i);
      setMQTT_S("mqttFieldName"+String(i,DEC), mqttFieldName[n][i]);
      setMQTT_N("mqttNature"   +String(i,DEC), mqttNature[n][i]   );
      setMQTT_N("mqttType"     +String(i,DEC), mqttType[n][i]     );
      setMQTT_S("mqttOnValue"  +String(i,DEC), mqttOnValue[n][i]  );
      setMQTT_S("mqttOffValue" +String(i,DEC), mqttOffValue[n][i] );
    }for(mqttEnable[n]=i; i<mqttFieldName[n].size(); i++){isNew=true;
      mqttFieldName[n].pop_back();
      mqttNature[n].pop_back();
      mqttType[n].pop_back();
      mqttOnValue[n].pop_back();
      mqttOffValue[n].pop_back();
  } }
 return isNew;
}

void  handleRoot(){ bool w, blankPage=false;
  if(ESPWebServer.hasArg("Clear")){                      //Reboot device(s)...
    for(ushort i(_outputPin.size()); i<outputCount(); i++){
      unsetTimer(i); outputValue[i]=0;
    }clearOutputCount(); reboot();
  }if((w=ESPWebServer.hasArg("hostname"))){
    hostname=ESPWebServer.arg("hostname");                //Set host name
    reboot();
  }else if((w=ESPWebServer.hasArg("password"))){
    handleSubmitSSIDConf(); shiftSSID();                  //Set WiFi connections
    if(WiFiAP && ssid[0].length()) WiFiDisconnect();
  }else if(ESPWebServer.hasArg("plugNum")){
    w|=handleSubmitMQTTConf(atoi(ESPWebServer.arg("plugNum").c_str()));
  }else{
    for(ushort i(0); i<outputCount(); i++)
      w|=handlePlugIdentSubmit(i);                        //Set plug name
    if(!w) for(ushort i(0); i<outputCount(); i++)
      w|=handlePlugNameSubmit(i);                         //Set plug name
    if(!w) for(ushort i(0); i<outputCount(); i++)
      w|=handleDurationOnSubmit(i);                       //Set timeouts
    if(!w){
      for(ushort i=(0); i<outputCount(); i++){            //Set values
        blankPage|=handleValueSubmit(i);
  } } }
  if(w) writeConfig();
  mySerialEvent();
  sendHTML(blankPage);
}

String getPlugNames(){
  String s="";
  for(ushort i(0); outputCount(); ){
    s += "\"" + outputName(i) + "\"";
    if((++i)>=outputCount()) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void  setPlugNames(){
  String v;
  for(ushort i(0); i<outputCount(); i++){
    v=outputName(i); v.toLowerCase();
    if ((v=ESPWebServer.arg(v))!=""){
      v.toLowerCase();
      outputName(i) = v;
} } }

String getPlugTimers(){
  String s="";
  for(ushort i(0); outputCount(); ){
    s += "\"" + String(maxDuration[i], DEC) + "\"";
    if((++i)>=outputCount()) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void   setPlugTimers(){
  String v;
  for(ushort i(0); i<outputCount(); i++){
    v=outputName(i); v.toLowerCase();
    if ((v=ESPWebServer.arg(v))!=""){
      v.toLowerCase();
      timerOn[i] = (v=="-1" ?-1L :(millis() + atol(v.c_str()) * 1000L));
      DEBUG_print("Timer(" + outputName(i) + "): " + (v=="-1" ?v :String(timerOn[i], DEC)) + "\n");
} } }

String getPlugValues(){
  String s="";
  for(ushort i(0); outputCount(); ){
    s += "\"" + String(outputValue[i] ?"1" :"0") + "\"";
    if((++i)>=outputCount()) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void setPlugValues(){
  String v;
  for(ushort i(0); i<outputCount(); i++){
    v=outputName(i); v.toLowerCase();
    if ((v=ESPWebServer.arg(v))!=""){
      bool val;
      v.toLowerCase(); val=((v=="1" || v=="true") ?1 :0);
      DEBUG_print("HTTP request on GPIO " + outputName(i) + "...\n");
      setPin(i, val, !val);
} } }

bool notifyHTTPProxy(ushort n, String msg){
#ifdef DEFAULT_MQTT_SERVER
  if(!mqttEnable[n]) return true;
  if(mqttBroker.length()){
    if(!mqttClient.connected())
      mqttClient.connect(mqttIdent.c_str(), mqttUser.c_str(), mqttPwd.c_str());
    if(mqttClient.connected()){
      String  s ="{\n";
      for(ushort i(0);;){
        s+=" \"" + mqttFieldName[n][i] + "\": ";
        if(mqttType[n][i]==0) s+= "\"";
        if(mqttNature[n][i])
              s+= mqttOnValue[n][i];
        else  s+= (outputValue[n] ?mqttOnValue[n][i] :mqttOffValue[n][i]);
        if(mqttType[n][i]==0) s+= "\"";
        if(!(++i<(mqttEnable[n]))) break;
        s+=",\n";
      }s+="\n}\n";
      mqttClient.publish(mqttQueue.c_str(), s.c_str());
      DEBUG_print("'" + msg + "' published to \"" + mqttBroker + "\".\n");
      return true;
    }else DEBUG_print("'" + msg + "': MQTT server \"" + mqttBroker + ":" + String(mqttPort,DEC) + "\" not found...\n");
  }
#else
#ifdef NOTIFYPROXY
  if(WIFI_STA_Connected()){
#ifdef NOTIFYPORT
    ushort port(NOTIFYPORT ?NOTIFYPORT :8081);
#else
    ushort port(8081);
#endif
    WiFiClient notifyClient;
    if(notifyClient.connect(NOTIFYPROXY, port))
      notifyClient.flush();
    else
      DEBUG_print("Notify server \"" + String(NOTIFYPROXY) + ":" + String(port, DEC) + "\" not found...\n");
    if (notifyClient && notifyClient.connected()){
      String s ="\n{\n";
             s+="  \"id\": \"" + getHostname() + "\",\n";
             s+="  \"names\": [" + getPlugNames() + "],\n";
             s+="  \"values\": [" + getPlugValues() + "],\n";
             s+="  \"msg\": \"" + msg + "\"\n";
             s+="}\n\n";
      // Make a HTTP request:
      notifyClient.println("POST / HTTP/1.1");
      notifyClient.println("Host: " + getHostname());
      notifyClient.println("Content-Type: application/json");
      notifyClient.println("Content-Length: " + String(s.length(), DEC));
      notifyClient.println(s);
      notifyClient.flush();
      notifyClient.stop();
      DEBUG_print("'" + msg + "' notified to \"" + String(NOTIFYPROXY) + ":" + String(port, DEC) + "\".\n");
      return true;
  } }
#endif
#endif
  return false;
}

//Gestion des switchs/Switchs management
void ICACHE_RAM_ATTR debouncedInterrupt(){if(!intr) {intr--;rebounds_completed=millis()+DEBOUNCE_TIME;}}

void interruptTreatment(){
  if (intr && isNow(rebounds_completed)){               //switch activated...
    uint16_t reg=GPI; ushort n=0;
    for(ushort i(0); i<inputCount(); i++) if( (reg&(1<<_inputPin[i]))==0 ) n+=(1<<i);
    if (intr<0){                                        //the switch has just switched.
      intr=n;
      DEBUG_print("\nIO init: "); for(ushort i(inputCount()); i; i--) DEBUG_print(n&(1<<(i-1)) ?1 :0); DEBUG_print("\n");
      rebounds_completed=millis()+DEBOUNCE_TIME;
    }else if(!n){                                       //Switch released...
      DEBUG_print("IO : "); for(ushort i(inputCount()); i; i--) DEBUG_print(1<<(i-1)); DEBUG_print("\n");
      DEBUG_print("GPI: "); for(ushort i(inputCount()); i; i--) DEBUG_print(intr&(1<<(i-1)) ?1 :0); DEBUG_print("\n");
      if(ushort(--intr)<_outputPin.size()){
        if((unsigned long)(millis()-rebounds_completed)>DISABLESWITCHTIMEOUT){
          unsetTimer(intr);
          if(isSlave())
            Serial_print("M(" + String(intr, DEC) + "):-1\n");
          DEBUG_print( "Timer removed on " + String(_inputPin[intr], DEC) + "(" + outputName(intr) + ")\n");
        }else setPin(intr, !outputValue[intr], outputValue[intr]);
      }intr=0;
    }else if(n!=intr){
      intr=0;
      DEBUG_print("\nIO ERROR.\n"); for(ushort i(inputCount()); i; i--) DEBUG_print(1<<(i-1));
} } }

// ***********************************************************************************************
// **************************************** SETUP ************************************************
void setup(){
  Serial.begin(115200);
  while(_outputName.size()<_outputPin.size()) addSwitch();
  while(!Serial);
  Serial_print("\n\nChipID(" + String(ESP.getChipId(), DEC) + ") says:\nHello World!\n\n");

  //initialisation des broches /pins init
  readConfig();
  for(ushort i(0); i<inputCount(); i++){    //Entrées/inputs:
    if(_inputPin[i]==3 || _inputPin[i]==1) Serial.end();
    //pinMode(_inputPin[i], INPUT);     //only FALLING mode works on all inputs !...
    pinMode(_inputPin[i], INPUT_PULLUP);     //only FALLING mode works on all inputs !...
    //See: https://www.arduino.cc/en/Reference/attachInterrupt
    // or: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
    attachInterrupt(_inputPin[i], debouncedInterrupt, FALLING);
  }for(ushort i(0); i<_outputPin.size(); i++){    //Sorties/ouputs:
      pinMode(_outputPin[i], OUTPUT);
      digitalWrite(_outputPin[i], (outputReverse[i] xor (outputValue[i]=((RESTO_VALUES_ON_BOOT || mustResto) ?outputValue[i] :false))));
      if(_outputPin[i]==3 || _outputPin[i]==1) Serial.end();
  }if(mustResto){
    mustResto=false;
    writeConfig();
  }

  if(Serial){
    serialInputString.reserve(32);
    if(ssid[0].length()){
      isMaster()=true;
      setAllPinSlave();
  } }

  // Servers:
  WiFi.softAPdisconnect(); WiFi.disconnect();
  //Definition des URLs d'entree /Input URL definitions
  ESPWebServer.on("/",           [](){handleRoot();    ESPWebServer.client().stop();});
  ESPWebServer.on("/plugNames",  [](){setPlugNames();  ESPWebServer.send(200, "text/plain", "[" + getPlugNames()  + "]");});
  ESPWebServer.on("/plugTimers", [](){setPlugTimers(); ESPWebServer.send(200, "text/plain", "[" + getPlugTimers() + "]");});
  ESPWebServer.on("/plugValues", [](){setPlugValues(); ESPWebServer.send(200, "text/plain", "[" + getPlugValues() + "]");});
  ESPWebServer.on("/restart",    [](){reboot();});
//ESPWebServer.on("/about",      [](){ ESPWebServer.send(200, "text/plain", getHelp()); });
  ESPWebServer.onNotFound([](){ESPWebServer.send(404, "text/plain", "404: Not found");});

  httpUpdater.setup(&ESPWebServer);  //Adds OnTheAir updates:
  ESPWebServer.begin();              //Demarrage du serveur web /Web server start
  MDNS.begin(getHostname().c_str());
  MDNS.addService("http", "tcp", 80);
  Serial_print("HTTP server started\n");

  if(mqttBroker.length())
    mqttClient.setServer(mqttBroker.c_str(), mqttPort);
}

// **************************************** LOOP *************************************************
void loop(){
  ESPWebServer.handleClient(); delay(1L);

  connectionTreatment();                //WiFi watcher
  interruptTreatment();                 //Gestion des switchs/Switchs management
  timersTreatment();                    //Timers control

  mySerialEvent();                      //Serial communication for Slave messages traitement
}
// ***********************************************************************************************
