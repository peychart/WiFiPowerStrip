//Reference: https://www.arduino.cc/en/Reference/HomePage
//See: http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
//Librairies et cartes ESP8266 sur IDE Arduino: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//http://arduino-esp8266.readthedocs.io/en/latest/
//JSon lib: see https://github.com/bblanchon/ArduinoJson.git
//peychart@netcourrier.com 20171021
// Licence: GNU v3
#include <FS.h>

#include "settings.h"   //Can be adjusted according to the project...

//Avoid to change the following:
#define DEBOUNCE_TIME      100L
String  hostname = DEFAULTHOSTNAME; //Can be change by interface
String  ssid[SSIDMax()];            //Identifiants WiFi /Wifi idents
String  password[SSIDMax()];        //Mots de passe WiFi /Wifi passwords
bool    WiFiAP=false,      outputValue[outputCount()];
unsigned short             nbWifiAttempts=MAXWIFIRETRY, WifiAPTimeout;
unsigned long              next_reconnect(0L), maxDurationOn[outputCount()], timerOn[outputCount()];
volatile unsigned short    intr(0);
volatile unsigned long     rebounds_completed;

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Webserver:
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
ESP8266WebServer        server(80);
//WiFiServer        server(80);
#include <ESP8266HTTPUpdateServer.h>
ESP8266HTTPUpdateServer httpUpdater;

void sendHTML(){    // See comments at the end of this fonction definition...
  String s;
  s = (F("<!DOCTYPE HTML>\n<html lang='us-US'>\n<head><meta charset='utf-8'/>\n<title>"));
  s+= hostname;
  s+= F("</title>\n<style>body{background-color:#fff7e6; font-family:Arial,Helvetica,Sans-Serif; Color:#000088;}\n");
  s+= F(" ul{list-style-type:square;} li{padding-left:5px; margin-bottom:10px;}\n");
  s+= F(" td{text-align:left; min-width:110px; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n");
  s+= F(" .modal{display:none; position:fixed; z-index:1; left:0%; top:0%; height:100%; width:100%; overflow:scroll; background-color:#000000;}\n");
  s+= F(" .modal-content{background-color:#fff7e6; margin:5% auto; padding:15px; border:2px solid #888; height:90%; width:90%; min-height:755px;}\n");
  s+= F(" .close{color:#aaa; float:right; font-size:30px; font-weight:bold;}\n");
  s+= F(" .delayConf{float: left; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n");
  s+= F(" .duration{width:50px;}\n  /");
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
  for(uint8_t i(0); outputCount();){
    s+= outputName[i] + "=" + (outputValue[i] ?"true" :"false");
    if(++i>=outputCount()) break;
    s+= "&";
  }
  s+= F("';e.href=e.innerHTML;\ne=document.getElementById('example2');e.innerHTML=document.URL+'plugValues';e.href=e.innerHTML;\n");
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
  s+= F("if(f.children[i].type=='text')\n");
  s+= F("if(f.children[i].value!=''){\n");
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
  s+= String(DEFAULTHOSTNAME) + "/" + DEFAULTWIFIPASS;
  s+= F("\" on 192.168.4.1).<br><br><h2><form method='POST'>\nNetwork name: <input type='text' name='hostname' value='");
  s+= hostname;
  s+= F("' style='width:110;'>\n <input type='button' value='Submit' onclick='submit();'>\n</form></h2>\n");
  s+= F("<h2>Network connection:</h2>\n");
  s+= F("<table style='width:100%'><tr>");
  for(uint8_t i(0); i<SSIDMax(); i++){
    s+= "<td><div><form method='POST'>\nSSID " + String(i+1, DEC) + ":<br><input type='text' name='SSID' value='" + ssid[i] + (ssid[i].length() ?"' readonly><br>\n": "'><br>\n");
    s+= F("Password:<br><input type='password' name='password' value='");
    s+= password[i];
    s+= F("'><br>\nConfirm password:<br><input type='password' name='confirm' value='");
    s+= password[i];
    s+= F("'><br><br>\n<input type='button' value='Submit' onclick='saveSSID(this);'>");
    s+= F("<input type='button' value='Remove' onclick='deleteSSID(this);'>\n</form></div></td>");
 }s+= F("</tr></table>\n");
  s+= F("<h2><form method='POST'>Names of Plugs: ");
  for(uint8_t i(0); i<outputCount(); i++)
    s+= "<input type='text' name='plugName" + String(i, DEC) + "' value='" + outputName[i] + "' style='width:70;'>";
  s+= F(" - <input type='button' value='Submit' onclick='submit();'></form></h2>\n");
  s+= F("<h6><a href='update' onclick='javascript:event.target.port=80'>Firmware update</a>");
  s+= F(" - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>");
  s+= F("</div></div>\n");
  s+= F("<table style='border:0;width:100%;'><tbody><tr><td><h1>");
  s+= hostname + " - " + (WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString()) + " [" + WiFi.macAddress();
  s+= F("] :</h1></td><td style='text-align:right;vertical-align:top;'><p><span class='close' onclick='showHelp();'>?</span></p></td>");
  s+= F("<tr></tbody></table>\n<h3>Status :</h3>\n");
  s+= F("<form id='switchs' method='POST'><ul>\n");
  for (uint8_t i=0; i<outputCount(); i++){ bool display;
    // Tittle:
    s+= "<li><table><tbody>\n<tr><td>" + outputName[i];
    // Switch:
    s+= F("</td><td>\n<div class='onoffswitch delayConf'>\n<input type='checkbox' class='onoffswitch-checkbox' id='");
    s+= outputName[i] + "' name='" + outputName[i] + "' " + String(outputValue[i] ?"checked" :"");
    s+= " onClick='switchSubmit(this);'>\n<label class='onoffswitch-label' for='" + outputName[i];
    s+= F("'><span class='onoffswitch-inner'></span><span class='onoffswitch-switch'></span></label>\n</div>\n<div class='delayConf'>&nbsp;&nbsp;&nbsp;(will be 'ON' during:&nbsp;\n");
    // Days duration:
    display=( (long)maxDurationOn[i]!=(-1L) && (maxDurationOn[i]/86400L)>0L );
    s+= "<input type='number' name='" + outputName[i] + "-max-duration-d' value='" + String(display ?(maxDurationOn[i]/86400L) :0L, DEC);
    s+= F("' min='0' max='31' data-unit=86400 class='duration' style='width:60px;display:");
    s+= String(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (display ?"d &nbsp;\n" :"\n");
    // Hours duration:
    display|=( (long)maxDurationOn[i]!=(-1L) && (maxDurationOn[i]%86400L/3600L)>0L );
    s+= "<input type='number' name='" + outputName[i] + "-max-duration-h' value='" + String(display ?(maxDurationOn[i]%86400L/3600L) :0L, DEC);
    s+= "' min='0' max='24' data-unit=3600 class='duration' style='display:" + String(display ?"inline-block" :"none");
    s+= ";' onChange='checkDelay(this);'>" + String(display ?"h &nbsp;\n" :"\n");
    // Minutes duration:
    display|=( (long)maxDurationOn[i]!=(-1L) && (maxDurationOn[i]%86400L%3600L/60L)>0L );
    s+= "<input type='number' name='" + outputName[i] + "-max-duration-mn' value='" + String(display ?(maxDurationOn[i]%86400L%3600L/60L) :0L, DEC);
    s+= "' min='-1' max='60' data-unit=60 class='duration' style='display:" + String(display ?"inline-block" :"none");
    s+= ";' onChange='checkDelay(this);'>" + String(display ?"mn &nbsp;\n" :"\n");
    // Secondes duration:
    s+= "<input type='number' name='" + outputName[i] + "-max-duration-s' value='";
    s+= ( ((long)maxDurationOn[i]!=(-1L)) ? String(maxDurationOn[i]%86400L%3600L%60L, DEC) : String(-1L, DEC) );
    s+= F("' min='-1' max='60' data-unit=1 class='duration' onChange='checkDelay(this);'>");
    s+= String( ((long)maxDurationOn[i]!=(-1L)) ?"s" :"-" );
    // End
    s+= F(")</div>\n</td></tr>\n</tbody></table></li>\n");
  } s+= F("</ul><div><input type='checkbox' name='newValue' id='newValue' checked style=\"display:none\"></div></form>\n</body>\n</html>\n");
  s+= "\n";
  server.send(200, "text/html", s);  // Open the stream...
}

bool WiFiHost(){
  Serial.println("\nNo custom SSID found: setting soft-AP configuration ... ");
  WifiAPTimeout=(WIFIAPDELAYRETRY/WIFISTADELAYRETRY); nbWifiAttempts=MAXWIFIRETRY;
  WiFi.mode(WIFI_AP);
//WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,254), IPAddress(255,255,255,0));
  WiFiAP=WiFi.softAP(DEFAULTHOSTNAME, DEFAULTWIFIPASS);
  Serial.println(
    WiFiAP
    ?(String("Connecting \"" + hostname+ "\" [") + WiFi.softAPIP().toString() + "] from: " + DEFAULTHOSTNAME + "/" + DEFAULTWIFIPASS + "\n").c_str()
    :"WiFi Timeout.\n");
  return WiFiAP;
}

bool WiFiConnect(){
  WiFi.softAPdisconnect(); WiFi.disconnect(); WiFiAP=false;
  delay(10);

  Serial.println("");
  for(uint8_t i(0); i<SSIDMax(); i++) if(ssid[i].length()){

    //Connection au reseau Wifi /Connect to WiFi network
    WiFi.mode(WIFI_STA);
    Serial.print(String("Connecting \"" + hostname+ "\" [") + String(WiFi.macAddress()) + "] to: " + ssid[i]);
    WiFi.begin(ssid[i].c_str(), password[i].c_str());

    //Attendre la connexion /Wait for connection
    for(uint8_t j(0); j<16 && WiFi.status()!=WL_CONNECTED; j++){
      delay(500);
      Serial.print(".");
    } Serial.println("");

    if(WiFi.status()==WL_CONNECTED){
      nbWifiAttempts=MAXWIFIRETRY;
      //Affichage de l'adresse IP /print IP address:
      Serial.println("WiFi connected");
      Serial.print("IP address: "); Serial.println(WiFi.localIP()); Serial.println("\n");
      return true;
    } WiFi.disconnect();
  }
  nbWifiAttempts--;
  if(ssid[0].length()){
    Serial.print("WiFi Timeout ("); Serial.print(nbWifiAttempts);
    Serial.println((nbWifiAttempts>1) ?" more attempts)." :" more attempt).");
  }if(!nbWifiAttempts){
    return WiFiHost();
  }return false;
}

bool readConfig(bool=true);
void writeConfig(){        //Save current config:
  if(!readConfig(false))
    return;
  File f=SPIFFS.open("/config.txt", "w+");
  if(f){
    f.println(ResetConfig);
    f.println(hostname);                   //Save hostname
    for(uint8_t i(0); i<SSIDMax(); i++){   //Save SSIDs
      if(!password[i].length()) ssid[i]="";
      f.println(ssid[i]);
      f.println(password[i]);
    } unsigned long v=millis();
    for(uint8_t i(0); i<outputCount(); i++){   //Save output states
      f.println(outputName[i]);
      f.println(outputValue[i]);
      f.println((long)maxDurationOn[i]);
      f.println( ( ((long)timerOn[i]==(-1L)) ?(-1L) :(long)((timerOn[i]<v) ?(~v+timerOn[i]) :(timerOn[i]-v)) ) );
    }Serial.println("SPIFFS writed.");
    f.close();
}  }

String readString(File f){ String ret=f.readStringUntil('\n'); ret.remove(ret.indexOf('\r')); return ret; }
inline bool getConfig(String& v, File f, bool w){String r=readString(f);               if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(bool&   v, File f, bool w){bool   r=atoi(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(int&    v, File f, bool w){int    r=atoi(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(long&   v, File f, bool w){long   r=atol(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
bool readConfig(bool w){      //Get config (return false if config is not modified):
  bool ret=false;
  File f=SPIFFS.open("/config.txt", "r");
  if(f && ResetConfig!=atoi(readString(f).c_str())){
    Serial.println("New configFile version...");
    f.close();
  }if(!f){    //Write default config:
    if(w){
      for(uint8_t i(0); i<SSIDMax(); i++) password[i]="";
      for(uint8_t i(0); i<outputCount(); i++){
        outputValue[i]=false; maxDurationOn[i]=(-1L);
      }SPIFFS.format(); writeConfig();
    Serial.println("SPIFFS initialized.");
    }return true;
  }ret|=getConfig(hostname, f, w);
  for(uint8_t i(0); i<SSIDMax(); i++){        //Get SSIDs
    ret|=getConfig(ssid[i], f, w);
    ret|=getConfig(password[i], f, w);
  } unsigned long m=millis();
  for(uint8_t i(0); i<outputCount(); i++){   //Get output states
    ret|=getConfig(outputName[i], f, w);
    ret|=getConfig(outputValue[i], f, w);
    ret|=getConfig((long&)maxDurationOn[i], f, w);
    ret|=getConfig((long&)timerOn[i], f, w); if( (long)maxDurationOn[i]!=(-1L) ) timerOn[i]+=m;
  }
  f.close();
  return ret;
}

void setPin(int i, bool v, bool force=false){
  if(outputValue[i]!=v || force){
    Serial.println("Set GPIO " + String(_outputPin[i], DEC) + "(" + outputName[i] + ") to " + (v ?"true" :"false"));
    digitalWrite(_outputPin[i], (outputValue[i]=v));
    timerOn[i]=(unsigned long)millis()+(1000L*maxDurationOn[i]);
    if(RESTO_VALUES) writeConfig();
} }

void handleSubmitSSIDConf(){           //Setting:
  uint8_t count=0;
  for(uint8_t i(0); i<SSIDMax(); i++) if(ssid[i].length()) count++;
  for(uint8_t i(0); i<count;     i++)
    if(ssid[i]==server.arg("SSID")){ //Modify password if SSID exist
      password[i]=server.arg("password");
      if(!password[i].length())      //Delete this ssid if no more password
        ssid[i]=="";
      if(!ssid[i].length()) readConfig();
      if(WiFiAP && ssid[0].length()) WiFiConnect();
      return;
    }
  if(count<SSIDMax()){                //Add ssid:
    ssid[count]=server.arg("SSID");
    password[count]=server.arg("password");
    if(WiFiAP && ssid[0].length()) WiFiConnect();
} }

inline bool handlePlugnameSubmit(uint8_t i){       //Set outputs names:
  if(server.hasArg("plugName"+(String)i) && server.arg("plugName"+(String)i))
    return(outputName[i]=server.arg("plugName"+(String)i));
  return false;
}

inline bool handleDurationOnSubmit(uint8_t i){ unsigned int v;        //Set outputs durations:
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

inline void handleValueSubmit(uint8_t i){      //Set outputs values:
  if(server.hasArg(outputName[i]) && outputValue[i]) // if param -> 1; else -> 0
    return;
  setPin(i, server.hasArg(outputName[i]));     // not arg if unchecked...
  return;
}

void  handleRoot(){ bool w;
  if((w=server.hasArg("hostname")))
    hostname=server.arg("hostname");                  //Set host name
  else if((w=server.hasArg("password")))
    handleSubmitSSIDConf();                           //Set WiFi connections
  else{
    for(uint8_t i(0); i<outputCount(); i++)
      w|=handlePlugnameSubmit(i);                     //Set plug name
    if(!w) for(uint8_t i(0); i<outputCount(); i++)
      w|=handleDurationOnSubmit(i);                   //Set timeouts
    if(!w && server.hasArg("newValue"))
      for(uint8_t i=(0); i<outputCount(); i++)
        handleValueSubmit(i);                         //Set values
  }if(w) writeConfig();
  sendHTML();
}

String getPlugNames(){
  String s="";
  for(uint8_t i(0); outputCount(); ){
    s += outputName[i];
    if((++i)>=outputCount()) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void  setPlugNames(){
  String v;
  for(uint8_t i(0); i<outputCount(); i++){
    v=outputName[i]; v.toLowerCase();
    if ((v=server.arg(v))!=""){
      v.toLowerCase();
      outputName[i] = v;
} } }

String getPlugTimers(){
  String s="";
  for(uint8_t i(0); outputCount(); ){
    s += maxDurationOn[i];
    if((++i)>=outputCount()) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void   setPlugTimers(){
  String v;
  for(uint8_t i(0); i<outputCount(); i++){
    v=outputName[i]; v.toLowerCase();
    if ((v=server.arg(v))!=""){
      v.toLowerCase();
      timerOn[i] += atol(v.c_str()) * 1000L;
} } }

String getPlugValues(){
  String s="";
  for(uint8_t i(0); outputCount(); ){
    s += outputValue[i];
    if((++i)>=outputCount()) break;
    s += ",";
  }return s;    //Format: nn,nn,nn,nn,nn,...
}

void   setPlugValues(){
  String v;
  for(uint8_t i(0); i<outputCount(); i++){
    v=outputName[i]; v.toLowerCase();
    if ((v=server.arg(v))!=""){
      v.toLowerCase();
      setPin(i, ((v=="1" || v=="true") ?1 :0));
} } }

//Gestion des switchs/Switchs management
void ICACHE_RAM_ATTR debounceInterrupt(){ if(!intr++) rebounds_completed=(unsigned long)millis()+DEBOUNCE_TIME; }

void setup(){
  Serial.begin(115200);

  //Definition des URL d'entree /Input URL definition
  server.on("/", [](){handleRoot(); server.client().stop();});
  server.on("/plugNames",  [](){setPlugNames();  server.send(200, "text/plain", "[" + getPlugNames()  + "]");});
  server.on("/plugTimers", [](){setPlugTimers(); server.send(200, "text/plain", "[" + getPlugTimers() + "]");});
  server.on("/plugValues", [](){setPlugValues(); server.send(200, "text/plain", "[" + getPlugValues() + "]");});
//server.on("/about", [](){ server.send(200, "text/plain", getHelp()); });
  server.onNotFound([](){server.send(404, "text/plain", "404: Not found");});

  Serial.println("\nHello World!");
  // Webserver:
  MDNS.begin(hostname.c_str());
  httpUpdater.setup(&server);  //Adds OnTheAir updates:
  server.begin();              //Demarrage du serveur web /Web server start
  Serial.println("Server started");

  //Open config:
  SPIFFS.begin(); readConfig();

  //initialisation des broches /pins init
  for(uint8_t i(0); i<outputCount(); i++){    //Sorties/ouputs:
    pinMode(_outputPin[i], OUTPUT);
    digitalWrite(_outputPin[i], outputValue[i]);
  }for(uint8_t i(0); i<inputCount(); i++){   //Entrées/inputs:
    pinMode(_inputPin[i], INPUT_PULLUP);    //only this mode works on all inputs !...
    //See: https://www.arduino.cc/en/Reference/attachInterrupt
    // or: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
    attachInterrupt(_inputPin[i], debounceInterrupt, FALLING);
    //attachInterrupt(_inputPin[i], debounceInterrupt, CHANGE);
  }
  WiFi.softAPdisconnect(); WiFi.disconnect(); WiFiAP=false;
}

//Because of millis() rollover:
inline bool isNow(unsigned long v) {unsigned long ms(millis()); return((v<ms) && (ms-v)<600000L);}

void loop(){
  server.handleClient(); delay(1L);/*for D1 mini*/             //Traitement des requetes /HTTP treatment

  if(isNow(next_reconnect)) {
    next_reconnect=(unsigned long)millis()+WIFISTADELAYRETRY;  //Test connexion/Check WiFi every mn:
  //Serial.println(ESP.getFreeHeap());
    if(ESP.getFreeHeap()<20000) {writeConfig(); ESP.restart();}
    if( (WiFi.status()!=WL_CONNECTED) || (WiFiAP && ssid[0].length() && !WifiAPTimeout--) )
      WiFiConnect();
  }

  if( intr && isNow(rebounds_completed) ) {                    //Interrupt treatment:
    uint8_t n; uint16_t reg=GPI;
    for(uint8_t i(n=0); i<inputCount(); i++) if( (reg & (1<<(_inputPin[i] & 0xF)))==0 ) n+=(1<<i);
    if(--n<outputCount()) setPin(n, !outputValue[n]);
    intr=0;
  }

  for(uint8_t i(0); i<outputCount(); i++)                      //Tiners control:
    if( outputValue[i] && (long)maxDurationOn[i]!=(-1L) && isNow(timerOn[i]) ) {
      Serial.println("Timeout(" + String(maxDurationOn[i], DEC) + "s) on GPIO " + String(_outputPin[i], DEC) + ":");
      setPin(i, false);
    }
}
