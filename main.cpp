//Reference: https://www.arduino.cc/en/Reference/HomePage
//See: http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
//Librairies et cartes ESP8266 sur IDE Arduino: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//http://arduino-esp8266.readthedocs.io/en/latest/
//JSon lib: see https://github.com/bblanchon/ArduinoJson.git
//peychart@netcourrier.com 20171021
// Licence: GNU v3
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FS.h>

//Ajust the following:
#define LOOPDELAY          10L
#define DEBOUNCE_DELAY     100L
uint8_t ResetConfig =      1;     //Change this value to reset current config on the next boot...
#define DEFAULTHOSTNAME    "ESP8266"
#define DEFAULTWIFIPASS    "defaultPassword"
#define WIFITRYDELAY       60000L
#define MAXWIFIERRORS      3
#define WIFIAPDELAY        10
#define SSIDMax()          3
#define inputCount()       3
// Restore output values after a reboot:
#define RESTO_VALUES       false
#define outputCount()      5
//String  outputName[outputCount()] = {"Yellow", "Orange", "Red", "Green", "Blue", "White"}; //can be change by interface...
//int _outputPin[outputCount()]      = {  D0,       D1,      D2,      D3,     D4,      D8  };
String  outputName[outputCount()] = {"Yellow", "Green", "Orange", "Red", "Blue" }; //can be change by interface...
int     _outputPin[outputCount()] = {  D1,       D2,      D3,      D4,     D0  };
int     _inputPin [inputCount()]  = {  D5,       D6,      D7  };

//Avoid to change the following:
String  hostname = DEFAULTHOSTNAME; //Can be change by interface
String  ssid[SSIDMax()];            //Identifiants WiFi /Wifi idents
String  password[SSIDMax()];        //Mots de passe WiFi /Wifi passwords
bool    WiFiAP=false,   outputValue[outputCount()];
unsigned short          nbWifiAttempts=MAXWIFIERRORS, WifiAPTimeout;
unsigned long           maxDurationOn[outputCount()], timerOn[outputCount()];
volatile unsigned short intr(0);
volatile unsigned long  last_intr;
unsigned long           ms, previous_ms(0L);

// Webserver:
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
ESP8266WebServer        server(80);
#include <ESP8266HTTPUpdateServer.h>
ESP8266HTTPUpdateServer httpUpdater;

String getMyMacAddress(){
  char ret[18];
  byte mac[6]; WiFi.macAddress(mac);
  sprintf(ret, "%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return ret;
}

void sendHTML(){    // See comments at the end of this fonction definition...
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);  // Well named setup...
  server.send(200, "text/html", "");  // Open the stream...
  server.sendContent(F("<!DOCTYPE HTML>\n<html lang='us-US'>\n<head><meta charset='utf-8'/>\n<title>"));
  server.sendContent(hostname);
  server.sendContent(F("</title>\n"));
  server.sendContent(F("<style>body{background-color:#fff7e6; font-family:Arial,Helvetica,Sans-Serif; Color:#000088;}\n"));
  server.sendContent(F(" ul{list-style-type:square;} li{padding-left:5px; margin-bottom:10px;}\n"));
  server.sendContent(F(" td{text-align:left; min-width:110px; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n"));
  server.sendContent(F(" .modal{display:none; position:fixed; z-index:1; left:0%; top:0%; height:100%; width:100%; overflow:scroll; background-color:#000000;}\n"));
  server.sendContent(F(" .modal-content{background-color:#fff7e6; margin:5% auto; padding:15px; border:2px solid #888; height:90%; width:90%; min-height:755px;}\n"));
  server.sendContent(F(" .close{color:#aaa; float:right; font-size:30px; font-weight:bold;}\n"));
  server.sendContent(F(" .delayConf{float: left; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n"));
  server.sendContent(F(" .duration{width:50px;}\n"));
  server.sendContent(F("  /*see: https://proto.io/freebies/onoff/: */\n"));
  server.sendContent(F(" .onoffswitch {position: relative; width: 90px;-webkit-user-select:none; -moz-user-select:none; -ms-user-select: none;}\n"));
  server.sendContent(F(" .onoffswitch-checkbox {display: none;}\n"));
  server.sendContent(F(" .onoffswitch-label {display: block; overflow: hidden; cursor: pointer;border: 2px solid #999999; border-radius: 20px;}\n"));
  server.sendContent(F(" .onoffswitch-inner {display: block; width: 200%; margin-left: -100%;transition: margin 0.3s ease-in 0s;}\n"));
  server.sendContent(F(" .onoffswitch-inner:before, .onoffswitch-inner:after {display: block; float: left; width: 50%; height: 30px; padding: 0; line-height: 30px;font-size: 14px; color: white; font-family: Trebuchet, Arial, sans-serif; font-weight: bold;box-sizing: border-box;}\n"));
  server.sendContent(F(" .onoffswitch-inner:before{content: 'ON';padding-left: 10px;background-color: #34C247; color: #FFFFFF;}\n"));
  server.sendContent(F(" .onoffswitch-inner:after {content: 'OFF';padding-right: 10px;background-color: #EEEEEE; color: #999999;text-align: right;}\n"));
  server.sendContent(F(" .onoffswitch-switch{display: block; width: 18px; margin: 6px;background: #FFFFFF;position: absolute; top: 0; bottom: 0;right: 56px;border: 2px solid #999999; border-radius: 20px;transition: all 0.3s ease-in 0s;}\n"));
  server.sendContent(F(" .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-inner {margin-left: 0;}\n"));
  server.sendContent(F(" .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-switch {right: 0px;}\n"));
  server.sendContent(F("</style></head>\n<body onload='init();'>\n"));
  server.sendContent(F("<script>\nthis.timer=0;\n"));
  server.sendContent(F("function init(){refresh();}\n"));
  server.sendContent(F("function refresh(v=30){\n clearTimeout(this.timer); document.getElementById('about').style.display='none';\n"));
  server.sendContent(F(" this.timer=setTimeout(function(){RequestStatus();refresh(v);}, v*1000);}\n"));
  server.sendContent(F("function RequestStatus(){var j, ret, e, req=new XMLHttpRequest();req.open('GET',document.URL+'plugValues',false); "));
  server.sendContent(F("req.send(null);ret=req.responseText;\nif((j=ret.indexOf('[')) >= 0){\n"));
  server.sendContent(F("if((e=document.getElementsByClassName('onoffswitch-checkbox')).length && (j=ret.indexOf('['))>=0)\n"));
  server.sendContent(F(" for(var e,v,i=0,r=ret.substr(j+1);r[0] && r[0]!=']';i++){\n"));
  server.sendContent(F("  j=r.indexOf(',');if(j<0) j=r.indexOf(']');v=parseInt(r.substr(0,j));\n"));
  server.sendContent(F("  if(v>=0) e[i].checked=(v?true:false);r=r.substr(j+1);\n"));
  server.sendContent(F(" }if((e=document.getElementsByClassName('onoffswitch-checkbox')).length)\n"));
  server.sendContent(F("  for(var e,v,i=0,ret=r.substr(j+1);r[0] && r[0]!=']';i++){\n"));
  server.sendContent(F("   j=r.indexOf(',');if(j<0) j=r.indexOf(']');v=parseInt(r.substr(0,j));\n"));
  server.sendContent(F("   if(v>=0) e[i].checked=(v?true:false);r=r.substr(j+1);\n"));
  server.sendContent(F("}}}\n"));
  server.sendContent(F("function showHelp(){var e;\ne=document.getElementById('example1');e.innerHTML=document.URL+'plugValues?"));
  for(uint8_t i(0); outputCount();){
    server.sendContent(outputName[i] + "=" + (outputValue[i] ?"true" :"false"));
    if(++i>=outputCount()) break;
    server.sendContent(F("+"));
  }
  server.sendContent(F("';e.href=e.innerHTML;\ne=document.getElementById('example2');e.innerHTML=document.URL+'plugValues';e.href=e.innerHTML;\n"));
  server.sendContent(F("refresh(120);document.getElementById('about').style.display='block';}\n"));
  server.sendContent(F("function saveSSID(f){\nif((f=f.parentNode)){var s, p=false;\n"));
  server.sendContent(F("for(var i=0;i<f.children.length;i++){\n"));
  server.sendContent(F("if(f.children[i].type=='password'){\n"));
  server.sendContent(F("if (!p) p=f.children[i];\nelse if(p.value!=f.children[i].value) p.value='';\n"));
  server.sendContent(F("}else if(f.children[i].type=='text') s=f.children[i];\n"));
  server.sendContent(F("}if(s.value==''){\nalert('Empty SSID...');f.reset();s.focus();\n"));
  server.sendContent(F("}else if(p.value==''){\nvar ssid=s.value;f.reset();s.value=ssid;\nalert('Incorrect password...');p.focus();\n"));
  server.sendContent(F("}else f.submit();\n"));
  server.sendContent(F("}}\nfunction deleteSSID(f){\n"));
  server.sendContent(F("if((f=f.parentNode))\nfor(var i=0;i<f.children.length;i++)\n"));
  server.sendContent(F("if(f.children[i].type=='text')\n"));
  server.sendContent(F("if(f.children[i].value!=''){\n"));
  server.sendContent(F("if(confirm('Are you sure to remove this SSID?')){\n"));
  server.sendContent(F("for(var i=0;i<f.children.length;i++)\nif(f.children[i].type=='password') f.children[i].value='';\n"));
  server.sendContent(F("f.submit();\n"));
  server.sendContent(F("}}else alert('Empty SSID...');\n"));
  server.sendContent(F("}\n"));
  server.sendContent(F("function switchSubmit(e){refresh();\n"));
  server.sendContent(F(" var b=false, l=e.parentNode.parentNode.getElementsByTagName('input');\n"));
  server.sendContent(F(" for(var i=0;i<l.length;i++) if(l[i].type=='number')\n   b|=(l[i].value!='0');\n"));
  server.sendContent(F(" e.checked&=b;document.getElementById('switchs').submit();\n"));
  server.sendContent(F("}\n"));
  server.sendContent(F("var checkDelaySubmit=0;\n"));
  server.sendContent(F("function checkDelay(e){refresh();\n"));
  server.sendContent(F(" if(e.value=='-1'){\n"));
  server.sendContent(F("  var l=e.parentNode.getElementsByTagName('input');\n"));
  server.sendContent(F("  for(var i=0;i<l.length;i++)\n   if(l[i].className=='duration'){\n    if(l[i].getAttribute('data-unit')!=1)\n     l[i].style.display='none';\n     else l[i].style.display='inline-block';}\n"));
  server.sendContent(F(" }clearTimeout(this.checkDelaySubmit);this.checkDelaySubmit=setTimeout(function(){this.checkDelaySubmit=0;document.getElementById('switchs').submit();}, 1000);\n"));
  server.sendContent(F("}\n"));
  server.sendContent(F("</script>\n<div id='about' class='modal'><div class='modal-content'>"));
  server.sendContent(F("<span class='close' onClick='refresh();'>&times;</span>"));
  server.sendContent(F("<h1>About</h1>"));
  server.sendContent(F("This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domotics or Jeedom.<br><br>"));
  server.sendContent(F("In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). "));
  server.sendContent(F("Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs, like this:"));
  server.sendContent(F("<a id='example1' style='padding:0 0 0 5px;'></a><br><br>"));
  server.sendContent(F("The state of the electrical outlets can also be requested from the following URL: "));
  server.sendContent(F("<a id='example2' style='padding:0 0 0 5px;'></a><br><br>"));
  server.sendContent(F("The status of the power strip is retained when the power is turned off and restored when it is turned on ;a power-on duration can be set on each output: (-1) no delay, (0) to disable an output and (number of s) to configure the power-on duration.<br><br>"));
  server.sendContent(F("The following allows you to configure some parameters of the Wifi Power Strip (until a SSID is set and reached, the socket works as an access point with its own SSID and default password: \""));
  server.sendContent(DEFAULTHOSTNAME+String("/")+DEFAULTWIFIPASS);
  server.sendContent(F("\" on 192.168.4.1).<br><br>"));
  server.sendContent(F("<h2><form method='POST'>\n"));
  server.sendContent(F("Network name: <input type='text' name='hostname' value='"));
  server.sendContent(hostname);
  server.sendContent(F("' style='width:110;'>\n"));
  server.sendContent(F(" <input type='button' value='Submit' onclick='submit();'>\n"));
  server.sendContent(F("</form></h2>\n"));
  server.sendContent(F("<h2>Network connection:</h2>\n"));
  server.sendContent(F("<table style='width:100%'><tr>"));
  for(uint8_t i(0); i<SSIDMax(); i++){
    server.sendContent(F("<td><div><form method='POST'>\nSSID "));
    server.sendContent(String(i+1, DEC));
    server.sendContent(F(":<br><input type='text' name='SSID' value='"));
    server.sendContent(ssid[i] + (ssid[i].length() ?"' readonly><br>\n": "'><br>\n"));
    server.sendContent(F("Password:<br><input type='password' name='password' value='"));
    if(ssid[i].length()) server.sendContent(password[i]);
    server.sendContent(F("'><br>\nConfirm password:<br><input type='password' name='confirm' value='"));
    if(ssid[i].length()) server.sendContent(password[i]);
    server.sendContent(F("'><br><br>\n<input type='button' value='Submit' onclick='saveSSID(this);'>"));
    server.sendContent(F("<input type='button' value='Remove' onclick='deleteSSID(this);'>\n"));
    server.sendContent(F("</form></div></td>"));
 }server.sendContent(F("</tr></table>\n"));
  server.sendContent(F("<h2><form method='POST'>Names of Plugs: "));
  for(uint8_t i(0); i<outputCount(); i++)
    server.sendContent("<input type='text' name='plugName" + String(i, DEC) + "' value='" + outputName[i] + "' style='width:70;'>");
  server.sendContent(F(" - <input type='button' value='Submit' onclick='submit();'></form></h2>\n"));
  server.sendContent(F("<h6><a href='update' onclick='javascript:event.target.port=80'>Firmware update</a>"));
  server.sendContent(F(" - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>"));
  server.sendContent(F("</div></div>\n"));
  server.sendContent(F("<table style='border:0;width:100%;'><tbody><tr><td><h1>"));
  server.sendContent(hostname + " - " + (WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString()) + " [" + getMyMacAddress());
  server.sendContent(F("] :</h1></td><td style='text-align:right;vertical-align:top;'><p><span class='close' onclick='showHelp();'>?</span></p></td>"));
  server.sendContent(F("<tr></tbody></table>\n"));
  server.sendContent(F("<h3>Status :</h3>\n"));
  server.sendContent(F("<form id='switchs' method='POST'><ul>\n"));
  for (uint8_t i=0; i<outputCount(); i++){ bool display;
    // Tittle:
    server.sendContent(F("<li><table><tbody>\n<tr><td>"));
    server.sendContent(outputName[i]);
    // Switch:
    server.sendContent(F("</td><td>\n<div class='onoffswitch delayConf'>\n<input type='checkbox' class='onoffswitch-checkbox' id='"));
    server.sendContent(outputName[i] + "' name='" + outputName[i] + "' " + String(outputValue[i] ?"checked" :""));
    server.sendContent(F(" onClick='switchSubmit(this);'>\n<label class='onoffswitch-label' for='"));
    server.sendContent(outputName[i]);
    server.sendContent(F("'><span class='onoffswitch-inner'></span><span class='onoffswitch-switch'></span></label>\n</div>\n<div class='delayConf'>&nbsp;&nbsp;&nbsp;(will be 'ON' during:&nbsp;\n"));
    // Days duration:
    display=( maxDurationOn[i]!=(unsigned long)(-1L) && (maxDurationOn[i]/86400L)>0L );
    server.sendContent(F("<input type='number' name='"));
    server.sendContent(outputName[i]);
    server.sendContent(F("-max-duration-d' value='"));
    server.sendContent(String(display ?(maxDurationOn[i]/86400L) :0L, DEC));
    server.sendContent(F("' min='0' max='366' data-unit=86400 class='duration' style='width:60px;display:"));
    server.sendContent(String(display ?"inline-block" :"none"));
    server.sendContent(F(";' onChange='checkDelay(this);'>"));
    server.sendContent(String(display ?"d &nbsp;\n" :"\n"));
    // Hours duration:
    display|=( maxDurationOn[i]!=(unsigned long)(-1L) && (maxDurationOn[i]%86400L/3600L)>0L );
    server.sendContent(F("<input type='number' name='"));
    server.sendContent(outputName[i]);
    server.sendContent(F("-max-duration-h' value='"));
    server.sendContent(String(display ?(maxDurationOn[i]%86400L/3600L) :0L, DEC));
    server.sendContent(F("' min='0' max='24' data-unit=3600 class='duration' style='display:"));
    server.sendContent(String(display ?"inline-block" :"none"));
    server.sendContent(F(";' onChange='checkDelay(this);'>"));
    server.sendContent(String(display ?"h &nbsp;\n" :"\n"));
    // Minutes duration:
    display|=( maxDurationOn[i]!=(unsigned long)(-1L) && (maxDurationOn[i]%86400L%3600L/60L)>0L );
    server.sendContent(F("<input type='number' name='"));
    server.sendContent(outputName[i]);
    server.sendContent(F("-max-duration-mn' value='"));
    server.sendContent(String(display ?(maxDurationOn[i]%86400L%3600L/60L) :0L, DEC));
    server.sendContent(F("' min='-1' max='60' data-unit=60 class='duration' style='display:"));
    server.sendContent(String(display ?"inline-block" :"none"));
    server.sendContent(F(";' onChange='checkDelay(this);'>"));
    server.sendContent(String(display ?"mn &nbsp;\n" :"\n"));
    // Secondes duration:
    server.sendContent(F("<input type='number' name='"));
    server.sendContent(outputName[i]);
    server.sendContent(F("-max-duration-s' value='"));
    server.sendContent(String((maxDurationOn[i]!=(unsigned long)(-1L)) ?(maxDurationOn[i]%86400L%3600L%60L) :-1L, DEC));
    server.sendContent(F("' min='-1' max='60' data-unit=1 class='duration' onChange='checkDelay(this);'>"));
    server.sendContent(String((maxDurationOn[i]!=(unsigned long)(-1L)) ?"s\n" :"-\n"));
    // End
    server.sendContent(F(")</div>\n</td></tr>\n</tbody></table></li>\n"));
  } server.sendContent(F("</ul><div><input type='checkbox' name='newValue' id='newValue' checked style=\"display:none\"></div></form>\n</body>\n</html>\n"));
  server.sendContent("");  // Stop the stream,
  server.client().flush();
  server.client().stop();  // Stop is needed because we sent no content length
}                          // The use of F()function allows to expel the const char* of the RAM towards the flash, freeing up working memory...

bool WiFiHost(){
  Serial.println();
  Serial.println("No custom SSID found: setting soft-AP configuration ... ");
  WifiAPTimeout=WIFIAPDELAY;
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,254), IPAddress(255,255,255,0));
  WiFiAP=WiFi.softAP(DEFAULTHOSTNAME, DEFAULTWIFIPASS);
  Serial.println(
    WiFiAP
    ?(String("Connecting \"" + hostname+ "\" [") + WiFi.softAPIP().toString() + "] from: " + DEFAULTHOSTNAME + "/" + DEFAULTWIFIPASS).c_str()
    :"WiFi Timeout."); Serial.println();
  return WiFiAP;
}

bool WiFiConnect(){
  WiFi.disconnect(); WiFi.softAPdisconnect(); WiFiAP=false;
  delay(10);

  Serial.println();
  for(uint8_t i(0); i<SSIDMax(); i++) if(ssid[i].length()){

    //Connection au reseau Wifi /Connect to WiFi network
    Serial.print(String("Connecting \"" + hostname+ "\" [") + getMyMacAddress() + "] to: " + ssid[i]);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid[i].c_str(), password[i].c_str());

    //Attendre la connexion /Wait for connection
    for(uint8_t j(0); j<16 && WiFi.status()!=WL_CONNECTED; j++){
      Serial.print(".");
      delay(500);
    } Serial.println();

    if(WiFi.status()==WL_CONNECTED){
      nbWifiAttempts=MAXWIFIERRORS;
      //Affichage de l'adresse IP /print IP address:
      Serial.println("WiFi connected");
      Serial.print("IP address: "); Serial.println(WiFi.localIP());
      Serial.println();
      return true;
  } }
  nbWifiAttempts--;
  if(ssid[0].length()){
    Serial.print("WiFi Timeout ("); Serial.print(nbWifiAttempts);
    Serial.println((nbWifiAttempts>1) ?" more attempts)." :" more attempt).");
  }if(!nbWifiAttempts){
    nbWifiAttempts=MAXWIFIERRORS;
    return WiFiHost();
  }return false;
}

bool readConfig(bool=true);
void writeConfig(){        //Save current config:
  if(!readConfig(false))
    return;
  File f=SPIFFS.open("/config.txt", "w+");
  if(f){ uint8_t count;
    f.println(ResetConfig);
    f.println(hostname);                //Save hostname
    for(uint8_t j(count=0); j<SSIDMax(); j++){   //Save SSIDs
      if(!password[j].length()) ssid[j]="";
      if(ssid[j].length()){
        f.println(ssid[j]);
        f.println(password[j]);
        count++;
    } }while(count++<SSIDMax()) f.println();
    for(count=0; count<outputCount(); count++){     //Save output states
      f.println(outputName[count]);
      f.println(outputValue[count]);
      f.println(maxDurationOn[count]);
    }Serial.println("SPIFFS writed.");
    f.close();
}  }

String readString(File f){ String ret=f.readStringUntil('\n'); ret.remove(ret.indexOf('\r')); return ret; }
inline bool getConfig(String        &v, File f, bool w){String r=readString(f);               if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(bool          &v, File f, bool w){bool   r=atoi(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(int           &v, File f, bool w){int    r=atoi(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
inline bool getConfig(long          &v, File f, bool w){long   r=atol(readString(f).c_str()); if(r==v) return false; if(w)v=r; return true;}
bool readConfig(bool w){      //Get config (return false if config is not modified):
  bool ret=false;
  File f=SPIFFS.open("/config.txt", "r");
  if(f && ResetConfig!=atoi(readString(f).c_str())) f.close();
  if(!f){    //Write default config:
    if(w){
      ssid[0]=""; password[0]=DEFAULTWIFIPASS;
      for(uint8_t i(0); i<outputCount(); i++){
        maxDurationOn[i]=(unsigned long)(-1L); outputValue[i]=false;
      }SPIFFS.format(); writeConfig();
    Serial.println("SPIFFS initialized.");
    }return true;
  }ret|=getConfig(hostname, f, w);
  for(uint8_t i(0); i<SSIDMax(); i++){        //Get SSIDs
    ret|=getConfig(ssid[i], f, w);
    if(ssid[i].length())
      ret|=getConfig(password[i], f, w);
  }
  if(!ssid[0].length()) password[0]=DEFAULTWIFIPASS; //Default values
  for(uint8_t i(0); i<outputCount(); i++){   //Get output states
    ret|=getConfig(outputName[i], f, w);
    ret|=getConfig(outputValue[i], f, w);
    ret|=getConfig((long&)maxDurationOn[i], f, w);
  }
  f.close();
  return ret;
}

void setPin(int i, bool v, bool force=false){
  if(outputValue[i]!=v || force){
    Serial.println("Set GPIO " + String(_outputPin[i], DEC) + "(" + outputName[i] + ") to " + (v ?"true" :"false"));
    digitalWrite(_outputPin[i], (outputValue[i]=v));
    timerOn[i]=(v ?(ms+(1000L*maxDurationOn[i])) :-1L);
    if(RESTO_VALUES) writeConfig();
} }

void ICACHE_RAM_ATTR debounceInterrupt(){    //Gestion des switchs/Switchs management
  if(!intr++) last_intr=ms;
}

void handleSubmitSSIDConf(){           //Setting:
  uint8_t count=0;
  for(uint8_t i(0); i<SSIDMax(); i++) if(ssid[i].length()) count++;
  for(uint8_t i(0); i<count;     i++)
    if(ssid[i]==server.arg("SSID")){
      password[i]=server.arg("password");
      if(WiFiAP) WiFiConnect();
      return;}
  if(count<SSIDMax()){                //Add ssid:
    ssid[count]=server.arg("SSID");
    password[count]=server.arg("password");
  } if(WiFiAP && ssid[0].length()) WiFiConnect();
}

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
  if(!server.hasArg("newValue"))
    return;                                    // It's a new connection...
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
    if(!w) for(uint8_t i=(0); i<outputCount(); i++)
      handleValueSubmit(i);                           //Set values
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

void setup(){
  Serial.begin(115200); delay(10);
  Serial.println(); Serial.println("Hello World!");

  //Definition des URL d'entree /Input URL definition
  server.on("/", handleRoot);
  server.on("/plugNames",  [](){setPlugNames();  server.send(200, "text/plain", "[" + getPlugNames()  + "]");});
  server.on("/plugTimers", [](){setPlugTimers(); server.send(200, "text/plain", "[" + getPlugTimers() + "]");});
  server.on("/plugValues", [](){setPlugValues(); server.send(200, "text/plain", "[" + getPlugValues() + "]");});
//server.on("/about", [](){ server.send(200, "text/plain", getHelp()); });

  // Webserver:
  MDNS.begin(hostname.c_str());
  httpUpdater.setup(&server);  //Adds OnTheAir updates:
  server.begin();              //Demarrage du serveur web /Web server start
  Serial.println("Server started");

  //Open config:
  SPIFFS.begin();

  //initialisation des broches /pins init
  readConfig();
  for(uint8_t i(0); i<outputCount(); i++){    //Sorties/ouputs:
    pinMode(_outputPin[i], OUTPUT);
    setPin(i, (RESTO_VALUES ?outputValue[i] :(outputValue[i]=false)), true);
  }for(uint8_t i(0); i<inputCount(); i++){   //Entrées/inputs:
    pinMode(_inputPin[i], INPUT_PULLUP);    //only this mode works on all inputs !...
    //See: https://www.arduino.cc/en/Reference/attachInterrupt
    // or: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
    attachInterrupt(_inputPin[i], debounceInterrupt, FALLING);
    //attachInterrupt(_inputPin[i], debounceInterrupt, CHANGE);
} }

void loop(){
  server.handleClient();                                          //Traitement des requetes /HTTP treatment

  ms=millis();
  if(ms-previous_ms>WIFITRYDELAY){ previous_ms=ms;                //Test connexion/Check WiFi every mn
    if( ((WiFi.status()!=WL_CONNECTED) && !WiFiAP) || (WiFiAP && ssid[0].length() && !WifiAPTimeout--) )
      WiFiConnect();
  }

  if( intr && ((ms-last_intr) >= DEBOUNCE_DELAY) ) {              // Interrupt treatment:
    uint8_t n; uint16_t reg=GPI;
    for(uint8_t i(n=0); i<inputCount(); i++) if( (reg & (1<<(_inputPin[i] & 0xF)))==0 ) n+=(1<<i);
    if(--n<outputCount()) setPin(n, !outputValue[n]);
    intr=0;  //Debounce
  }

  for(uint8_t i(0); i<outputCount(); i++) if( ms>timerOn[i] ){    //Tiners control
    Serial.println("Timeout(" + String(maxDurationOn[i], DEC) + "s) on GPIO " + String(_outputPin[i], DEC) + "(" + outputName[i] + ")");
    setPin(i, false);
  }ms+=LOOPDELAY;

  delay(LOOPDELAY);
}
