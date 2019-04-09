//Reference: https://www.arduino.cc/en/Reference/HomePage
//See: http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
//Librairies et cartes ESP8266 sur IDE Arduino: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//http://arduino-esp8266.readthedocs.io/en/latest/
//JSon lib: see https://github.com/bblanchon/ArduinoJson.git
//peychart@netcourrier.com 20171021
// Licence: GNU v3
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ESP8266HTTPUpdateServer.h>

//Ajust the following:
#define LOOPDELAY          1000
short ResetConfig =        1;     //Just change this value to reset current config on the next boot...
#define DEFAULTWIFIPASS    "defaultPassword"
#define MAXWIFIERRORS      10
#define SSIDMax()          3
#define inputCount()       3
#define outputCount()      5
#define MEMOVALUES         false
//String  outputName[outputCount()] = {"Yellow", "Orange", "Red", "Green", "Blue", "White"}; //can be change by interface...
//int _outputPin[outputCount()]      = {  D0,       D1,      D2,      D3,     D4,      D8  };
String  outputName[outputCount()] = {"Yellow", "Green", "Orange", "Red" }; //can be change by interface...
int _inputPin [inputCount()]      = {  D5,       D6,      D7  };
int _outputPin[outputCount()]     = {  D1,       D2,       D3,      D4,      D0  };

//Avoid to change the following:
String hostname = "ESP8266";//Can be change by interface
String ssid[SSIDMax()];     //Identifiants WiFi /Wifi idents
String password[SSIDMax()]; //Mots de passe WiFi /Wifi passwords
bool WiFiAP=false, outputValue[outputCount()];
unsigned short nbWifiAttempts=MAXWIFIERRORS, WifiAPDelay;
unsigned int maxDurationOn[outputCount()];
unsigned long timerOn[outputCount()];
volatile bool intr=false;
volatile unsigned long last_millis;
#define BOUNCE_DELAY  500L
ESP8266WebServer server(80); //Instanciation du serveur port 80
ESP8266WebServer updater(8081);
ESP8266HTTPUpdateServer httpUpdater;

String getMyMacAddress(){
  char ret[18];
  uint8_t mac[6]; WiFi.macAddress(mac);
  sprintf(ret, "%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return ret;
}

String getPlugsValues(){
  String page="";
  if (outputCount()){
    page += String(outputValue[0] ?"1" :"0");
    for (short i=1; i<outputCount(); i++)
      page += "," + String(outputValue[i] ?"1" :"0");
  } return page;    //Format: nn,nn,nn,nn,nn,...
}

String ultos(unsigned long v){ char ret[11]; sprintf(ret, "%ld", v); return ret; }

String  getPage(){
  String page="<!DOCTYPE HTML>\n<html lang='us-US'>\n<head><meta charset='utf-8'/>\n<title>" + hostname + "</title>\n";
  page += "<style>body{background-color:#fff7e6; font-family:Arial,Helvetica,Sans-Serif; Color:#000088;}\n";
  page += " ul{list-style-type:square;} li{padding-left:5px; margin-bottom:10px;}\n";
  page += " td{text-align:left; min-width:110px; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n";
  page += " .modal{display:none; position:fixed; z-index:1; left:0%; top:0%; height:100%; width:100%; overflow:scroll; background-color:#000000;}\n";
  page += " .modal-content{background-color:#fff7e6; margin:5% auto; padding:15px; border:2px solid #888; height:90%; width:90%; min-height:755px;}\n";
  page += " .close{color:#aaa; float:right; font-size:30px; font-weight:bold;}\n";
  page += " .delayConf{float: left; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n";
  page += " .duration{width:50px;}\n";
  page += "  /*see: https://proto.io/freebies/onoff/: */\n";
  page += " .onoffswitch {position: relative; width: 90px;-webkit-user-select:none; -moz-user-select:none; -ms-user-select: none;}\n";
  page += " .onoffswitch-checkbox {display: none;}\n";
  page += " .onoffswitch-label {display: block; overflow: hidden; cursor: pointer;border: 2px solid #999999; border-radius: 20px;}\n";
  page += " .onoffswitch-inner {display: block; width: 200%; margin-left: -100%;transition: margin 0.3s ease-in 0s;}\n";
  page += " .onoffswitch-inner:before, .onoffswitch-inner:after {display: block; float: left; width: 50%; height: 30px; padding: 0; line-height: 30px;font-size: 14px; color: white; font-family: Trebuchet, Arial, sans-serif; font-weight: bold;box-sizing: border-box;}\n";
  page += " .onoffswitch-inner:before{content: 'ON';padding-left: 10px;background-color: #34C247; color: #FFFFFF;}\n";
  page += " .onoffswitch-inner:after {content: 'OFF';padding-right: 10px;background-color: #EEEEEE; color: #999999;text-align: right;}\n";
  page += " .onoffswitch-switch{display: block; width: 18px; margin: 6px;background: #FFFFFF;position: absolute; top: 0; bottom: 0;right: 56px;border: 2px solid #999999; border-radius: 20px;transition: all 0.3s ease-in 0s;}\n";
  page += " .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-inner {margin-left: 0;}\n";
  page += " .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-switch {right: 0px;}\n";
  page += "</style></head>\n<body onload='init();'>\n";
  page += "<script>\nthis.timer=0;\n";
  page += "function init(){var e;\n";
  page += "e=document.getElementById('example1');\ne.innerHTML=document.URL+'[" + getPlugsValues() + "]'; e.href=e.innerHTML;\n";
  page += "e=document.getElementById('example2');\ne.innerHTML=document.URL+'status'; e.href=e.innerHTML;\n";
  page += "refresh();}\n";
  page += "function refresh(v=30){clearTimeout(this.timer); document.getElementById('about').style.display='none';\n";
  page += "this.timer=setTimeout(function(){getStatus(); refresh(v);}, v*1000);}\n";
  page += "function getStatus(){var j, ret, e, req=new XMLHttpRequest(); req.open('GET', document.URL+'status', false); req.send(null); ret=req.responseText;\n";
  page += "if((j=ret.indexOf('[')) >= 0){\n";
  page += "if((e=document.getElementsByClassName('onoffswitch-checkbox')).length && (j=ret.indexOf('['))>=0)\n";
  page += " for(var e,v,i=0,r=ret.substr(j+1); r[0] && r[0]!=']'; i++){\n";
  page += "  j=r.indexOf(',');if(j<0) j=r.indexOf(']'); v=parseInt(r.substr(0,j));\n";
  page += "  if(v>=0) e[i].checked=(v?true:false); r=r.substr(j+1);\n";
  page += " }if((e=document.getElementsByClassName('onoffswitch-checkbox')).length)\n";
  page += "  for(var e,v,i=0,ret=r.substr(j+1); r[0] && r[0]!=']'; i++){\n";
  page += "   j=r.indexOf(',');if(j<0) j=r.indexOf(']'); v=parseInt(r.substr(0,j));\n";
  page += "   if(v>=0) e[i].checked=(v?true:false); r=r.substr(j+1);\n";
  page += "}}}\n";
  page += "function showHelp(){refresh(120); document.getElementById('about').style.display='block';}\n";
  page += "function saveSSID(f){\n";
  page += "if((f=f.parentNode)){var s, p=false;\n";
  page += "for(var i=0; i<f.children.length; i++){\n";
  page += "if(f.children[i].type=='password'){\n";
  page += "if (!p) p=f.children[i];\n";
  page += "else if(p.value!=f.children[i].value) p.value='';\n";
  page += "}else if(f.children[i].type=='text') s=f.children[i];\n";
  page += "}if(s.value==''){\n";
  page += "alert('Empty SSID...'); f.reset(); s.focus();\n";
  page += "}else if(p.value==''){\n";
  page += "var ssid=s.value; f.reset(); s.value=ssid;\n";
  page += "alert('Incorrect password...'); p.focus();\n";
  page += "}else f.submit();\n";
  page += "}}\n";
  page += "function deleteSSID(f){\n";
  page += "if((f=f.parentNode))\n";
  page += "for(var i=0; i<f.children.length; i++)\n";
  page += "if(f.children[i].type=='text')\n";
  page += "if(f.children[i].value!=''){\n";
  page += "if(confirm('Are you sure to remove this SSID?')){\n";
  page += "for(var i=0; i<f.children.length; i++)\n";
  page += "if(f.children[i].type=='password') f.children[i].value='';\n";
  page += "f.submit();\n";
  page += "}}else alert('Empty SSID...');\n";
  page += "}\n";
  page += "function switchSubmit(e){refresh();\n";
  page += " var b=false, l=e.parentNode.parentNode.getElementsByTagName('input');\n";
  page += " for(var i=0; i<l.length; i++) if(l[i].type=='number')\n   b|=(l[i].value!='0');\n";
  page += " e.checked&=b; document.getElementById('switchs').submit();\n";
  page += "}\n";
  page += "var checkDelaySubmit=0;\n";
  page += "function checkDelay(e){refresh();\n";
  page += " if(e.value=='-1'){\n";
  page += "  var l=e.parentNode.getElementsByTagName('input');\n";
  page += "  for(var i=0; i<l.length; i++)\n   if(l[i].className=='duration'){\n    if(l[i].getAttribute('data-unit')!=1)\n     l[i].style.display='none';\n     else l[i].style.display='inline-block';}\n";
  page += " }clearTimeout(this.checkDelaySubmit); this.checkDelaySubmit=setTimeout(function(){this.checkDelaySubmit=0; document.getElementById('switchs').submit();}, 1000);\n";
  page += "}\n";
  page += "</script>\n<div id='about' class='modal'><div class='modal-content'>";
  page += "<span class='close' onClick='refresh();'>&times;</span>";
  page += "<h1>About</h1>";
  page += "This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domotics or Jeedom.<br><br>";
  page += "In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). ";
  page += "Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs (Json format), like this:";
  page += "<a id='example1' style='padding:0 0 0 5px;'></a> (-1 -> unchanged)<br><br>";
  page += "The state of the electrical outlets can also be requested from the following URL: ";
  page += "<a id='example2' style='padding:0 0 0 5px;'></a><br><br>";
  page += "The status of the power strip is retained when the power is turned off and restored when it is turned on ; a power-on delay can be set on each output: (-1) no delay, (0) to disable an output and (number of mn) to configure a power-on delay.<br><br>";
  page += "The following allows you to configure some parameters of the Wifi Power Strip (until a SSID is set, the socket works as an access point with its own SSID and default password: \"" + hostname + "/" + DEFAULTWIFIPASS + "\").<br><br>";
  page += "<h2><form method='POST'>\n";
  page += "Network name: <input type='text' name='hostname' value='" + hostname + "' style='width:110;'>\n";
  page += " <input type='button' value='Submit' onclick='submit();'>\n";
  page += "</form></h2>\n";
  page += "<h2>Network connection:</h2>\n";
  page += "<table style='width:100%'><tr>";
  for(int i=0; i<SSIDMax(); i++){
   page += "<td><div><form method='POST'>\n";
   page += "SSID " + String(i+1) + ":<br><input type='text' name='SSID' value='" + ssid[i] + (ssid[i].length() ?"' readonly": "'") + "><br>\n";
   page += "Password:<br><input type='password' name='password' value='" + String(ssid[i][0] ?password[i] :"") + "'><br>\n";
   page += "Confirm password:<br><input type='password' name='confirm' value='" + String(ssid[i][0] ?password[i] :"") + "'><br><br>\n";
   page += "<input type='button' value='Submit' onclick='saveSSID(this);'>";
   page += "<input type='button' value='Remove' onclick='deleteSSID(this);'>\n";
   page += "</form></div></td>";
 }page += "</tr></table>\n";
  page += "<h2><form method='POST'>Names of Plugs: ";
  for(short i=0; i<outputCount(); i++)
   page += "<input type='text' name='plugName" + ultos(i) + "' value='" + outputName[i] + "' style='width:70;'>";
  page += " - <input type='button' value='Submit' onclick='submit();'></form></h2>\n";
  page += "<h6><a href='update' onclick='javascript:event.target.port=8081'>Firmware update</a>";
  page += " - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>";
  page += "</div></div>\n";
  page += "<table style='border:0; width:100%;'><tbody><tr>";
  page += "<td><h1>" + hostname + " - " + (WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString()) + " [" + getMyMacAddress() + "]" + " :</h1></td>";
  page += "<td style='text-align:right; vertical-align:top;'><p><span class='close' onclick='showHelp();'>?</span></p></td>";
  page += "<tr></tbody></table>\n";
  page += "<h3>Status :</h3>\n";
  page += "<form id='switchs' method='POST'><ul>\n";
  for (short i=0; i<outputCount(); i++){ bool display;
    page += "<li><table><tbody>\n<tr><td>" + outputName[i] + "</td><td>\n";
    page += "<div class='onoffswitch delayConf'>\n";
    page += "<input type='checkbox' class='onoffswitch-checkbox' id='" + outputName[i] + "' name='" + outputName[i] + "' " + (outputValue[i] ?"checked" :"") + " onClick='switchSubmit(this);'>\n";
    page += "<label class='onoffswitch-label' for='" + outputName[i] + "'><span class='onoffswitch-inner'></span><span class='onoffswitch-switch'></span></label>\n";
    page += "</div>\n<div class='delayConf'> &nbsp; &nbsp; &nbsp; (will be 'ON' during: &nbsp;\n";
    display=(maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]/86400L);
    page += "<input type='number' name='" + outputName[i] + "-max-duration-d' value='" + (display ?ultos((unsigned long)maxDurationOn[i]/86400L) :(String)"0") + "' min='0' max='366' data-unit=86400 class='duration' style='width:60px;display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"d &nbsp;\n" :"\n");
    display|=(maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]%86400L/3600L);
    page += "<input type='number' name='" + outputName[i] + "-max-duration-h' value='" + (display ?ultos((unsigned long)maxDurationOn[i]%86400L/3600L) :(String)"0") + "' min='0' max='24' data-unit=3600 class='duration' style='display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"h &nbsp;\n" :"\n");
    display|=( (maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]%86400L%3600L/60L) );
    page += "<input type='number' name='" + outputName[i] + "-max-duration-mn' value='" + (display ?ultos((unsigned long)maxDurationOn[i]%86400L%3600L/60L) :(String)"0") + "' min='-1' max='60' data-unit=60 class='duration' style='display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"mn &nbsp;\n" :"\n");
    page += "<input type='number' name='" + outputName[i] + "-max-duration-s'  value='" + ((maxDurationOn[i]!=(unsigned int)(-1)) ?ultos((unsigned long)maxDurationOn[i]%86400L%3600L%60L) :(String)"-1") + "' min='-1' max='60' data-unit=1 class='duration' onChange='checkDelay(this);'>" + (String)((maxDurationOn[i]!=(unsigned int)(-1)) ?"s\n" :"-\n");
    page += ")</div>\n</td></tr>\n</tbody></table></li>\n";
  } page += "</ul><div><input type='checkbox' name='newValue' id='newValue' checked style=\"display:none\"></div></form>\n</body>\n</html>\n";
  return page;
}

bool WiFiHost(){
  Serial.println();
  Serial.print("No custom SSID defined: ");
  Serial.println("setting soft-AP configuration ... ");
  WiFiAP=WiFi.softAP(hostname.c_str(), password[0].c_str());
  Serial.println(String("Connecting [") + WiFi.softAPIP().toString() + "] from: " + hostname + "/" + password[0]);
  nbWifiAttempts=(nbWifiAttempts==-1 ?1 :nbWifiAttempts); WifiAPDelay=60;
  return WiFiAP;
}

bool WiFiConnect(){
  WiFi.disconnect(); WiFi.softAPdisconnect(); WiFiAP=false;
  delay(10);

  if(!ssid[0][0] || !nbWifiAttempts--)
    return WiFiHost();

  for(short i=0; i<SSIDMax(); i++) if(ssid[i].length()){
    Serial.println(); Serial.println();

    //Connection au reseau Wifi /Connect to WiFi network
    Serial.println(String("Connecting [") + getMyMacAddress() + "] to: " + ssid[i]);
    WiFi.begin(ssid[i].c_str(), password[i].c_str());
    WiFi.mode(WIFI_STA);

    //Attendre la connexion /Wait for connection
    for(short j=0; j<16 && WiFi.status()!=WL_CONNECTED; j++){
      delay(500);
      Serial.print(".");
    } Serial.println();

    if(WiFi.status()==WL_CONNECTED){
      //Affichage de l'adresse IP /print IP address
      Serial.println("WiFi connected");
      Serial.println("IP address: " + WiFi.localIP().toString());
      Serial.println();
      nbWifiAttempts=MAXWIFIERRORS;
      return true;
    }else Serial.println("WiFi Timeout.");
  }
  return false;
}

bool readConfig(bool=true);
void writeConfig(){        //Save current config:
  if(!readConfig(false))
    return;
  File f=SPIFFS.open("/config.txt", "w+");
  if(f){ uint8_t count;
    f.println(ResetConfig);
    f.println(hostname);                //Save hostname
    for(uint8_t j=count=0; j<SSIDMax(); j++){   //Save SSIDs
      if(!password[j].length()) ssid[j]="";
      if(ssid[j].length()){
        f.println(ssid[j]);
        f.println(password[j]);
        count++;
    } }while(count++<SSIDMax()) f.println();
    for(count=0; count<outputCount(); count++){     //Save output states
      f.println(outputName[count]);
      f.println(outputValue[count]);
      f.println((int)maxDurationOn[count]);
    }Serial.println("SPIFFS writed.");
    f.close();
  }
  if(WiFiAP && ssid[0].length()) WiFiConnect();
}

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
      for(short i=0; i<outputCount(); i++){
        maxDurationOn[i]=(unsigned int)(-1); outputValue[i]=false;
      }SPIFFS.format(); writeConfig();
    Serial.println("SPIFFS initialized.");
    }return true;
  }ret|=getConfig(hostname, f, w);
  for(short i=0; i<SSIDMax(); i++){        //Get SSIDs
    ret|=getConfig(ssid[i], f, w);
    if(ssid[i].length())
      ret|=getConfig(password[i], f, w);
  }
  if(!ssid[0].length()) password[0]=DEFAULTWIFIPASS; //Default values
  for(short i=0; i<outputCount(); i++){   //Get output states
    ret|=getConfig(outputName[i], f, w);
    ret|=getConfig(outputValue[i], f, w);
    ret|=getConfig((int&)maxDurationOn[i], f, w);
  }f.close(); return ret;
}

void setPin(int i, bool v, bool force=false){
  if(outputValue[i]!=v || force){
    Serial.println((String)"Set GPIO " + _outputPin[i] + "(" + outputName[i] + ")" + " to " + (String)v);
    digitalWrite(_outputPin[i], ((outputValue[i]=v) ?HIGH :LOW));
    timerOn[i]=(millis()+(unsigned long)(1000*maxDurationOn[i]));
    if(MEMOVALUES) writeConfig();
} }

void handleSubmitSSIDConf(){           //Setting:
  int count=0;
  for(short i=0; i<SSIDMax(); i++) if(ssid[i].length()) count++;
  for(short i=0; i<count;     i++)
    if(ssid[i]==server.arg("SSID")){
      password[i]=server.arg("password");
      return;}
  if(count<SSIDMax()){            //Add ssid:
    ssid[count]=server.arg("SSID");
    password[count]=server.arg("password");
} }

inline bool handlePlugnameSubmit(short i){       //Set outputs names:
  if(server.hasArg("plugName"+ultos(i)) && server.arg("plugName"+ultos(i)))
    return(outputName[i]=server.arg("plugName"+ultos(i)));
  return false;
}

inline bool handleDurationOnSubmit(short i){ unsigned int v;        //Set outputs durations:
  if(!server.hasArg(outputName[i]+"-max-duration-s"))
    return false;
  v   =atoi((server.arg(outputName[i]+"-max-duration-s")).c_str());
  if(server.hasArg(outputName[i]+"-max-duration-mn"))
    v+=atoi((server.arg(outputName[i]+"-max-duration-mn")).c_str())*60;
  if(server.hasArg(outputName[i]+"-max-duration-h"))
    v+=atoi((server.arg(outputName[i]+"-max-duration-h")).c_str())*3600;
  if(server.hasArg(outputName[i]+"-max-duration-d"))
    v+=atoi((server.arg(outputName[i]+"-max-duration-d")).c_str())*86400;
  if(maxDurationOn[i]==v)
    return false;
  maxDurationOn[i] = v;
  return true;
}

inline bool handleValueSubmit(short i){        //Set outputs values:
  if(!server.hasArg("newValue"))
    return false;                              // It's a new connection...
  if(server.hasArg(outputName[i])==outputValue[i])
    return false;
  setPin(i, server.hasArg(outputName[i]));     // not arg if unchecked...
  return true;
}

void  handleRoot(){bool ret;
  if((ret=server.hasArg("hostname")))
    hostname=server.arg("hostname");                  //Set host name
  else if((ret=server.hasArg("password")))
    handleSubmitSSIDConf();                           //Set WiFi connections
  else{
    for(short i=0; i<outputCount(); i++)
      ret|=handlePlugnameSubmit(i);                   //Set plug name
    if(!ret) for(short i=0; i<outputCount(); i++)
      ret|=handleDurationOnSubmit(i);                 //Set timeouts
    for(short i=0; !ret && i<outputCount(); i++)
      handleValueSubmit(i);                           //Set values
  }if(ret) writeConfig();
  server.send(200, "text/html", getPage());
}

void  setValues(String v){ //Expected format: nn,nn,nn,nn,nn,...
  v.replace("%20", ""); v.replace("%0A", ""); v.replace("%0D", ""); v.replace("%2C", ",");
  v.replace(" ", "");   v.replace("\r", "");  v.replace("\n", "");
  for (short i=0,n; v.length(); ){
    if ( (n=v.substring(0, v.indexOf(',')).toInt()) >=0 ){
      setPin(i++, n ?HIGH :LOW);
      v = v.substring(v.indexOf(',') + 1);
} } }

void  handleJsonData(){
  if (server.args() && server.arg(0).startsWith("%5B") && server.arg(0).endsWith("%5D"))
    setValues( server.arg(0).substring(3, server.arg(0).length() - 6) );
  server.send(200, "text/plain", "[" + getPlugsValues() + "]");
}

void debounceInterrupt(){    //Gestion des switchs/Switchs management
  unsigned long n, reg;
  if(!intr){
    intr=true; reg=GPI; last_millis = millis();
    for(short i=(n=0L); i<inputCount(); i++) if( (reg & (1<<(_inputPin[i] & 0xF)))==0 ) n+=pow(2,i);
    if(--n<outputCount()) setPin(n, !outputValue[n]);
}  }

void setup(){
  Serial.begin(115200);
  delay(10);Serial.println();Serial.println("Hello World!");

  //Definition des URL d'entree /Input URL definition
  server.on("/", handleRoot);
  server.on("/status", handleJsonData);
  //server.on("/about", [](){ server.send(200, "text/plain", getHelp()); });

  //Demarrage du serveur web /Web server start
  server.begin();
  Serial.println("Server started");

  //Open config:
  SPIFFS.begin();
  readConfig(); if(!MEMOVALUES) for(short i=0; i<outputCount(); i++) outputValue[i]=false;

  //initialisation des broches /pins init
  for(short i=0; i<outputCount(); i++){   //Sorties/ouputs:
    pinMode(_outputPin[i], OUTPUT);
    setPin(i, outputValue[i], true);    //Entrées/inputs:
  }for(short i=0; i<inputCount(); i++){
    pinMode(_inputPin[i], INPUT_PULLUP);  //only this mode works on all inputs !...
    //See: https://www.arduino.cc/en/Reference/attachInterrupt
    // or: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
    attachInterrupt(_inputPin[i], debounceInterrupt, FALLING);
  }

  //Allows OnTheAir updates:
  MDNS.begin(hostname.c_str());
  httpUpdater.setup(&updater);
  updater.begin();
}

unsigned short int count=0;
void loop(){   if(intr) intr=(((long)millis()-last_millis) < BOUNCE_DELAY );
  updater.handleClient();

  if(!count--){ count=60000/LOOPDELAY;            //Test connexion/Check WiFi every mn
    if(WiFi.status() != WL_CONNECTED && (!WiFiAP || !WifiAPDelay--))
      WiFiConnect();
  }

  for(short i=0; i<outputCount(); i++)              //Check timers:
    if( outputValue[i] && maxDurationOn[i]!=(unsigned int)(-1) && millis()>timerOn[i] ){
        Serial.println((String)"Timeout on GPIO " + _outputPin[i] + "(" + outputName[i] + ")");
        setPin(i, false);
    }

  server.handleClient();                         //Traitement des requetes /HTTP treatment

  delay(LOOPDELAY);
}
