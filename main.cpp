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
#include <uart.h>

#include "setting6.h"   //Can be adjusted according to the project...

//Avoid to change the following:
#define DEBOUNCE_TIME        100L
#define INFINY               60000L
static String                hostname = DEFAULTHOSTNAME;   //Can be change by interface
static String                ssid[SSIDCount()];            //Identifiants WiFi /Wifi idents
static String                password[SSIDCount()];        //Mots de passe WiFi /Wifi passwords
static bool                  WiFiAP=false, outputValue[outputCount(allPins)], mustResto=false;
#ifdef DEFAULTWIFIPASS
static ushort                nbWifiAttempts=MAXWIFIRETRY, WifiAPTimeout;
#endif
static unsigned long         next_reconnect(0L), maxDurationOn[outputCount(allPins)], timerOn[outputCount(allPins)];
volatile short               intr(0);
volatile unsigned long       rebounds_completed;
volatile bool                serialAvaible=true;
static unsigned int          serialBufferLen=0;
static String                serialInputString;
ESP8266WebServer             server(80);
ESP8266HTTPUpdateServer      httpUpdater;

#define Serial_print(m)     {if(serialAvaible) Serial.print(m);}
#define Serial_printf(m,n)  {if(serialAvaible) Serial.printf(m,n);}
#ifndef DEFAULTWIFIPASS
  #undef DEBUG
#endif
#ifdef DEBUG
  WiFiServer                 telnetServer(23);
  WiFiClient                 telnetClient;
  #define DEBUG_print(m)    {if(String(DEFAULTWIFIPASS).length()){if(telnetClient && telnetClient.connected()) telnetClient.print(m);    Serial_print(m);}}
  #define DEBUG_printf(m,n) {if(String(DEFAULTWIFIPASS).length()){if(telnetClient && telnetClient.connected()) telnetClient.printf(m,n); Serial_printf(m,n);}}
#else
  #define DEBUG_print(m)     ;
  #define DEBUG_printf(m,n)  ;
#endif

void notifyHTTPProxy(String="");

inline bool isNow(unsigned long v) {unsigned long ms(millis()); return((v<ms) && (ms-v)<INFINY);}  //Because of millis() rollover:
inline void setTimer    (ushort i) {timerOn[i] = millis() + (1000L*maxDurationOn[i]);}
inline void unsetTimer  (ushort i) {timerOn[i] = (unsigned)(-1L);}
inline bool isTimer     (ushort i) {return(((signed)timerOn[i]!=(-1L)));}

inline String getHostname(){return hostname;}

void sendHTML(){
  String s;
  s = (F("<!DOCTYPE HTML>\n<html lang='us-US'>\n<head><meta charset='utf-8'/>\n<title>"));
  s+= getHostname();
  s+= F("</title>\n<style>body{background-color:#fff7e6; font-family:Arial,Helvetica,Sans-Serif; Color:#000088;}\n");
  s+= F(" ul{list-style-type:square;} li{padding-left:5px; margin-bottom:10px;}\n");
  s+= F(" td{text-align:left; min-width:110px; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n");
  s+= F(" .modal{display:none; position:fixed; z-index:1; left:0%; top:0%; height:100%; width:100%; overflow:scroll; background-color:#000000;}\n");
  s+= F(" .modal-content{background-color:#fff7e6; margin:5% auto; padding:15px; border:2px solid #888; height:90%; width:90%; min-height:755px;}\n");
  s+= F(" .close{color:#aaa; float:right; font-size:30px; font-weight:bold;}\n");
  s+= F(" .delayConf{float: left; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n");
  s+= F(" .duration{width:40px;}\n  /");
  s+= F("*see: https://proto.io/freebies/onoff/: *");
  s+= F("/\n .onoffswitch {position: relative; width: 90px;-webkit-user-select:none; -moz-user-select:none; -ms-user-select: none;}\n");
  s+= F(" .onoffswitch-checkbox {display: none;}\n");
  s+= F(" .onoffswitch-label {display: block; overflow: hidden; cursor: pointer;border: 2px solid #999999; border-radius: 20px;}\n");
  s+= F(" .onoffswitch-inner {display: block; width: 200%; margin-left: -100%;transition: margin 0.3s ease-in 0s;}\n");
  s+= F(" .onoffswitch-inner:before, .onoffswitch-inner:after {display: block; float: left; width: 50%; height: 30px; padding: 0; line-height: 30px;font-size: 14px; color: white; font-family: Trebuchet, Arial, sans-serif; font-weight: bold;box-sizing: border-box;}\n");
  s+= F(" .onoffswitch-inner:before{content: 'ON';padding-left: 10px;background-color: #34C247; color: #FFFFFF;}\n");
  s+= F(" .onoffswitch-inner:after {content: 'OFF';padding-right: 10px;background-color: #EEEEEE; color: #999999;text-align: right;}\n");
  s+= F(" .onoffswitch-switch{display: block; width: 18px; margin: 6px;background: #FFFFFF;position: absolute; top: 0; bottom: 0;right: 56px;border: 2px solid #999999; border-radius: 20px;transition: all 0.3s ease-in 0s;}\n");
  s+= F(" .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-inner {margin-left: 0;}\n");
  s+= F(" .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-switch {right: 0px;}\n");
  s+= F("</style></head>\n<body onload='init();'>\n");
  s+= F("<script>\nthis.timer=0;\n");
  s+= F("function init(){refresh();}\n");
  s+= F("function refresh(v=30){\n clearTimeout(this.timer); document.getElementById('about').style.display='none';\n");
  s+= F(" this.timer=setTimeout(function(){RequestStatus();refresh(v);}, v*1000);}\n");
  s+= F("function RequestStatus(){var j, ret, e, req=new XMLHttpRequest();req.open('GET',document.URL+'plugValues',false); ");
  s+= F("req.send(null);ret=req.responseText;\nif((j=ret.indexOf('[')) >= 0){\n");
  s+= F("if((e=document.getElementsByClassName('onoffswitch-checkbox')).length && (j=ret.indexOf('['))>=0)\n");
  s+= F(" for(var e,v,i=0,r=ret.substr(j+1);r[0] && r[0]!=']';i++){\n");
  s+= F("  j=r.indexOf(',');if(j<0) j=r.indexOf(']');v=parseInt(r.substr(0,j));\n");
  s+= F("  if(v>=0) e[i].checked=(v?true:false);r=r.substr(j+1);\n");
  s+= F(" }if((e=document.getElementsByClassName('onoffswitch-checkbox')).length)\n");
  s+= F("  for(var e,v,i=0,ret=r.substr(j+1);r[0] && r[0]!=']';i++){\n");
  s+= F("   j=r.indexOf(',');if(j<0) j=r.indexOf(']');v=parseInt(r.substr(0,j));\n");
  s+= F("   if(v>=0) e[i].checked=(v?true:false);r=r.substr(j+1);\n");
  s+= F("}}}\nfunction showHelp(){var e;\ne=document.getElementById('example1');e.innerHTML=document.URL+'plugValues?");
  for(ushort i(0); outputCount(allPins);){
    s+= outputName[i] + "=" + (outputValue[i] ?"true" :"false");
    if(++i>=outputCount(allPins)) break;
    s+= "&";
  }s+=F("';e.href=e.innerHTML;\ne=document.getElementById('example2');e.innerHTML=document.URL+'plugValues';e.href=e.innerHTML;\n");
  s+= F("refresh(120);document.getElementById('about').style.display='block';}\n");
  s+= F("function saveSSID(f){\nif((f=f.parentNode)){var s, p=false;\n");
  s+= F("for(var i=0;i<f.children.length;i++){\n");
  s+= F("if(f.children[i].type=='password'){\n");
  s+= F("if (!p) p=f.children[i];\nelse if(p.value!=f.children[i].value) p.value='';\n");
  s+= F("}else if(f.children[i].type=='text') s=f.children[i];\n");
  s+= F("}if(s.value==''){\nalert('Empty SSID...');f.reset();s.focus();\n");
  s+= F("}else if(p.value==''){\nvar ssid=s.value;f.reset();s.value=ssid;\nalert('Incorrect password...');p.focus();\n");
  s+= F("}else f.submit();\n");
  s+= F("}}\nfunction deleteSSID(f){\n");
  s+= F("if((f=f.parentNode))\nfor(var i=0;i<f.children.length;i++)\n");
  s+= F("if(f.children[i].type=='text') if(f.children[i].value!=''){\n");
  s+= F("if(confirm('Are you sure to remove this SSID?')){\n");
  s+= F("for(var i=0;i<f.children.length;i++)\nif(f.children[i].type=='password') f.children[i].value='';\nf.submit();\n");
  s+= F("}}else alert('Empty SSID...');\n}\n");
  s+= F("function switchSubmit(e){refresh();\n");
  s+= F(" var b=false, l=e.parentNode.parentNode.getElementsByTagName('input');\n");
  s+= F(" for(var i=0;i<l.length;i++) if(l[i].type=='number')\n   b|=(l[i].value!='0');\n");
  s+= F(" e.checked&=b;document.getElementById('switchs').submit();\n}\n");
  s+= F("var checkDelaySubmit=0;\n");
  s+= F("function checkDelay(e){refresh();\n");
  s+= F(" if(e.value=='-1'){\n");
  s+= F("  var l=e.parentNode.getElementsByTagName('input');\n");
  s+= F("  for(var i=0;i<l.length;i++)\n   if(l[i].className=='duration'){\n    if(l[i].getAttribute('data-unit')!=1)\n     l[i].style.display='none';\n     else l[i].style.display='inline-block';}\n");
  s+= F(" }clearTimeout(this.checkDelaySubmit);this.checkDelaySubmit=setTimeout(function(){this.checkDelaySubmit=0;document.getElementById('switchs').submit();}, 1000);\n}\n");
  s+= F("</script>\n<div id='about' class='modal'><div class='modal-content'>");
  s+= F("<span class='close' onClick='refresh();'>&times;</span>");
  s+= F("<h1>About</h1>");
  s+= F("This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domotics or Jeedom.<br><br>");
  s+= F("In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). ");
  s+= F("Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs, like this:");
  s+= F("<a id='example1' style='padding:0 0 0 5px;'></a><br><br>");
  s+= F("The state of the electrical outlets can also be requested from the following URL: ");
  s+= F("<a id='example2' style='padding:0 0 0 5px;'></a><br><br>");
  s+= F("The status of the power strip is retained when the power is turned off and restored when it is turned on ; a power-on duration can be set on each output: (-1) no delay, (0) to disable an output and (number of s) to configure the power-on duration.<br><br>");
  s+= F("The following allows you to configure some parameters of the Wifi Power Strip (until a SSID is set and reached, the socket works as an access point with its own SSID and default password: \"");
  s+= String(DEFAULTHOSTNAME) + "/";
#ifdef DEFAULTWIFIPASS
  s+= String(DEFAULTWIFIPASS).length() ?DEFAULTWIFIPASS :"none";
#else
  s+= "none";
#endif
  s+= F("\" on 192.168.4.1).<br><br>");
  s+= F("<table style='width:100%'><tr>");
  s+= F("<td style='width:50%'><form method='POST'>\n<h2>Network name: <input type='text' name='hostname' value='");
  s+= getHostname();
  s+= F("' size='10'><input type='button' value='Submit' onclick='submit();'></h2></form></td>");
  s+= F("<td><form method='POST'><h2>Reboot device :  <input type='button' name='Reboot' value='Reboot' onclick='submit();'></h2><input type='checkbox' name='Reboot' id='Reboot' checked style=\"display:none\"></form></td></tr></table>\n");
#ifdef DEFAULTWIFIPASS
  if(String(DEFAULTWIFIPASS).length()){
    s+= F("<h2>Network connection:</h2>\n");
    s+= F("<table style='width:100%'><tr>");
    for(ushort i(0); i<SSIDCount(); i++){
      s+= F("<td><div><form method='POST'>\nSSID ");
      s+= String(i+1, DEC);
      s+= F(":<br><input type='text' name='SSID' value='");
      s+= ssid[i] + (ssid[i].length() ?F("' readonly><br>\n"): F("'><br>\n"));
      s+= F("Password:<br><input type='password' name='password' value='");
      s+= password[i];
      s+= F("'><br>\nConfirm password:<br><input type='password' name='confirm' value='");
      s+= password[i];
      s+= F("'><br><br>\n<input type='button' value='Submit' onclick='saveSSID(this);'>");
      s+= F("<input type='button' value='Remove' onclick='deleteSSID(this);'>\n</form></div></td>");
    }s+= F("</tr></table>\n");
  }
#endif
  s+= F("<h2><form method='POST'>Names of Plugs: ");
  for(ushort i(0); i<outputCount(allPins); i++){
    s+= F("<input type='text' name='plugName");
    s+= String(i, DEC);
    s+= F("' value='");
    s+= outputName[i];
    s+= F("' style='width:70;'>");
  }
  s+= F(" - <input type='button' value='Submit' onclick='submit();'></form></h2>\n");
  s+= F("<h6><a href='update' onclick='javascript:event.target.port=80'>Firmware update</a>");
  s+= F(" - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>");
  s+= F("</div></div>\n");
  s+= F("<table style='border:0;width:100%;'><tbody><tr><td><h1>");
  s+= getHostname() + " - " + (WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString()) + " [" + WiFi.macAddress();
  s+= F("] :</h1></td><td style='text-align:right;vertical-align:top;'><p><span class='close' onclick='showHelp();'>?</span></p></td>");
  s+= F("<tr></tbody></table>\n<h3>Status :</h3>\n");
  s+= F("<form id='switchs' method='POST'><ul>\n");
  for (ushort i=0; i<outputCount(allPins); i++){ bool display;
    s+= F("<li><table><tbody>\n<tr><td>");
    // Tittle:
    s+= outputName[i];
    // Switch:
    s+= F("</td><td>\n<div class='onoffswitch delayConf'>\n<input type='checkbox' class='onoffswitch-checkbox' id='");
    s+= outputName[i] + "' name='" + outputName[i] + "' " + String(outputValue[i] ?"checked" :"");
    s+= F(" onClick='switchSubmit(this);'>\n<label class='onoffswitch-label' for='");
    s+= outputName[i];
    s+= F("'><span class='onoffswitch-inner'></span><span class='onoffswitch-switch'></span></label>\n</div>\n<div class='delayConf'>&nbsp;&nbsp;&nbsp;\n");
    s+= "(<input type='checkbox' name='" + outputName[i] + "-timer' " + String((outputValue[i] || (signed)maxDurationOn[i]==(-1L) || !maxDurationOn[i]) ?"uncheck": "checked") + String(((signed)maxDurationOn[i]==(-1L) || !maxDurationOn[i]) ?" disabled": "") + ">Timer:&nbsp;\n";
    // Days duration:
    display=( (long)maxDurationOn[i]!=(-1L) && (maxDurationOn[i]/86400L)>0L );
    s+= "<input type='number' name='" + outputName[i] + "-max-duration-d' value='" + String(display ?(maxDurationOn[i]/86400L) :0L, DEC);
    s+= F("' min='0' max='31' data-unit=86400 class='duration' style='width:60px;display:");
    s+= String(display ?F("inline-block") :F("none")) + F(";' onChange='checkDelay(this);'>") + (display ?F("d &nbsp;\n") :F("\n"));
    // Hours duration:
    display|=( (long)maxDurationOn[i]!=(-1L) && (maxDurationOn[i]%86400L/3600L)>0L );
    s+= "<input type='number' name='" + outputName[i] + "-max-duration-h' value='" + String(display ?(maxDurationOn[i]%86400L/3600L) :0L, DEC);
    s+= String(F("' min='0' max='24' data-unit=3600 class='duration' style='display:")) + String(display ?F("inline-block") :F("none"));
    s+= String(F(";' onChange='checkDelay(this);'>")) + String(display ?F("h &nbsp;\n") :F("\n"));
    // Minutes duration:
    display|=( (long)maxDurationOn[i]!=(-1L) && (maxDurationOn[i]%86400L%3600L/60L)>0L );
    s+= "<input type='number' name='" + outputName[i] + "-max-duration-mn' value='" + String(display ?(maxDurationOn[i]%86400L%3600L/60L) :0L, DEC);
    s+= String(F("' min='0' max='60' data-unit=60 class='duration' style='display:")) + String(display ?F("inline-block") :F("none"));
    s+= String(F(";' onChange='checkDelay(this);'>")) + String(display ?F("mn &nbsp;\n") :F("\n"));
    // Secondes duration:
    s+= String(F("<input type='number' name='")) + outputName[i] + String(F("-max-duration-s' value='"));
    s+= ( ((long)maxDurationOn[i]!=(-1L)) ? String(maxDurationOn[i]%86400L%3600L%60L, DEC) : String(-1L, DEC) );
    s+= F("' min='-1' max='60' data-unit=1 class='duration' onChange='checkDelay(this);'>");
    s+= String( ((long)maxDurationOn[i]!=(-1L)) ?"s" :"-" );
    // End
    s+= F(")</div>\n</td></tr>\n</tbody></table></li>\n");
  } s+= F("</ul><div><input type='checkbox' name='newValue' id='newValue' checked style=\"display:none\"></div></form>\n");
  unsigned long sec=millis()/1000L;
  s+= F("<h6>(Uptime: ");
  s+= String(sec/(24L*3600L)) + "d-";
  s+= String((sec%=24L*3600L)/3600L) + "h-";
  s+= String((sec%=3600L)/60L) + "mn)</h6>";
  s+= F("\n</body></html>\n\n");
  server.send(200, "text/html", s);
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

bool readConfig(bool=true);
void writeConfig(){                                      //Save current config:
  if(!readConfig(false))
    return;
  if( !SPIFFS.begin() ){
    DEBUG_print("Cannot open SPIFFS!...\n");
    return;
  }File f=SPIFFS.open("/config.txt", "w+");
  if(f){
    f.println(ResetConfig);
    f.println(getHostname());                            //Save hostname
    shiftSSID(); for(ushort i(0); i<SSIDCount(); i++){   //Save SSIDs
      f.println(ssid[i]);
      f.println(password[i]);
    }f.println(mustResto);
    unsigned long m=millis();
    for(ushort i(0); i<outputCount(allPins); i++){       //Save output states
      f.println(outputName[i]);
      f.println(outputValue[i]);
      f.println((long)maxDurationOn[i]);
      f.println( ((long)timerOn[i]==(-1L)) ?(-1L) :(long)((timerOn[i]<m) ?(~m+timerOn[i]) :(timerOn[i]-m)) );
    }f.close(); SPIFFS.end();
    DEBUG_print("SPIFFS writed.\n");
} }

String readString(File f){ String ret=f.readStringUntil('\n'); ret.remove(ret.indexOf('\r')); return ret; }
inline bool getConfig(String& v, File f, bool w){String r(readString(f).c_str());       if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(bool&   v, File f, bool w){bool   r(atoi(readString(f).c_str())); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(int&    v, File f, bool w){int    r(atoi(readString(f).c_str())); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(long&   v, File f, bool w){long   r(atol(readString(f).c_str())); if(r==v) return false; if(w)v=r; return true;}
bool readConfig(bool w){                      //Get config (return false if config is not modified):
  bool ret=false;
  if( !SPIFFS.begin() ){
    DEBUG_print("Cannot open SPIFFS!...\n");
    return false;
  }File f=SPIFFS.open("/config.txt", "r");
  if(f && ResetConfig!=atoi(readString(f).c_str())){
    f.close();
    if(w) DEBUG_print("New configFile version...\n");
  }if(!f){                                    //Write default config:
    if(w){
      for(ushort i(0); i<SSIDCount(); i++) password[i]="";
      for(ushort i(0); i<outputCount(allPins); i++){
        outputValue[i]=false; maxDurationOn[i]=timerOn[i]=(unsigned)(-1L);
      }SPIFFS.format(); SPIFFS.end(); writeConfig();
      DEBUG_print("SPIFFS initialized.\n");
    } return true;
  }ret|=getConfig(hostname, f, w);
  for(ushort i(0); i<SSIDCount(); i++){        //Get SSIDs
    ret|=getConfig(ssid[i], f, w);
    ret|=getConfig(password[i], f, w);
  } ret|=getConfig(mustResto, f, w);
  for(ushort i(0); i<outputCount(allPins); i++){     //Get output states
    ret|=getConfig(outputName[i], f, w);
    ret|=getConfig(outputValue[i], f, w);
    ret|=getConfig((long&)maxDurationOn[i], f, w);
    getConfig((long&)timerOn[i], f, w); if(timerOn[i]!=(unsigned)(-1L)) timerOn[i]+=millis();
  }f.close(); SPIFFS.end();
  return ret;
}

bool WiFiHost(){
#ifdef DEFAULTWIFIPASS
  if(!String(DEFAULTWIFIPASS).length()) return false;

  DEBUG_print("\nNo custom SSID found: setting soft-AP configuration ... \n");
  WifiAPTimeout=(WIFIAPDELAYRETRY/WIFISTADELAYRETRY); nbWifiAttempts=MAXWIFIRETRY;
  WiFi.mode(WIFI_AP);
//WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,254), IPAddress(255,255,255,0));
  WiFiAP=WiFi.softAP(DEFAULTHOSTNAME, DEFAULTWIFIPASS);
  DEBUG_print(
    WiFiAP
    ?(String("Connecting \"" + getHostname()+ "\" [") + WiFi.softAPIP().toString() + "] from: " + DEFAULTHOSTNAME + "/" + DEFAULTWIFIPASS + "\n\n").c_str()
    :"WiFi Timeout.\n\n");
  return WiFiAP;
#else
  return false;
#endif
}

void WiFiDisconnect(){
  next_reconnect=millis()+WIFISTADELAYRETRY;
  WiFi.softAPdisconnect(); WiFi.disconnect(); WiFiAP=false;
  DEBUG_print("Wifi disconnected!...\n");
}

bool WiFiConnect(){
#ifdef DEFAULTWIFIPASS
  if(!String(DEFAULTWIFIPASS).length()) return false;

  WiFiDisconnect();
  DEBUG_print("\n");
  for(ushort i(0); i<SSIDCount(); i++) if(ssid[i].length()){

    //Connection au reseau Wifi /Connect to WiFi network
    WiFi.mode(WIFI_STA);
    DEBUG_print(String("Connecting \"" + getHostname()+ "\" [") + String(WiFi.macAddress()) + "] to: " + ssid[i]);
    WiFi.begin(ssid[i].c_str(), password[i].c_str());

    //Attendre la connexion /Wait for connection
    for(ushort j(0); j<12 && WiFi.status()!=WL_CONNECTED; j++){
      delay(500L);
      DEBUG_print(".");
    }DEBUG_print("\n");

    if(WiFi.status()==WL_CONNECTED){
      nbWifiAttempts=MAXWIFIRETRY;
      //Affichage de l'adresse IP /print IP address:
      DEBUG_print("WiFi connected\n");
      DEBUG_print("IP address: "); DEBUG_print(WiFi.localIP()); DEBUG_print("\n\n");
      notifyHTTPProxy("Connected");
      return true;
    } WiFi.disconnect();
  }
  nbWifiAttempts--;
  if(ssid[0].length()){
    DEBUG_print("WiFi Timeout ("); DEBUG_print(nbWifiAttempts);
    DEBUG_print((nbWifiAttempts>1) ?" more attempts)." :" more attempt).\n");
  }else nbWifiAttempts=0;
  if(!nbWifiAttempts){
    return WiFiHost();
  }
#endif
  return false;
}

void reboot(){
  DEBUG_print("Restart needed!...\n");
  notifyHTTPProxy("Reboot");
  if(serialAvaible){
    Serial.print("S(-1)\n");
    delay(2000L);
  }mustResto=true; writeConfig();
  ESP.restart();
}

void connectionTreatment(){                   //Test connexion/Check WiFi every mn:
  if(isNow(next_reconnect)){
#ifdef MEMORYLEAKS
    if(String(MEMORYLEAKS).length()){
      if((next_reconnect=ESP.getFreeHeap())<MEMORYLEAKS) reboot(); DEBUG_print("FreeMem: ");
      DEBUG_print(next_reconnect); DEBUG_print("\n");
    }
#endif
    next_reconnect=millis()+WIFISTADELAYRETRY;

#ifdef DEFAULTWIFIPASS
    if(!String(DEFAULTWIFIPASS).length()) return;

    if( (!WiFiAP && WiFi.status()!=WL_CONNECTED) || (WiFiAP && ssid[0].length() && !WifiAPTimeout--) ){
      if (WiFiConnect())
#ifdef DEBUG
      { telnetServer.begin();
        telnetServer.setNoDelay(true);
      } }
    else if (telnetServer.hasClient()){   //Telnet client connection:
      if (!telnetClient || !telnetClient.connected()){
        if(telnetClient){
          telnetClient.stop();
          DEBUG_print("Telnet Client Stop\n");
        }telnetClient=telnetServer.available();
        telnetClient.flush();
        DEBUG_print("New Telnet client connected...\n");
      }
#endif
    ;}
#endif
} }

void setPin(int i, bool v, bool withNoTimer=false){
  if(i<outputCount(allPins) && outputValue[i]!=v){
    bool slave=true;                                      //I'm the Slave throught my UART
#ifdef DEFAULTWIFIPASS
    if(String(DEFAULTWIFIPASS).length()) slave=false;     //I'm the Master
#endif
    unsetTimer(i);
    if(i<outputCount(physPins)){
      if(slave) Serial_print("V(" + String(i, DEC) + "):" + (v ?"1" :"0") + (withNoTimer ?":-1\n" :"\n"));
      digitalWrite( _outputPin[i], (REVERSE_OUTPUT xor (outputValue[i]=v)) );
      DEBUG_print( "Set GPIO " + String(_outputPin[i], DEC) + "(" + outputName[i] + ") to " + (outputValue[i] ?"true\n" :"false\n") );
    }else if(!slave) Serial_print("S(" + String(i-physPins, DEC) + "):" + (v ?"1\n" :"0\n"));
    if(!slave){
      if(!withNoTimer && ((signed)maxDurationOn[i]!=(-1L)))
        setTimer(i);
      if(RESTO_VALUES_ON_BOOT)
        writeConfig();
      notifyHTTPProxy("Status-changed");
} } }

void uartSwitchsTreatment(){
  if(serialBufferLen){
#ifdef DEFAULTWIFIPASS
    //VitualPin state to Master from Slave:
    if(String(DEFAULTWIFIPASS).length())
    if(serialInputString.startsWith("V(")){
      ushort i=outputCount(physPins)+serialInputString[2]-'0';
      outputValue[i] = (serialInputString[5]=='1');
      if(serialInputString[6]==':')  unsetTimer(i);
      DEBUG_print( "Set GPIO uart(" + outputName[i] + ") to " + (outputValue[i] ?"true\n" :"false\n") );
    }
#else
    //Setting to Slave from Master:
    if(serialInputString.startsWith("S(")){
      if(serialInputString[2]=='-') ESP.restart();
      else setPin(serialInputString[2]-'0', (serialInputString[5]=='1'));
    }
#endif
    serialInputString=serialInputString.substring(serialBufferLen);
    serialBufferLen=0;
} }

void mySerialEvent(){
  if(serialAvaible) while(Serial.available()){
    char inChar = (char)Serial.read();
    if(inChar == '\n')
         serialBufferLen=serialInputString.length();
    else serialInputString += inChar;
} }

void timersTreatment(){
  for(ushort i(0); i<outputCount(allPins); i++)
    if(isTimer(i) && isNow(timerOn[i])){
      DEBUG_print("Timeout(" + String(maxDurationOn[i], DEC) + "s) on GPIO " + ((i<outputCount(physPins)) ?String(_outputPin[i], DEC) :"uart") + "(" + outputName[i] + "):\n");
      setPin(i, !outputValue[i], true);
      notifyHTTPProxy("Status-changed");
}   }

void handleSubmitSSIDConf(){                                        //Setting:
  ushort count=0;
  for(ushort i(0); i<SSIDCount(); i++) if(ssid[i].length()) count++;
  for(ushort i(0); i<count;     i++)
    if(ssid[i]==server.arg("SSID")){                                //Modify password if SSID exist
      password[i]=server.arg("password");
      if(!password[i].length())                                     //Delete this ssid if no more password
        ssid[i]=="";
      return;
    }
  if(count<SSIDCount()){                                            //Add ssid:
    ssid[count]=server.arg("SSID");
    password[count]=server.arg("password");
} }

inline bool handlePlugnameSubmit(ushort i){                         //Set outputs names:
  if(server.hasArg("plugName"+(String)i) && server.arg("plugName"+(String)i))
    return(outputName[i]=server.arg("plugName"+(String)i));
  return false;
}

inline bool handleDurationOnSubmit(ushort i){ unsigned int v;        //Set outputs durations:
  if(!server.hasArg(outputName[i]+"-max-duration-s"))
    return false;
  v   =atoi((server.arg(outputName[i]+"-max-duration-s")).c_str());
  if(server.hasArg(outputName[i]+"-max-duration-mn"))
    v+=atoi((server.arg(outputName[i]+"-max-duration-mn")).c_str())*60;
  if(server.hasArg(outputName[i]+"-max-duration-h"))
    v+=atoi((server.arg(outputName[i]+"-max-duration-h")).c_str())*3600;
  if(server.hasArg(outputName[i]+"-max-duration-d"))
    v+=atoi((server.arg(outputName[i]+"-max-duration-d")).c_str())*86400;
  if(maxDurationOn[i]==(unsigned long)v)
    return false;
  maxDurationOn[i] = (unsigned long)v;
  return true;
}

inline void handleValueSubmit(ushort i){                            //Set outputs values:
  if(server.hasArg(outputName[i]) && outputValue[i])                // if param -> 1; else -> 0
    return;
  setPin(i, server.hasArg(outputName[i]), (!server.hasArg(outputName[i]+"-timer"))); // not arg if unchecked...
  return;
}

void  handleRoot(){ bool w;
  if(server.hasArg("Reboot"))
    reboot();
  if((w=server.hasArg("hostname")))
    hostname=server.arg("hostname");                                //Set host name
  else if((w=server.hasArg("password"))){
    handleSubmitSSIDConf(); shiftSSID();                            //Set WiFi connections
    if(WiFiAP && ssid[0].length()) WiFiDisconnect();
  }else{
    for(ushort i(0); i<outputCount(allPins); i++)
      w|=handlePlugnameSubmit(i);                                   //Set plug name
    if(!w) for(ushort i(0); i<outputCount(allPins); i++)
      w|=handleDurationOnSubmit(i);                                 //Set timeouts
    if(!w && server.hasArg("newValue"))
      for(ushort i=(0); i<outputCount(allPins); i++)
        handleValueSubmit(i);                                       //Set values
  }if(w) writeConfig();
  if(serialAvaible) {delay(100L);mySerialEvent(); uartSwitchsTreatment();}
  sendHTML();
}

String getPlugNames(){
  String s="";
  for(ushort i(0); outputCount(allPins); ){
    s += outputName[i];
    if((++i)>=outputCount(allPins)) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void  setPlugNames(){
  String v;
  for(ushort i(0); i<outputCount(allPins); i++){
    v=outputName[i]; v.toLowerCase();
    if ((v=server.arg(v))!=""){
      v.toLowerCase();
      outputName[i] = v;
} } }

String getPlugTimers(){
  String s="";
  for(ushort i(0); outputCount(allPins); ){
    s += maxDurationOn[i];
    if((++i)>=outputCount(allPins)) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void   setPlugTimers(){
  String v;
  for(ushort i(0); i<outputCount(allPins); i++){
    v=outputName[i]; v.toLowerCase();
    if ((v=server.arg(v))!=""){
      v.toLowerCase();
      timerOn[i] = millis() + atol(v.c_str()) * 1000L;
} } }

String getPlugValues(){
  String s="";
  for(ushort i(0); outputCount(allPins); ){
    s += outputValue[i];
    if((++i)>=outputCount(allPins)) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void   setPlugValues(){
  String v;
  for(ushort i(0); i<outputCount(allPins); i++){
    v=outputName[i]; v.toLowerCase();
    if ((v=server.arg(v))!=""){
      v.toLowerCase();
      setPin(i, ((v=="1" || v=="true") ?1 :0));
} } }

void notifyHTTPProxy(String s){
#ifdef NOTIFYPROXY
#ifdef DEFAULTWIFIPASS
  if(!String(DEFAULTWIFIPASS).length()) return;
  int port=8080;
#ifdef NOTIFYPort
  port=NOTIFYPort;
#endif
  WiFiClient client;
  if (client.connect(NOTIFYPROXY, port)) {
    String s="hostname=" + getHostname() + "&plugNames=" + getPlugNames() + "&values=" + getPlugValues() + "&msg=" + s;
    DEBUG_print("connected to the notification proxy...\n");
    // Make a HTTP request:
    client.println("POST / HTTP/1.1");
    client.println("Host: " + String(NOTIFYPROXY));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + s.length());
    client.println();
    client.println(s);
    client.println("Connection: close");
    client.println();
    client.stop();
  }
#endif
#endif
}

//Gestion des switchs/Switchs management
void ICACHE_RAM_ATTR debouncedInterrupt(){if(!intr) {intr--;rebounds_completed=millis()+DEBOUNCE_TIME;}}

void interruptTreatment(){
  if (intr && isNow(rebounds_completed)){               //switch activated...
    uint16_t reg=GPI; ushort n=0;
    for(ushort i(0); i<inputCount(); i++) if( (reg&(1<<_inputPin[i]))==0 ) n+=(1<<i);
    if (intr<0){                                        //the switch has just switched.
      intr=n;
      DEBUG_print("\nIO init: "); for(ushort i(inputCount()); i; i--) DEBUG_print(n&(1<<(i-1)) ?1 :0);
      rebounds_completed=millis()+DEBOUNCE_TIME;
    }else if(!n){                                       //Switch released...
      DEBUG_print("\nIO : "); for(ushort i(inputCount()); i; i--) DEBUG_print(1<<(i-1));
      DEBUG_print("\nGPI: "); for(ushort i(inputCount()); i; i--) DEBUG_print(intr&(1<<(i-1)) ?1 :0); DEBUG_print("\n");
      if(--intr<outputCount(physPins))
        setPin(intr, !outputValue[intr], (millis()-rebounds_completed>DISABLESWITCHTIMEOUT));
      intr=0;
    }else if(n!=intr){
      intr=0;
      DEBUG_print("\nIO ERROR.\n");
} } }

// ***********************************************************************************************
// **************************************** SETUP ************************************************
void setup(){
  //initialisation des broches /pins init
  readConfig();
  for(ushort i(0); i<inputCount(); i++){    //Entrées/inputs:
    pinMode(_inputPin[i], INPUT_PULLUP);     //only this mode works on all inputs !...
    //See: https://www.arduino.cc/en/Reference/attachInterrupt
    // or: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
    attachInterrupt(_inputPin[i], debouncedInterrupt, FALLING);
    if(_inputPin[i]==3 || _inputPin[i]==1)   serialAvaible=false;
  }for(ushort i(0); i<outputCount(physPins); i++){    //Sorties/ouputs:
      pinMode(_outputPin[i], OUTPUT);
      digitalWrite(_outputPin[i], (REVERSE_OUTPUT xor (outputValue[i]=((RESTO_VALUES_ON_BOOT || mustResto) ?outputValue[i] :false))));
      if(_outputPin[i]==3 || _outputPin[i]==1) serialAvaible=false;
  }

  if(serialAvaible){
    Serial.begin(115200);   //Disable use of D9 and D10...
    delay(10L);
    Serial.print("\nHello World!\n(ChipID="); Serial.print(ESP.getChipId()); Serial.print(")\n\n");
    serialInputString.reserve(32);
  }

  for(ushort i(physPins); i<outputCount(allPins); i++)
    Serial.print("S(" + String(i-physPins, DEC) + "):" + ((outputValue[i]=((RESTO_VALUES_ON_BOOT || mustResto) ?outputValue[i] :false)) ?"1\n" :"0\n"));

  if(mustResto){
    mustResto=false;
    writeConfig();
  }

  // Servers:
  WiFi.disconnect(); WiFi.softAPdisconnect();
#ifdef DEFAULTWIFIPASS
  if(String(DEFAULTWIFIPASS).length()){
    //Definition des URL d'entree /Input URL definition
    server.on("/",           [](){handleRoot();    server.client().stop();});
    server.on("/plugNames",  [](){setPlugNames();  server.send(200, "text/plain", "[" + getPlugNames()  + "]");});
    server.on("/plugTimers", [](){setPlugTimers(); server.send(200, "text/plain", "[" + getPlugTimers() + "]");});
    server.on("/plugValues", [](){setPlugValues(); server.send(200, "text/plain", "[" + getPlugValues() + "]");});
//  server.on("/about",      [](){ server.send(200, "text/plain", getHelp()); });
    server.onNotFound([](){server.send(404, "text/plain", "404: Not found");});

    httpUpdater.setup(&server);  //Adds OnTheAir updates:
    server.begin();              //Demarrage du serveur web /Web server start
    MDNS.begin(getHostname().c_str());
    MDNS.addService("http", "tcp", 80);
    DEBUG_print("Server started\n");
  }
#endif
}

// **************************************** LOOP *************************************************
void loop(){
#ifdef DEFAULTWIFIPASS
  if(String(DEFAULTWIFIPASS).length()){  //Traitement des requetes /HTTP treatment
    server.handleClient();
    delay(1L);
    MDNS.update();
  }
#endif

  connectionTreatment();                //WiFi watcher
  interruptTreatment();                 //Gestion des switchs/Switchs management
  timersTreatment();                    //Timers control
  uartSwitchsTreatment();               //Slave messages traitement
  mySerialEvent();
}
// ***********************************************************************************************
