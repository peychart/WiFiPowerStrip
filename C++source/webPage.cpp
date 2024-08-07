/* ESP8266-WEB-Manager C++ (Version 0.1 - 2020/07)
    <https://github.com/peychart/WiFiPowerStrip>

    Copyright (C) 2020  -  peychart

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this program.
    If not, see <http://www.gnu.org/licenses/>.

    Details of this licence are available online under:
                        http://www.gnu.org/licenses/gpl-3.0.html
*/
#include "webPage.h"

void setupWebServer(){
  //Definition des URLs d'entree /Input URL definitions
  ESPWebServer.on(F("/")                 ,[](){ handleRoot(); });
  ESPWebServer.on(F("/" ROUTE_PIN_STATE) ,[](){ configDeviceFromJS(); sendToJS( getDeviceConfig().c_str() ); });
  ESPWebServer.on(F("/" ROUTE_PIN_GPIO)  ,[](){ configDeviceFromJS(); sendToJS( getDeviceStatus().c_str() ); });
  ESPWebServer.on(F("/" ROUTE_RESET)     ,[](){ ESPWebServer.send(200, F("text/html"), F("<meta http-equiv='refresh' content='15 ;URL=/'/>Rebooting...\n")); reboot(); });
  ESPWebServer.on(F("/" ROUTE_HTML_CODE) ,[](){ handleRoot(false); });
//ESPWebServer.on(F("/about")            ,[](){ ESPWebServer.send(200, F("text/plain"), getHelp()); });
  ESPWebServer.onNotFound([](){ ESPWebServer.send(404, F("text/plain"), F("404: Not found")); });
  ESPWebServer.begin();
  DEBUG_print(F("HTTP server started\n"));
}

void sendToJS( String s ){
  ESPWebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  ESPWebServer.send( 200, F("application/json"), "" );
  ESPWebServer.sendContent( s.c_str(), s.length() );
  ESPWebServer.sendContent("");
  ESPWebServer.client().flush();
}

std::string getDeviceConfig(){
  untyped b; std::stringstream o; o.clear();
  b[G(ROUTE_VERSION)]           = myWiFi.version();
  b[G(ROUTE_UPTIME)]            = millis();
  b[G(ROUTE_HOSTNAME)]          = myWiFi.hostname();
  b[G(ROUTE_IP_ADDR)]           = ( myWiFi.apConnected() ?WiFi.softAPIP().toString().c_str() :WiFi.localIP().toString().c_str() );
  b[G(ROUTE_MAC_ADDR)]          = WiFi.macAddress().c_str();
  b[G(ROUTE_CHIP_IDENT)]        = STDSTR(ESP.getChipId());
  b[G(ROUTE__DEFAULT_HOSTNAME)] = (G(DEFAULTHOSTNAME "-")+String(ESP.getChipId())).c_str();
  b[G(ROUTE__DEFAULT_PASSWORD)] = DEFAULTWIFIPASS;
  for(size_t i=0; i<myWiFi.ssidCount(); i++)
    b[(ROUTE_WIFI_SSID)][i]     = myWiFi.ssid(i);
  for(auto &x: myPins)
    b[G("pinOrder")][b[G("pinOrder")].vectorSize()]=x.gpio();
#ifdef DEFAULT_MQTT_BROKER
  b[G(ROUTE_MQTT_BROKER)]       = myMqtt.broker();
  b[G(ROUTE_MQTT_PORT)]         = myMqtt.port();
  b[G(ROUTE_MQTT_IDENT)]        = myMqtt.ident();
  b[G(ROUTE_MQTT_USER)]         = myMqtt.user();
  b[G(ROUTE_MQTT_PWD)]          = G("******"); //myMqtt.password();
  b[G(ROUTE_MQTT_PUBTOPIC)]     = myMqtt.pubTopic();
#endif
#ifdef DEFAULT_NTPSOURCE
  b[G(ROUTE_NTP_SOURCE)]        = myNTP.source();
  b[G(ROUTE_NTP_ZONE)]          = myNTP.zone();
  b[G(ROUTE_NTP_DAYLIGHT)]      = myNTP.dayLight();
#endif
  b.serializeJson(o).clear().assign(o.str()); o.clear(); //No "o.clear()" -> More Random Memory fault...
  return( b.c_str() );
}

std::string getDeviceStatus(){
  untyped b; std::stringstream o; o.clear();
  for(auto &x: myPins){
    b[G(ROUTE_PIN_GPIO)][STDSTR(x.gpio())][G(ROUTE_PIN_NAME)]        = x.name();
    b[G(ROUTE_PIN_GPIO)][STDSTR(x.gpio())][G(ROUTE_PIN_STATE)]       = x.isOn();
    b[G(ROUTE_PIN_GPIO)][STDSTR(x.gpio())][G(ROUTE_PIN_REVERSE)]     = x.reverse();
    if(x.timeout()==-1UL)
          b[G(ROUTE_PIN_GPIO)][STDSTR(x.gpio())][G(ROUTE_PIN_VALUE)] = -1L;
    else  b[G(ROUTE_PIN_GPIO)][STDSTR(x.gpio())][G(ROUTE_PIN_VALUE)] = x.timeout();
  }DEBUG_print(F("FreeMem: ")); DEBUG_print(ESP.getFreeHeap()); DEBUG_print(F("\n"));
  b.serializeJson(o).clear().assign(o.str()); o.clear(); //No "o.clear()" -> More Random Memory fault...
  return( b.c_str() );
}

void configDevice( std::string json ) {
  untyped b; b.deserializeJson( json.c_str() );
  DEBUG_print(F("JSON Request received: \"")); DEBUG_print(json.c_str()); DEBUG_print(F("\"\n"));
  myWiFi.set(b).saveToSD();
#ifdef DEFAULT_MQTT_BROKER
  myMqtt.set(b).saveToSD();
#endif
  myPins.set(b).saveToSD();
#ifdef DEFAULT_NTPSOURCE
  myNTP.set(b).saveToSD();
#endif
}

void configDeviceFromJS() {
  for(int i(0); i<ESPWebServer.args(); i++ ){
    configDevice( ESPWebServer.argName(i).c_str() );
} }

void handleRoot( bool active ) {
  ESPWebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  ESPWebServer.send( 200, active ?F("text/html") :F("text/plain"), F("<!DOCTYPE HTML>\n<html lang='us-US'>\n") );
#ifdef EXTERN_WEBUI
  ESPWebServer.sendContent( HTML_redirHeader() );
  if( active )
#else
  ESPWebServer.sendContent( HTML_Header() );
  if( false )
#endif
    ESPWebServer.sendContent( F("<body>\nLoading...\n") );
  else{
#ifndef EXTERN_WEBUI
    ESPWebServer.sendContent( F("<body onload='init();'>\n") );
    ESPWebServer.sendContent( HTML_AboutPopup() );
    ESPWebServer.sendContent( HTML_MainForm() );
    ESPWebServer.sendContent( HTML_ConfPopup() );
    ESPWebServer.sendContent( F("<!-==========JScript==========->\n") );
    ESPWebServer.sendContent( F("<script>this.timer=0;parameters={'" ROUTE_IP_ADDR "':'") );
    ESPWebServer.sendContent( (active ?(myWiFi.apConnected() ?WiFi.softAPIP().toString() :WiFi.localIP().toString()) :String("")) + G("'};\n") );
#ifdef DEFAULT_MQTT_BROKER
    ESPWebServer.sendContent( ("parameters." ROUTE_MQTT_BROKER   "='" + myMqtt.broker()       + "';").c_str() );
    ESPWebServer.sendContent( ("parameters." ROUTE_MQTT_PORT     "="  + STDSTR(myMqtt.port()) + ";" ).c_str() );
    ESPWebServer.sendContent( ("parameters." ROUTE_MQTT_USER     "='" + myMqtt.user()         + "';").c_str() );
    ESPWebServer.sendContent( ("parameters." ROUTE_MQTT_PWD      "='" + myMqtt.password()     + "';").c_str() );
    ESPWebServer.sendContent( ("parameters." ROUTE_MQTT_IDENT    "='" + myMqtt.ident()        + "';\n").c_str() );
    ESPWebServer.sendContent( ("parameters." ROUTE_MQTT_PUBTOPIC "='" + myMqtt.pubTopic()     + "';\n").c_str() );
    for(short i=0; i<(short)myMqtt.subscribe().size(); i++)
      ESPWebServer.sendContent( ((i?",\"":"subscribeTopic=[\"") + myMqtt.subscribe().at(i)+ ((i==(short)myMqtt.subscribe().size()-1)?"\"];\n":"\"")).c_str() );
#endif
    ESPWebServer.sendContent( HTML_JRefresh() );
    ESPWebServer.sendContent( HTML_JSubmits() );
    ESPWebServer.sendContent( HTML_JMainDisplay() );
    ESPWebServer.sendContent( HTML_JSSIDDisplay() );
    ESPWebServer.sendContent( HTML_JMQTTDisplay() );
    ESPWebServer.sendContent( F("</script>\n") );
//#ifdef DEFAULT_MQTT_BROKER
//    ESPWebServer.sendContent( F("<script src='https://cdnjs.cloudflare.com/ajax/libs/paho-mqtt/1.0.2/mqttws31.min.js' type='text/javascript'></script>\n") );
//#endif
#endif
  }ESPWebServer.sendContent( F("</body>\n</html>\n\n") );
  ESPWebServer.sendContent("");
  ESPWebServer.client().flush();
  ESPWebServer.client().stop();
}

#ifdef EXTERN_WEBUI
String HTML_redirHeader(){
  String ret;
  ret += (F("<head>\n<meta http-equiv='refresh' content='0 ;URL="));
  ret += (EXTERN_WEBUI);
  ret += F("?ip=");
  ret += (myWiFi.apConnected() ?WiFi.softAPIP() :WiFi.localIP()).toString();
  ret += F("'/>\n<title id=title name='hostname'>ESP8266</title>\n</head>\n");
  return ret;
}

#else
const __FlashStringHelper* HTML_AboutPopup(){ return(F("\
<div id='about' class='modal'><div class='modal-content'><span class='close' onclick='refresh();'>&times;</span><h1>About</h1>\
This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domoticz or Jeedom.<br><br>\
In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). \
Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs, like this:\
<a id='example1' style='padding:0 0 0 5px;'></a><br><br>\
The state of the electrical outlets can also be requested from the following URL: <a id='example2' style='padding:0 0 0 5px;'></a>.\
 In addition, a MQTT server can be notified of each of the state changes in order, for example, to relay the state of the switches (on manual action) to the centralized home automation interface.<br><br>\
The status of the power strip can be saved when the power is turned off and restored when it is turned on ; a power-on duration can be set on each output: (-1) no delay, (0) to disable a specific output and (number of s) to configure the power-on duration (but this timer can be temporarily disabled by holding the switch for 3s).<br><br>\
A second slave module (without any declared WiFi SSID) can be connected to the first (which thus becomes master by automatically detecting its slave) through its UART interface (USB link) in order to increase the number of outputs to a maximum of 12 on the same management interface. \
The manual action of these additional physical switches adds them automatically to the web interface (on refresh). The \"clear\" button can be used to delete them when removing the slave module.<br><br>\
The following allows you to configure some parameters of the Wifi Power Strip (as long as no SSID is set and it is not connected to a master, the device acts as an access point with its own SSID and default password: '<span id='" ROUTE__DEFAULT_HOSTNAME "'>" DEFAULTHOSTNAME "/" DEFAULTWIFIPASS "</span>' on 192.168.4.1).<br><br>\n\
<table style='width:100%;'>\n\
<tr><th align='left' width='150px'><h3>ESP8266</h3></th>\n\
<th align='center'><h3 id='ntpLib' style='display:none;'>NTP Server - TZone - daylight</h3></th>\n\
<th align='center' width='120px'><h3>Clear serial</h3></th></tr>\n\
<tr style='white-space:nowrap;'><td style='text-align:center;'>\n\
<input id='hostname' type='text' value='ESP8266' style='width:100px' maxlength=20 pattern='^[a-zA-Z][a-zA-Z0-9-]*$' onchange='checkHostname(this);'>\n\
<input id='hostnameSubmit' type='button' value='Submit' disabled onclick='hostnameSubmit();'>\n\
</td><td style='text-align:center;display:online-block;'><div id='ntp' style='display:none;'>\n\
<input id='ntpSource' type='text' pattern='^[a-z0-9]*\\.[a-z0-9][a-z0-9\\.]*$' value='fr.pool.ntp.org' style='width:200px' onchange='checkNTP(this);'>\n\
&nbsp;<input id='ntpZone' type='number' value='-10' min=-11 max=11 size=2 style='width:40px' onchange='checkNTP(this);'>\n\
&nbsp;&nbsp;<input id='ntpDayLight' type='checkbox' onclick='checkNTP(this);'>\n\
&nbsp;&nbsp;<input id='ntpSubmit' type='button' value='Submit' disabled onclick='ntpSubmit(this);'></div>\n\
</td><td style='text-align:center;'>\n\
<input id='Clear' type='button' value='Restart' onclick='restartDevice();'>&nbsp;\n\
</td></tr>\n</table>\n\
<br><h3>Network connection [<span id='macAddr'>00:00:00:00:00:00</span>] (device ident=<span id='chipIdent'>Ident</span>): \n\
<input type='button' value='+' onclick='addSSID();'>\n\
</h3>\n\
<div><div style='overflow-x:auto;'><table style='width:100%;'><tr id='ssids'></tr></table></div></div>\n\
<h6><a href='update' onclick='javascript:event.target.port=80'>Firmware update</a> - <a href='https://github.com/peychart/WiFiPowerStrip'>Website here</a></h6>\n\
</div></div>\n\n\
"));}

const __FlashStringHelper* HTML_Header(){ return(F("\
<head>\n<meta charset='utf-8'/>\n<title id=title name='hostname'>ESP8266</title>\n<style>\n\
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
</style>\n</head>\n\
"));}

const __FlashStringHelper* HTML_MainForm(){ return(F("\
<!-============MAIN FORM============->\n\
<table style='width:100%;'>\n<tr style='height:75px;'>\n\
<td style='width:800px;'><h1><span name='hostname'>ESP8266</span> - <span id='ipAddr'>255.255.255.255</span></h1></td>\n\
<td style='text-align:right;vertical-align:top;'><div style='text-align:right;white-space: nowrap;'><p><span class='close' onclick='showHelp();'>?</span></p></div></td>\n\
</tr></table>\n\
<h3>Status :</h3><table id='switches' style='width:100%;'></table>\n<h6>(V<span id='version'></span>, Uptime: <span id='uptime'></span>)</h6>\n\n\
"));}

const __FlashStringHelper* HTML_ConfPopup(){ return(F("\
<!-============Config POPUP->\n\
<div id='confPopup' class='confPopup'><div id='mqttConf'>\n\
<input id='pinNumber' type='text' style='display:none;'>\n\
<a title='Save configuration' class='closeconfPopup' onclick='closeConfPopup();'>X</a>\n\
<table style='width:100%;'><col width=90%><tr><td><div style='text-align:center;'><h2>'<span id='confName'></span>' configuration:</h2></div></td></tr></table>\n\
<h3 title='for this switch'>Output parameters</h3>\n\
<table title='for this switch' style='width:100%;'>\n<col width='50%'>\n<tr>\n\
<td style='text-align:center;'>Plug Name<br><input id='plugName' type='text' pattern='^[a-zA-Z][a-zA-Z0-9]*$' style='width:150px;' onchange='plugnameSubmit(this);'>\n\
</td>\n<td align=center>Reverse level<br><input id='outReverse' type='checkbox' style='vertical-align:middle;' onclick='reverseSubmit(this);'>\n\
</td>\n</tr></table>\n<h3 title='for this switch'>MQTT parameters</h3>\n\
<!-=====and MQTT configuration:->\n\
<div id='mqttParams'><p align=center title='for all switches'>Broker: <input id='mqttBroker' type='text' pattern='^[a-zA-Z][a-zA-Z0-9/\\.]*$' style='width:65%;' onchange='mqttBrokerSubmit(this);'>\n\
:<input id='mqttPort' type='number' min='0' max='65535' style='width:10%;' onchange='mqttPortSubmit(this);'>\n\
</p>\n<table style='width:100%;'>\n<col width='42%'><col width='30%'><tr title='for all switches' style='white-space: nowrap;'><td>\n\
Identification: <input id='mqttIdent' type='text' pattern='^[a-zA-Z][a-zA-Z0-9]*$' style='width:120px;' onchange='mqttIdentSubmit(this);'>\n\
</td><td>\nUser: <input id='mqttUser' type='text' pattern='^[a-zA-Z][a-zA-Z0-9]*$' style='width:120px;' onchange='mqttUserSubmit(this);'>\n\
</td><td>\nPassword: <input id='mqttPwd' type='password' style='width:75px;' onchange='mqttPwdSubmit(this);'>\n\
</td></tr></table>\n<p align=center title='for all switches'>Topic: <input id='mqttOutTopic' type='text' style='width:80%;' onchange='mqttOutTopicSubmit(this);'>\n\
</p>\n</div></div></div>\n\n\
"));}

const __FlashStringHelper* HTML_JRefresh( void ){
#ifdef DEFAULT_MQTT_BROKE_R
  return(F("\
//======Refresh:\n\
function startConnect(){\n\
// mqttClient=new Paho.MQTT.Client(parameters." ROUTE_MQTT_BROKER ",Number(parameters." ROUTE_MQTT_PORT "),parameters." ROUTE_MQTT_IDENT ");\n\
 mqttClient=new Paho.MQTT.Client(parameters." ROUTE_MQTT_BROKER ",Number(9001),parameters." ROUTE_MQTT_IDENT ");\n\
 mqttClient.onConnectionLost = onConnectionLost;\n\
 mqttClient.onMessageArrived = onMessageArrived;\n\
 mqttClient.connect({\n\
  onSuccess: onConnect\n\
// userName: parameters." ROUTE_MQTT_USER ",\n\
// passwordId: parameters." ROUTE_MQTT_PWD "\n\
 });\n}\n\
function onConnect(){for(var i=0; i<subscribeTopic.length; i++) mqttClient.subscribe(subscribeTopic[i]);}\n\
function onConnectionLost(responseObject){\n\
 clearTimeout(this.timer); this.timer=setTimeout(function(){if(!mqttClient.isConnected()) onConnectionLost(responseObject);}, 30000);\n}\n\
function onMessageArrived(message){\n\
//for(var a in message.payloadString) parameters[a]=message.payloadString[a];\n\
 JSON.parse(message.payloadString);\n\
 if(message.payloadString." ROUTE_PIN_GPIO ") displayNTP();else refreshSwitches();\n}\n\
function startDisconnect(){mqttClient.disconnect();}\n\
function publishMessage(topic, msg){var Message=new Paho.MQTT.Message(msg);Message.destinationName=topic;mqttClient.send(Message);}\n\
function refresh(v=30){clearTimeout(this.timer);document.getElementById('about').style.display='none';}\n\
function RequestDevice(url){publishMessage(parameters." ROUTE_MQTT_PUBTOPIC " + '/', url);}\n\
function getGpioParam(name,i){return(parameters." ROUTE_PIN_GPIO "[parameters.pinOrder[i]][name]);}\n\
function init(){startConnect(); RequestDevice('" ROUTE_PIN_STATE "'); refresh(1);}\n\n\
"));
#else
  return(F("\
//======Refresh:\n\
function refresh(v=30){\nclearTimeout(this.timer);document.getElementById('about').style.display='none';\n\
 if(v>0)this.timer=setTimeout(function(){RequestDevice('" ROUTE_PIN_GPIO "');refresh();},v*1000);\n}\n\
function RequestDevice(url){var req=new XMLHttpRequest();\n\
 if(!parameters." ROUTE_IP_ADDR ".length){var url=new URL(window.document.location);var p=url.searchParams.get('ip'); if(p.length) parameters." ROUTE_IP_ADDR "=p;}\n\
 req.open('POST',location.protocol+'//'+parameters." ROUTE_IP_ADDR "+'/'+url); req.responseType='json'; req.send();\n\
 if(url=='" ROUTE_PIN_STATE "') req.onload=function(){for(var a in req.response) parameters[a]=req.response[a]; displayNTP();}\n\
 else req.onload=function(){for(var a in req.response) parameters[a]=req.response[a]; refreshSwitches();}\n}\n\
function getGpioParam(name,i){return(parameters." ROUTE_PIN_GPIO "[parameters.pinOrder[i]][name]);}\n\
function init(){RequestDevice('" ROUTE_PIN_STATE "');refresh(1);}\n\n\
"));
#endif
}

const __FlashStringHelper* HTML_JSubmits(){ return(F("\
//===========Submits:\n\
function showHelp(){var v,e=document.getElementById('example1');\n\
 //e.innerHTML=location.protocol+'//'+parameters." ROUTE_IP_ADDR "+'/plugValues?';\n\
 e.innerHTML=location.protocol+'//'+location.host+'/plugValues?';\n\
 v=document.getElementById('switch0');e.innerHTML+=v.name+'='+(v.checked?'true':'false');\n\
 for(var i=1; i<3 &&(v=document.getElementById('switch'+i));i++)e.innerHTML+='&'+v.name+'='+(v.checked?'true':'false');e.href=e.innerHTML;\n\
 e=document.getElementById('example2');\n\
 //e.innerHTML=location.protocol+'//'+parameters." ROUTE_IP_ADDR "+'/plugValues';\n\
 e.innerHTML=location.protocol+'//'+location.host+'/plugValues';\n\
 e.href=e.innerHTML;\n\
 refresh(120);document.getElementById('about').style.display='block';\n\
}\n\
function ssidPwFocus(e){var f;\n\
 for(f=e;f.tagName!='TABLE';)f=f.parentNode;f.querySelectorAll('input[type=button]')[0].disabled=false;e.value='';\n\
}\n\
function ssidSubmit(e){var f;for(f=e;f.tagName!='TABLE';)f=f.parentNode;e=f.querySelectorAll('input[type=text]');\n\
 if(e[0].value==''){alert('Empty SSID...');e[0].focus();}\n\
 else{var p=f.querySelectorAll('input[type=password]');\n\
  if(p[0].value.length<6){alert('Incorrect password...');p[0].focus();}\n\
  else{\n\
   RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_WIFI_SSID "\":[\"'+e[0].value+'\"],\"" ROUTE_WIFI_PWD "\":[\"'+p[0].value+'\"]}');\n\
   e[0].readOnly=true;p[0].value='************';\n\
   p=f.querySelectorAll('input[type=button]'); p[0].disabled=!(p[1].disabled=false);\n\
}}}\n\
function deleteSSID(e){var f;for(f=e;f.tagName!='TABLE';)f=f.parentNode;\n\
 e=f.querySelectorAll('input[type=text]');\n\
 if(e[0].value!='' && confirm('Are you sure to remove this SSID?')){\n\
  var p=f.querySelectorAll('input[type=password]');\n\
  RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_WIFI_SSID "\":[\"'+e[0].value+'\"],\"" ROUTE_WIFI_PWD "\":[\"\"]}');\n\
  e[0].value=p[0].value='';e[0].readOnly=false;p=f.querySelectorAll('input[type=button]'); p[0].disabled=!(p[1].disabled=true);\n\
}}\n\
function checkHostname(e){\n\
 if(e.value.length && (e.value && e.value!=document.getElementById('title').value))\n\
  document.getElementById('hostnameSubmit').disabled=false;\n\
 else document.getElementById('hostnameSubmit').disabled=true;\n\
}\n\
function setHostName(n){var v=document.getElementsByName('hostname');\n\
 for(var i=0;i<v.length;i++) if(v[i].value) v[i].value=n;else v[i].innerHTML=n;\n\
}\n\
function hostnameSubmit(){var e=document.getElementById('hostname');\n\
 setHostName(e.value);document.getElementById('hostnameSubmit').disabled=true;\n\
 RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_HOSTNAME "\":\"'+e.value+'\"}');\n\
}\n\
function displayNTP(){if(parameters.ntp && parameters." ROUTE_NTP_SOURCE "!='')document.getElementById('ntpLib').style=document.getElementById('ntp').style='display:inline-block;'}\n\
function checkNTP(e){\n\
 if(document.getElementById('ntpSource').value!=parameters.source || document.getElementById('ntpZone').value!=parameters.zone || document.getElementById('ntpDayLight').checked!=parameters.dayLight)\n\
  document.getElementById('ntpSubmit').disabled=false;else document.getElementById('ntpSubmit').disabled=true;\n\
}\n\
function ntpSubmit(e){var v;\n\
 v ='\"" ROUTE_NTP_SOURCE   "\":\"'+document.getElementById('ntpSource').value+'\",';\n\
 v+='\"" ROUTE_NTP_ZONE     "\":'+document.getElementById('ntpZone').value+',';\n\
 v+='\"" ROUTE_NTP_DAYLIGHT "\":'+document.getElementById('ntpDayLight').value;\n\
 RequestDevice('" ROUTE_PIN_GPIO "?{'+v+'}');\n\
 e.disabled=true;\n\
}\n\
function switchSubmit(e){var t,b=false,pin;\n\
 for(t=e;t.tagName!='TR';)t=t.parentNode;t=t.getElementsByTagName('input');\n\
 for(var i=0;i<t.length;i++)if(t[i].type=='number')b|=Number(t[i].value); // <--Check if delay!=0\n\
 if(b){var req;\n\
  req='" ROUTE_PIN_GPIO "?{\"" ROUTE_PIN_GPIO "\":{\"'+parameters.pinOrder[e.id.replace('switch','')]+'\":{\"" ROUTE_PIN_STATE "\":'+e.checked;\n\
  if(e.checked && !document.getElementById(e.id+'-timer').checked) req+=',\"" ROUTE_PIN_ENABLED "\":false';\n\
  RequestDevice(req+'}}}');\n\
}}\n\
function displayDelay(v,i){var e,b=false;v/=1000;\n\
 (e=document.getElementById('switch'+i+'-d-duration')).value=Math.trunc(v/86400);\n\
 if(e.value==0) document.getElementById('switch'+i+'days-duration').style='display:none;';\n\
 else {b=true;document.getElementById('switch'+i+'days-duration').style='display:inline-block;';}\n\
 (e=document.getElementById('switch'+i+'-h-duration')).value=Math.trunc(Math.trunc(v%86400)/3600);\n\
 if(!b&&e.value==0) document.getElementById('switch'+i+'hours-duration').style='display:none;';\n\
 else {b=true;document.getElementById('switch'+i+'hours-duration').style='display:inline-block;';}\n\
 (e=document.getElementById('switch'+i+'-mn-duration')).value=Math.trunc(Math.trunc(v%3600)/60);\n\
 if(!b&&e.value==0) document.getElementById('switch'+i+'minutes-duration').style='display:none;';\n\
 else document.getElementById('switch'+i+'minutes-duration').style='display:inline-block;';\n\
 (e=document.getElementById('switch'+i+'-s-duration')).value=(v>0?Math.trunc(v%60):-1);\n\
}\n\
checkSubmitDelay=0;checkDisplayDelay=0;\n\
function delaySubmit(e){var v=0,w,i=parseInt(e.id.substring(6, e.id.indexOf('-')));\n\
 v+=Number(document.getElementById('switch'+i+'-d-duration').value)*86400;\n\
 w=Number(document.getElementById('switch'+i+'-h-duration').value);if(w>0||v)v+=w*3600;\n\
 w=Number(document.getElementById('switch'+i+'-mn-duration').value);if(w>0||v)v+=w*60;\n\
 v+=Number(document.getElementById('switch'+i+'-s-duration').value);\n\
 v=((v&&(v+1))?(v*1000):-1);\n\
 clearTimeout(checkDisplayDelay);checkDisplayDelay=setTimeout(function(){displayDelay(v,i);},500);\n\
 document.getElementById('delayOn'+i).value=v;\n\
 clearTimeout(checkSubmitDelay);checkSubmitDelay=setTimeout(function(){\n\
  checkSubmitDelay=0;RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_PIN_GPIO "\":{\"'+parameters.pinOrder[i]+'\":{\"" ROUTE_PIN_VALUE "\":'+v+'}}}');}, 3000);\n\
}\n\
function reverseSubmit(e){\n\
 RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_PIN_GPIO "\":{\"'+document.getElementById('pinNumber').value+'\":{\"" ROUTE_PIN_REVERSE "\":'+e.checked+'}}}');\n\
}\n\
function restartDevice(){RequestDevice('" ROUTE_PIN_GPIO "?" ROUTE_RESET "');}\n\
function plugnameSubmit(e){if(checkPlugName(e))RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_PIN_GPIO "\":{\"'+document.getElementById('pinNumber').value+'\":{\"" ROUTE_PIN_NAME "\":\"'+e.value+'\"}}}');}\n\
function mqttBrokerSubmit(e){if(checkMqttBroker(e)){RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_MQTT_BROKER "\":\"'+e.value+'\"}');};checkConfPopup();}\n\
function mqttPortSubmit(e){if(checkMqttPort(e)){RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_MQTT_PORT "\":'+e.value+'}');};checkConfPopup();}\n\
function mqttIdentSubmit(e){if(checkMqttIdent(e)){RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_MQTT_IDENT "\":\"'+e.value+'\"}');};checkConfPopup();}\n\
function mqttUserSubmit(e){if(checkMqttUser(e)){RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_MQTT_USER "\":\"'+e.value+'\"}');};checkConfPopup();}\n\
function mqttPwdSubmit(e){if(checkMqttPwd(e)){RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_MQTT_PWD "\":\"'+e.value+'\"}');};checkConfPopup();}\n\
function mqttOutTopicSubmit(e){if(checkMqttOutTopic(e)){RequestDevice('" ROUTE_PIN_GPIO "?{\"" ROUTE_MQTT_PUBTOPIC "\":\"'+e.value+'\"}');};checkConfPopup();}\n\n\
"));}

const __FlashStringHelper* HTML_JMainDisplay(){ return(F("\
//===========Create the page:\n\
function addParameters(i){var v,d=document.createElement('div');d.style='display:none;';\n\
 d.appendChild(v=document.createElement('input'));v.type='checkbox';v.id='outReverse'+i;\n\
 d.appendChild(v=document.createElement('input'));v.type='text';v.id='delayOn'+i;v.value=(getGpioParam('" ROUTE_PIN_VALUE "',i)||-1);\n\
 return d;\n\
}\n\
function addTheBullet(i){var d=document.createElement('div'),b; d.appendChild(b=document.createElement('button'));\n\
 b.name=parameters.pinOrder[b.id=i]; b.classList.add('bulle'); b.title=(getGpioParam('" ROUTE_PIN_NAME "',i)||('switch'+i))+' configuration';b.setAttribute('onclick','initConfPopup(this);');\n\
 return d;\n\
}\n\
function addTheSwitchName(i){var d=document.createElement('div');\n\
 d.classList.add('outputName'); d.innerHTML=(getGpioParam('" ROUTE_PIN_NAME "',i)||('switch'+i));return d;\n\
}\n\
function addTheSwitch(i){var l,s,d;\n\
 d=document.createElement('div');d.classList.add('onoffswitch');d.classList.add('delayConf');\n\
 d.appendChild(l=document.createElement('input'));l.type='checkbox';l.classList.add('onoffswitch-checkbox');\n\
 l.id='switch'+i;l.name=(getGpioParam('" ROUTE_PIN_NAME "',i)||('switch'+i));l.setAttribute('onclick','switchSubmit(this);');\n\
 d.appendChild(l=document.createElement('label'));l.classList.add('onoffswitch-label');l.setAttribute('for','switch'+i);\n\
 l.appendChild(s=document.createElement('span'));s.classList.add('onoffswitch-inner');\n\
 l.appendChild(s=document.createElement('span'));s.classList.add('onoffswitch-switch');\n\
 return d;\n\
}\n\
function addTheDelay(i){var v,d,ret=document.createElement('div');\n\
 ret.appendChild((d=document.createElement('div')));d.classList.add('delayConf');\n\
 d.appendChild(v=document.createElement('span'));v.innerHTML='&nbsp;&nbsp;&nbsp;(Timer&nbsp;';\n\
 d.appendChild(v=document.createElement('input'));v.type='checkbox';v.id='switch'+i+'-timer';v.classList.add('onofftimer');\n\
 d.appendChild(v=document.createElement('span'));v.innerHTML=':&nbsp;&nbsp;';\n\
 ret.appendChild((d=document.createElement('div')));d.id='switch'+i+'days-duration';d.classList.add('delayConf');d.style='display:none;';\n\
 d.appendChild(v=document.createElement('input'));v.type='number';v.id='switch'+i+'-d-duration';\n\
 v.classList.add('duration');v.setAttribute('min','0');\n\
 v.setAttribute('data-unit','86400');v.setAttribute('onChange','delaySubmit(this);');\n\
 d.appendChild(v=document.createElement('span'));v.innerHTML='d &nbsp;';\n\
 ret.appendChild((d=document.createElement('div')));d.id='switch'+i+'hours-duration';d.classList.add('delayConf');d.style='display:none;';\n\
 d.appendChild(v=document.createElement('input'));v.type='number';v.id='switch'+i+'-h-duration';\n\
 v.classList.add('duration');v.setAttribute('min','-1');\n\
 v.setAttribute('data-unit','3600');v.setAttribute('onChange','delaySubmit(this);');\n\
 d.appendChild(v=document.createElement('span'));v.innerHTML='h &nbsp;';\n\
 ret.appendChild((d=document.createElement('div')));d.id='switch'+i+'minutes-duration';d.classList.add('delayConf');d.style='display:none;';\n\
 d.appendChild(v=document.createElement('input'));v.type='number';v.id='switch'+i+'-mn-duration';\n\
 v.classList.add('duration');v.setAttribute('min','-1');\n\
 v.setAttribute('data-unit','60');v.setAttribute('onChange','delaySubmit(this);');\n\
 d.appendChild(v=document.createElement('span'));v.innerHTML='mn &nbsp;';\n\
 ret.appendChild((d=document.createElement('div')));d.id='switch'+i+'secondes-duration';d.classList.add('delayConf');\n\
 d.appendChild(v=document.createElement('input'));v.type='number';v.id='switch'+i+'-s-duration';\n\
 v.classList.add('duration');v.classList.add('sDuration');v.setAttribute('min','-1');\n\
 v.setAttribute('data-unit','1');v.setAttribute('onChange','delaySubmit(this);');v.value='-1';\n\
 d.appendChild(v=document.createElement('span'));v.innerHTML='s)';\n\
 ret.classList.add('delayConf');\n\
 return ret;\n\
}\n\
function addRawSwitch(i){var v,rawSwitch;\n\
 rawSwitch=document.createElement('tr');\n\
 //Add a BLANK FIELD:\n\
 rawSwitch.appendChild(v=document.createElement('td'));v.style='width:30px;';\n\
 //Add the BULLET:\n\
 rawSwitch.appendChild(v=document.createElement('td'));v.style='width:30px;';v.appendChild(addTheBullet(i));\n\
 //Add the SWITCHNAME:\n\
 rawSwitch.appendChild(v=document.createElement('td'));v.style='width:150px;';v.appendChild(addTheSwitchName(i));\n\
 //Add the SWITCH:\n\
 rawSwitch.appendChild(v=document.createElement('td'));v.appendChild(addTheSwitch(i));v.appendChild(addTheDelay(i));\n\
 //Add the PARAMETERS:\n\
 rawSwitch.appendChild(addParameters(i));\n\
 //Create the rawTable and return:\n\
 v=document.createElement('table');v.id='switch'+i+'mqttConf';v.appendChild(rawSwitch);\n\
 return v;\n\
}\n\
function createSwitches(){var p,e;\n\
 setHostName(document.getElementById('hostname').value=parameters." ROUTE_HOSTNAME ");\n\
 if(parameters." ROUTE_IP_ADDR ".length) document.getElementById('ipAddr').innerHTML=parameters." ROUTE_IP_ADDR ";\n\
 if(parameters." ROUTE_MAC_ADDR ".length) document.getElementById('macAddr').innerHTML=parameters." ROUTE_MAC_ADDR ";\n\
 if(parameters." ROUTE_CHIP_IDENT ".length) document.getElementById('chipIdent').innerHTML=parameters." ROUTE_CHIP_IDENT ";\n\
 e=document.getElementById('" ROUTE__DEFAULT_HOSTNAME "');\n\
 e.innerHTML =(parameters." ROUTE__DEFAULT_HOSTNAME ".length?parameters." ROUTE__DEFAULT_HOSTNAME ":'" ROUTE__DEFAULT_HOSTNAME "')+'/';\n\
 e.innerHTML+=(parameters." ROUTE__DEFAULT_PASSWORD ".length?parameters." ROUTE__DEFAULT_PASSWORD ":'" ROUTE__DEFAULT_PASSWORD "');\n\
 if(parameters." ROUTE_NTP_SOURCE "){\n\
 if(parameters." ROUTE_NTP_SOURCE ".length) document.getElementById('ntpSource').value=parameters." ROUTE_NTP_SOURCE ";\n\
 if(parameters." ROUTE_NTP_ZONE ".length) document.getElementById('" ROUTE_NTP_ZONE "').value=parameters." ROUTE_NTP_ZONE ";\n\
 if(parameters." ROUTE_NTP_DAYLIGHT ".length)document.getElementById('ntpDayLight').checked=(parameters." ROUTE_NTP_DAYLIGHT "!='0');\n}\n\
 if(parameters." ROUTE_VERSION ".length) document.getElementById('version').innerHTML=parameters." ROUTE_VERSION ";else document.getElementById('version').innerHTML='?';\n\
 if(!document.getElementById('switches').rows.length){setSSID();\n\
  for(var i=0,td,tr,e=document.getElementById('switches');i<parameters.pinOrder.length; i++){\n\
   e.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
   td.classList.add('switches');td.appendChild(addRawSwitch(i));\n\
}}}\n\
function refreshUptime(sec){var e;\n\
 if((e=document.getElementById('uptime'))){sec/=1000;\n\
  e.innerHTML=Math.trunc(sec/(24*3600));e.innerHTML+='d-';\n\
  e.innerHTML+=Math.trunc((sec%=24*3600)/3600);e.innerHTML+='h-';\n\
  e.innerHTML+=Math.trunc((sec%=3600)/60);e.innerHTML+='mn';\n\
}}\n\
function displayDelays(){\n\
 for(var i=0;i<parameters.pinOrder.length;i++)displayDelay(Number(getGpioParam('" ROUTE_PIN_VALUE "',i)),i);\n}\n\
function refreshSwitches(){if(parameters.pinOrder.length){\n\
 createSwitches();refreshUptime(parameters." ROUTE_UPTIME ");displayDelays();\n\
 for(var i=0;i<parameters.pinOrder.length;i++){var e;\n\
  if((e=document.getElementById('switch'+i)))e.checked=(getGpioParam('" ROUTE_PIN_STATE "',i) ?true :false);\n\
  document.getElementById('switch'+i+'-timer').disabled=!(document.getElementById('switch'+i+'-timer').checked=!document.getElementById('switch'+i).checked);\n\
  if(document.getElementById('switch'+i+'-s-duration').value==-1)document.getElementById('switch'+i+'-timer').checked=(document.getElementById('switch'+i+'-timer').disabled=true);\n\
 }\n\
}}\n\n\
"));}

const __FlashStringHelper* HTML_JSSIDDisplay(){ return(F("\
function addSSID(b=true){\n\
 if(!(e=document.getElementById('ssids'))) return false;\n\
 var e,v,table,td,tr,i=e.getElementsByTagName('TABLE').length;\n\
 e.appendChild(td=document.createElement('td'));td.appendChild(table=document.createElement('table'));\n\
 table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
 td.innerHTML='SSID '+(i+1)+':';\n\
 table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
 td.appendChild(v=document.createElement('input'));v.type='text';v.id='ssid'+i;v.value='';\n\
 table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
 td.innerHTML='Password:';\n\
 table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
 td.appendChild(v=document.createElement('input'));v.type='password';v.id='pwd'+i;v.setAttribute('onfocus','ssidPwFocus(this)');\n\
 table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
 td.appendChild(v=document.createElement('input'));v.id='addSSID'+i;v.disabled=!b;\n\
 v.type='button';v.value='Submit';v.setAttribute('onclick','ssidSubmit(this);');\n\
 td.appendChild(v=document.createElement('span'));v.innerHTML='&nbsp;';\n\
 td.appendChild(v=document.createElement('input'));v.id='removeSSID'+i;v.disabled=b;\n\
 v.type='button';v.value='Remove';v.setAttribute('onclick','deleteSSID(this);');\n\
 return true;\n\
}\n\
function setSSID(){\n\
 if(parameters." ROUTE_WIFI_SSID "){\n\
  for(var i=0;i<parameters." ROUTE_WIFI_SSID ".length;i++) if(addSSID(false)){var v;\n\
   (v=document.getElementById('ssid'+i)).value=parameters.ssid[i];v.readOnly=true;\n\
   (v=document.getElementById('pwd'+i)).value='************';\n\
 }}else addSSID();\n\
}\n\n\
"));}

const __FlashStringHelper* HTML_JMQTTDisplay(){ return(F("\
//===========MQTT management:\n\
function setDisabled(v, b){for(var i=0;v[i];i++)v[i].disabled=b;}\n\
\n\
function checkPlugName(e){return (e.value===''?false:true);}\n\
function checkMqttBroker(e){return true;}\n\
function checkMqttPort(e){return(Number(e.value)!=0);}\n\
function checkMqttIdent(e){return true;}\n\
function checkMqttUser(e){return true;}\n\
function checkMqttPwd(e){return true;}\n\
function checkMqttOutTopic(e){return true;}\n\
function checkMqttFieldName(e){return(e.value!='');}\n\
\n\
function checkConfPopup(){\n\
 if(!checkPlugName(document.getElementById('plugName')))return false;\n\
 if(!checkMqttBroker(document.getElementById('mqttBroker')))return false;\n\
 if(!checkMqttPort(document.getElementById('mqttPort')))return false;\n\
 return true;\n\
}\n\
function refreshConfPopup(){checkConfPopup();}\n\
function initConfPopup(e){var f;\n\
 window.location.href='#confPopup';\n\
 document.getElementById('pinNumber').value=e.name;\n\
 document.getElementById('confName').innerHTML='Switch'+e.id;\n\
 document.getElementById('plugName').value=(getGpioParam('" ROUTE_PIN_NAME "',e.id)||('switch'+e.id));\n\
 document.getElementById('outReverse').checked=getGpioParam('" ROUTE_PIN_REVERSE "',e.id);\n\
 document.getElementById('mqttBroker').value=parameters." ROUTE_MQTT_BROKER ";\n\
 document.getElementById('mqttPort').value=parameters." ROUTE_MQTT_PORT ";\n\
 document.getElementById('mqttIdent').value=parameters." ROUTE_MQTT_IDENT ";\n\
 document.getElementById('mqttUser').value=parameters." ROUTE_MQTT_USER ";\n\
 document.getElementById('mqttPwd').value=parameters." ROUTE_MQTT_PWD ";\n\
 document.getElementById('mqttOutTopic').value=parameters." ROUTE_MQTT_PUBTOPIC ";\n\
 refreshConfPopup();\n\
}\n\
function closeConfPopup(){\n\
 if(checkConfPopup()){\n\
  ;\n\
 }else if(!confirm('Are you sure to cancel modifications?')) return;\n\
 window.location.href='';\n\
}\n\
"));}
#endif
