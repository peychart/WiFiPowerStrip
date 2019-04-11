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
short   ResetConfig =      1;     //Just change this value to reset current config on the next boot...
#define DEFAULTWIFIPASS    "defaultPassword"
#define MAXWIFIERRORS      10
#define SSIDMax()          3
#define inputCount()       3
#define outputCount()      5
// Restore output values after a reboot:
#define RESTO_VALUES       false
//String  outputName[outputCount()] = {"Yellow", "Orange", "Red", "Green", "Blue", "White"}; //can be change by interface...
//int _outputPin[outputCount()]      = {  D0,       D1,      D2,      D3,     D4,      D8  };
String  outputName[outputCount()] = {"Yellow", "Green", "Orange", "Red", "Blue" }; //can be change by interface...
int     _outputPin[outputCount()] = {  D1,       D2,      D3,      D4,     D0  };
int     _inputPin [inputCount()]  = {  D5,       D6,      D7  };

//Avoid to change the following:
String  hostname = "ESP8266"; //Can be change by interface
String  ssid[SSIDMax()];      //Identifiants WiFi /Wifi idents
String  password[SSIDMax()];  //Mots de passe WiFi /Wifi passwords
bool    WiFiAP=false,   outputValue[outputCount()];
unsigned short          nbWifiAttempts=MAXWIFIERRORS, WifiAPDelay;
unsigned int            maxDurationOn[outputCount()];
unsigned long           timerOn[outputCount()];
volatile short          intr=0;
volatile unsigned long  last_intr=millis();
#define  BOUNCE_DELAY   500L
ESP8266WebServer        server(80); //Instanciation du serveur port 80
ESP8266WebServer        updater(8081);
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

String& getPage(){
  static String s;
  s  = "<!DOCTYPE HTML>\n<html lang='us-US'>\n<head><meta charset='utf-8'/>\n<title>" + hostname + "</title>\n";
  s += "<style>body{background-color:#fff7e6; font-family:Arial,Helvetica,Sans-Serif; Color:#000088;}\n";
  s += " ul{list-style-type:square;} li{padding-left:5px; margin-bottom:10px;}\n";
  s += " td{text-align:left; min-width:110px; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n";
  s += " .modal{display:none; position:fixed; z-index:1; left:0%; top:0%; height:100%; width:100%; overflow:scroll; background-color:#000000;}\n";
  s += " .modal-content{background-color:#fff7e6; margin:5% auto; padding:15px; border:2px solid #888; height:90%; width:90%; min-height:755px;}\n";
  s += " .close{color:#aaa; float:right; font-size:30px; font-weight:bold;}\n";
  s += " .delayConf{float: left; vertical-align:middle; display: inline-block; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}\n";
  s += " .duration{width:50px;}\n";
  s += "  /*see: https://proto.io/freebies/onoff/: */\n";
  s += " .onoffswitch {position: relative; width: 90px;-webkit-user-select:none; -moz-user-select:none; -ms-user-select: none;}\n";
  s += " .onoffswitch-checkbox {display: none;}\n";
  s += " .onoffswitch-label {display: block; overflow: hidden; cursor: pointer;border: 2px solid #999999; border-radius: 20px;}\n";
  s += " .onoffswitch-inner {display: block; width: 200%; margin-left: -100%;transition: margin 0.3s ease-in 0s;}\n";
  s += " .onoffswitch-inner:before, .onoffswitch-inner:after {display: block; float: left; width: 50%; height: 30px; padding: 0; line-height: 30px;font-size: 14px; color: white; font-family: Trebuchet, Arial, sans-serif; font-weight: bold;box-sizing: border-box;}\n";
  s += " .onoffswitch-inner:before{content: 'ON';padding-left: 10px;background-color: #34C247; color: #FFFFFF;}\n";
  s += " .onoffswitch-inner:after {content: 'OFF';padding-right: 10px;background-color: #EEEEEE; color: #999999;text-align: right;}\n";
  s += " .onoffswitch-switch{display: block; width: 18px; margin: 6px;background: #FFFFFF;position: absolute; top: 0; bottom: 0;right: 56px;border: 2px solid #999999; border-radius: 20px;transition: all 0.3s ease-in 0s;}\n";
  s += " .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-inner {margin-left: 0;}\n";
  s += " .onoffswitch-checkbox:checked + .onoffswitch-label .onoffswitch-switch {right: 0px;}\n";
  s += "</style></head>\n<body onload='init();'>\n";
  s += "<script>\nthis.timer=0;\n";
  s += "function init(){var e;\n";
  s += "e=document.getElementById('example1');\ne.innerHTML=document.URL+'status&[" + getPlugsValues() + "]'; e.href=e.innerHTML;\n";
  s += "e=document.getElementById('example2');\ne.innerHTML=document.URL+'status'; e.href=e.innerHTML;\n";
  s += "refresh();}\n";
  s += "function refresh(v=30){clearTimeout(this.timer); document.getElementById('about').style.display='none';\n";
  s += "this.timer=setTimeout(function(){getStatus(); refresh(v);}, v*1000);}\n";
  s += "function getStatus(){var j, ret, e, req=new XMLHttpRequest(); req.open('GET', document.URL+'status', false); req.send(null); ret=req.responseText;\n";
  s += "if((j=ret.indexOf('[')) >= 0){\n";
  s += "if((e=document.getElementsByClassName('onoffswitch-checkbox')).length && (j=ret.indexOf('['))>=0)\n";
  s += " for(var e,v,i=0,r=ret.substr(j+1); r[0] && r[0]!=']'; i++){\n";
  s += "  j=r.indexOf(',');if(j<0) j=r.indexOf(']'); v=parseInt(r.substr(0,j));\n";
  s += "  if(v>=0) e[i].checked=(v?true:false); r=r.substr(j+1);\n";
  s += " }if((e=document.getElementsByClassName('onoffswitch-checkbox')).length)\n";
  s += "  for(var e,v,i=0,ret=r.substr(j+1); r[0] && r[0]!=']'; i++){\n";
  s += "   j=r.indexOf(',');if(j<0) j=r.indexOf(']'); v=parseInt(r.substr(0,j));\n";
  s += "   if(v>=0) e[i].checked=(v?true:false); r=r.substr(j+1);\n";
  s += "}}}\n";
  s += "function showHelp(){refresh(120); document.getElementById('about').style.display='block';}\n";
  s += "function saveSSID(f){\n";
  s += "if((f=f.parentNode)){var s, p=false;\n";
  s += "for(var i=0; i<f.children.length; i++){\n";
  s += "if(f.children[i].type=='password'){\n";
  s += "if (!p) p=f.children[i];\n";
  s += "else if(p.value!=f.children[i].value) p.value='';\n";
  s += "}else if(f.children[i].type=='text') s=f.children[i];\n";
  s += "}if(s.value==''){\n";
  s += "alert('Empty SSID...'); f.reset(); s.focus();\n";
  s += "}else if(p.value==''){\n";
  s += "var ssid=s.value; f.reset(); s.value=ssid;\n";
  s += "alert('Incorrect password...'); p.focus();\n";
  s += "}else f.submit();\n";
  s += "}}\n";
  s += "function deleteSSID(f){\n";
  s += "if((f=f.parentNode))\n";
  s += "for(var i=0; i<f.children.length; i++)\n";
  s += "if(f.children[i].type=='text')\n";
  s += "if(f.children[i].value!=''){\n";
  s += "if(confirm('Are you sure to remove this SSID?')){\n";
  s += "for(var i=0; i<f.children.length; i++)\n";
  s += "if(f.children[i].type=='password') f.children[i].value='';\n";
  s += "f.submit();\n";
  s += "}}else alert('Empty SSID...');\n";
  s += "}\n";
  s += "function switchSubmit(e){refresh();\n";
  s += " var b=false, l=e.parentNode.parentNode.getElementsByTagName('input');\n";
  s += " for(var i=0; i<l.length; i++) if(l[i].type=='number')\n   b|=(l[i].value!='0');\n";
  s += " e.checked&=b; document.getElementById('switchs').submit();\n";
  s += "}\n";
  s += "var checkDelaySubmit=0;\n";
  s += "function checkDelay(e){refresh();\n";
  s += " if(e.value=='-1'){\n";
  s += "  var l=e.parentNode.getElementsByTagName('input');\n";
  s += "  for(var i=0; i<l.length; i++)\n   if(l[i].className=='duration'){\n    if(l[i].getAttribute('data-unit')!=1)\n     l[i].style.display='none';\n     else l[i].style.display='inline-block';}\n";
  s += " }clearTimeout(this.checkDelaySubmit); this.checkDelaySubmit=setTimeout(function(){this.checkDelaySubmit=0; document.getElementById('switchs').submit();}, 1000);\n";
  s += "}\n";
  s += "</script>\n<div id='about' class='modal'><div class='modal-content'>";
  s += "<span class='close' onClick='refresh();'>&times;</span>";
  s += "<h1>About</h1>";
  s += "This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domotics or Jeedom.<br><br>";
  s += "In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). ";
  s += "Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs (Json format), like this:";
  s += "<a id='example1' style='padding:0 0 0 5px;'></a> (-1 -> unchanged)<br><br>";
  s += "The state of the electrical outlets can also be requested from the following URL: ";
  s += "<a id='example2' style='padding:0 0 0 5px;'></a><br><br>";
  s += "The status of the power strip is retained when the power is turned off and restored when it is turned on ; a power-on delay can be set on each output: (-1) no delay, (0) to disable an output and (number of s) to configure a power-on delay.<br><br>";
  s += "The following allows you to configure some parameters of the Wifi Power Strip (until a SSID is set, the socket works as an access point with its own SSID and default password: \"" + hostname + "/" + DEFAULTWIFIPASS + "\" on 192.168.4.1).<br><br>";
  s += "<h2><form method='POST'>\n";
  s += "Network name: <input type='text' name='hostname' value='" + hostname + "' style='width:110;'>\n";
  s += " <input type='button' value='Submit' onclick='submit();'>\n";
  s += "</form></h2>\n";
  s += "<h2>Network connection:</h2>\n";
  s += "<table style='width:100%'><tr>";
  for(int i=0; i<SSIDMax(); i++){
    s += "<td><div><form method='POST'>\n";
    s += "SSID " + String(i+1) + ":<br><input type='text' name='SSID' value='" + ssid[i] + (ssid[i].length() ?"' readonly": "'") + "><br>\n";
    s += "Password:<br><input type='password' name='password' value='" + String(ssid[i][0] ?password[i] :"") + "'><br>\n";
    s += "Confirm password:<br><input type='password' name='confirm' value='" + String(ssid[i][0] ?password[i] :"") + "'><br><br>\n";
    s += "<input type='button' value='Submit' onclick='saveSSID(this);'>";
    s += "<input type='button' value='Remove' onclick='deleteSSID(this);'>\n";
    s += "</form></div></td>";
 }s += "</tr></table>\n";
  s += "<h2><form method='POST'>Names of Plugs: ";
  for(short i=0; i<outputCount(); i++)
    s += "<input type='text' name='plugName" + String(i) + "' value='" + outputName[i] + "' style='width:70;'>";
  s += " - <input type='button' value='Submit' onclick='submit();'></form></h2>\n";
  s += "<h6><a href='update' onclick='javascript:event.target.port=8081'>Firmware update</a>";
  s += " - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>";
  s += "</div></div>\n";
  s += "<table style='border:0; width:100%;'><tbody><tr>";
  s += "<td><h1>" + hostname + " - " + (WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString()) + " [" + getMyMacAddress() + "]" + " :</h1></td>";
  s += "<td style='text-align:right; vertical-align:top;'><p><span class='close' onclick='showHelp();'>?</span></p></td>";
  s += "<tr></tbody></table>\n";
  s += "<h3>Status :</h3>\n";
  s += "<form id='switchs' method='POST'><ul>\n";
  for (short i=0; i<outputCount(); i++){ bool display;
    s += "<li><table><tbody>\n<tr><td>" + outputName[i] + "</td><td>\n";
    s += "<div class='onoffswitch delayConf'>\n";
    s += "<input type='checkbox' class='onoffswitch-checkbox' id='" + outputName[i] + "' name='" + outputName[i] + "' " + (outputValue[i] ?"checked" :"") + " onClick='switchSubmit(this);'>\n";
    s += "<label class='onoffswitch-label' for='" + outputName[i] + "'><span class='onoffswitch-inner'></span><span class='onoffswitch-switch'></span></label>\n";
    s += "</div>\n<div class='delayConf'> &nbsp; &nbsp; &nbsp; (will be 'ON' during: &nbsp;\n";
    display=(maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]/86400);
    s += "<input type='number' name='" + outputName[i] + "-max-duration-d' value='" + (display ?(maxDurationOn[i]/86400) :0) + "' min='0' max='366' data-unit=86400 class='duration' style='width:60px;display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"d &nbsp;\n" :"\n");
    display|=(maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]%86400/3600);
    s += "<input type='number' name='" + outputName[i] + "-max-duration-h' value='" + (display ?(maxDurationOn[i]%86400/3600) :0) + "' min='0' max='24' data-unit=3600 class='duration' style='display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"h &nbsp;\n" :"\n");
    display|=( (maxDurationOn[i]!=(unsigned int)(-1)) && (maxDurationOn[i]%86400%3600/60) );
    s += "<input type='number' name='" + outputName[i] + "-max-duration-mn' value='" + (display ?(maxDurationOn[i]%86400%3600/60) :0) + "' min='-1' max='60' data-unit=60 class='duration' style='display:" + (String)(display ?"inline-block" :"none") + ";' onChange='checkDelay(this);'>" + (String)(display ?"mn &nbsp;\n" :"\n");
    s += "<input type='number' name='" + outputName[i] + "-max-duration-s'  value='" + ((maxDurationOn[i]!=(unsigned int)(-1)) ?(maxDurationOn[i]%86400%3600%60) :-1) + "' min='-1' max='60' data-unit=1 class='duration' onChange='checkDelay(this);'>" + ((maxDurationOn[i]!=(unsigned int)(-1)) ?"s\n" :"-\n");
    s += ")</div>\n</td></tr>\n</tbody></table></li>\n";
  } s += "</ul><div><input type='checkbox' name='newValue' id='newValue' checked style=\"display:none\"></div></form>\n</body>\n</html>\n";
  return s;
}

bool WiFiHost(){
  Serial.println();
  Serial.print("No custom SSID defined: setting soft-AP configuration ... ");
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
  }
  f.close();
  return ret;
}

void setPin(int i, bool v, bool force=false){
  if(outputValue[i]!=v || force){
    Serial.println((String)"Set GPIO " + _outputPin[i] + "(" + outputName[i] + ")" + " to " + (String)v);
    digitalWrite(_outputPin[i], ((outputValue[i]=v) ?HIGH :LOW));
    timerOn[i]=(millis()+(unsigned long)(1000*maxDurationOn[i]));
    if(RESTO_VALUES) writeConfig();
} }

void handleSubmitSSIDConf(){           //Setting:
  int count=0;
  for(short i=0; i<SSIDMax(); i++) if(ssid[i].length()) count++;
  for(short i=0; i<count;     i++)
    if(ssid[i]==server.arg("SSID")){
      password[i]=server.arg("password");
      return;}
  if(count<SSIDMax()){                //Add ssid:
    ssid[count]=server.arg("SSID");
    password[count]=server.arg("password");
} }

inline bool handlePlugnameSubmit(short i){       //Set outputs names:
  if(server.hasArg("plugName"+(String)i) && server.arg("plugName"+(String)i))
    return(outputName[i]=server.arg("plugName"+(String)i));
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
  if(maxDurationOn[i]==(unsigned int)v)
    return false;
  maxDurationOn[i] = (unsigned int)v;
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

void  handleRoot(){bool w;
  if((w=server.hasArg("hostname")))
    hostname=server.arg("hostname");                  //Set host name
  else if((w=server.hasArg("password")))
    handleSubmitSSIDConf();                           //Set WiFi connections
  else{
    for(short i=0; i<outputCount(); i++)
      w|=handlePlugnameSubmit(i);                     //Set plug name
    if(!w) for(short i=0; i<outputCount(); i++)
      w|=handleDurationOnSubmit(i);                   //Set timeouts
    for(short i=0; !w && i<outputCount(); i++)
      handleValueSubmit(i);                           //Set values
  }if(w) writeConfig();
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
  unsigned short n; unsigned long reg=GPI;
  if(!intr++){
    for(short i=n=0; i<inputCount(); i++) if( (reg & (1<<(_inputPin[i] & 0xF)))==0 ) n+=(short)pow(2,i);
    if(--n<outputCount()) setPin(n, !outputValue[n]);
    last_intr = millis();
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
  readConfig(); if(!RESTO_VALUES) for(short i=0; i<outputCount(); i++) outputValue[i]=false;

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
void loop(){   if( intr && ((millis()-last_intr) >= BOUNCE_DELAY) ) intr=0;
  updater.handleClient();

  if(!count--){ count=60000/LOOPDELAY;            //Test connexion/Check WiFi every mn
    if( (WiFi.status()!=WL_CONNECTED) && (!WiFiAP || !WifiAPDelay--) )
      WiFiConnect();
  }

  for(short i=0; i<outputCount(); i++)              //Check timers:
    if( outputValue[i] && (maxDurationOn[i]!=(unsigned int)(-1)) && (millis()>timerOn[i]) ){
        Serial.println((String)"Timeout on GPIO " + _outputPin[i] + "(" + outputName[i] + ")");
        setPin(i, false);
    }

  server.handleClient();                         //Traitement des requetes /HTTP treatment

  delay(LOOPDELAY);
}
