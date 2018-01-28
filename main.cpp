//Reference: https://www.arduino.cc/en/Reference/HomePage
//See: http://esp8266.github.io/Arduino/versions/2.1.0-rc1/doc/libraries.html
//Librairies et cartes ESP8266 sur IDE Arduino: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//http://arduino-esp8266.readthedocs.io/en/latest/
//peychart@netcourrier.com 20171021
// Licence: GNU v3
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ESP8266HTTPUpdateServer.h>

//Ajust the following:
short ResetConfig = 3;     //Just change this value to reset current config on the next boot...
#define DEFAULTWIFIPASS    "defaultPassword"
#define SSIDMax()          3
#define outputCount()      6
#define inputCount()       3
String  outputName[outputCount()] = {"Yellow", "Orange", "Red", "Green", "Blue", "White"}; //can be change by interface...
int outputPin[outputCount()]      = {  D0,       D1,      D2,      D3,     D4,      D8  };
int inputPin[inputCount()]        = {  D5,       D6,      D7  };

//Avoid to change the following:
String hostName = "ESP8266";//Can be change by interface
String ssid[SSIDMax()];     //Identifiants WiFi /Wifi idents
String password[SSIDMax()]; //Mots de passe WiFi /Wifi passwords
boolean inInt = false, WiFiAP=false, outputValue[outputCount()];
unsigned int maxDuration[outputCount()];
unsigned long timer[outputCount()];
ESP8266WebServer server(80); //Instanciation du serveur port 80
ESP8266WebServer updater(8081);
ESP8266HTTPUpdateServer httpUpdater;

String getMyMacAddress(){
  char ret[18];
  uint8_t mac[6]; WiFi.macAddress(mac);
  sprintf(ret, "%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return ret;
}

void setPin(int i, boolean v, boolean force=false){
  if(outputValue[i]!=v || force){
    Serial.println((String)"Set GPIO " + outputPin[i] + "(" + outputName[i] + ")" + " to " + (String)v);
    digitalWrite(outputPin[i], outputValue[i]=v);
    timer[i]=(unsigned long)(millis()+60000*maxDuration[i]);
} }

String getPlugsValues(){
  String page="";
  if (outputCount()){
    page += String(outputValue[0]);
    for (short i=1; i<outputCount(); i++)
      page += "," + String(outputValue[i]);
  } return page;    //Format: nn,nn,nn,nn,nn,...
}

String ultos(unsigned long v){ char ret[11]; sprintf(ret, "%ld", v); return ret; }

String  getPage(){
  String page="<html lang='us-US'><head><meta charset='utf-8'/>";
  page += "<title>" + hostName + "</title>\n";
  page += "<style>body{background-color:#fff7e6; font-family:Arial,Helvetica,Sans-Serif; Color:#000088;}\n";
  page += " ul{list-style-type:square;} li{padding-left:5px; margin-bottom:10px;}\n";
  page += " td{text-align:left; min-width:100px; vertical-align:middle;}\n";
  page += " .modal{display:none; position:fixed; z-index:1; left:0%; top:0%; height:100%; width:100%; overflow:scroll; background-color:#000000;}\n";
  page += " .modal-content{background-color:#fff7e6; margin:5% auto; padding:15px; border:2px solid #888; height:90%; width:90%; min-height:755px;}\n";
  page += " .close{color:#aaa; float:right; font-size:30px; font-weight:bold;}\n";
  page += " .duration{width:50px;}\n";
  page += "</style></head>\n<body onload='init();'>\n";
  page += "<script>\nthis.timer=0;\n";
  page += "function init(){var e;\n";
  page += "e=document.getElementById('example1');\ne.innerHTML=document.URL+'[" + getPlugsValues() + "]'; e.href=e.innerHTML;\n";
  page += "e=document.getElementById('example2');\ne.innerHTML=document.URL+'JsonData'; e.href=e.innerHTML;\n";
  page += "refresh();}\n";
  page += "function refresh(v=60){this.timer=setTimeout(function(){location.reload(true);},v*1000);}\n";
  page += "function showHelp(){clearTimeout(this.timer); refresh(600); document.getElementById('about').style.display='block';}\n";
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
  page += "</script><div id='about' class='modal'><div class='modal-content'>";
  page += "<span class='close' onClick='location.reload(true);'>&times;</span>";
  page += "<h1>About</h1>";
  page += "This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domotics or Jeedom.<br><br>";
  page += "In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). ";
  page += "Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs (Json format), like this:";
  page += "<a id='example1' style='padding:0 0 0 5px;'></a><br><br>";
  page += "The state of the electrical outlets can also be requested from the following URL: ";
  page += "<a id='example2' style='padding:0 0 0 5px;'></a> (-1 -> unchanged)<br><br>";
  page += "The status of the power strip is retained when the power is turned off and restored when it is turned on ; a power-on delay can be set on each output: (-1) no delay, (0) to disable an output and (number of mn) to configure a power-on delay.<br><br>";
  page += "The following allows you to configure some parameters of the Wifi Power Strip (until a SSID is set, the socket works as an access point with its own SSID and default password: \"" + hostName + "/" + DEFAULTWIFIPASS + "\").<br><br>";
  page += "<h2><form method='POST'>";
  page += "Network name: <input type='text' name='hostname' value='" + hostName + "' style='width:110;'>";
  page += " <input type='button' value='Submit' onclick='submit();'>";
  page += "</form></h2>";
  page += "<h2>Network connection:</h2>";
  page += "<table style='width:100%'><tr>";
  for(int i=0; i<SSIDMax(); i++){
   page += "<td><div><form method='POST'>";
   page += "SSID " + String(i+1) + ":<br><input type='text' name='SSID' value='" + ssid[i] + (ssid[i].length() ?"' readonly": "'") + "><br>";
   page += "Password:<br><input type='password' name='password' value='" + String(ssid[i][0] ?password[i] :"") + "'><br>";
   page += "Confirm password:<br><input type='password' name='confirm' value='" + String(ssid[i][0] ?password[i] :"") + "'><br><br>";
   page += "<input type='button' value='Submit' onclick='saveSSID(this);'>";
   page += "<input type='button' value='Remove' onclick='deleteSSID(this);'>";
   page += "</form></div></td>";
  }page += "</tr></table>";
  page += "<h2><form method='POST'>Names of Plugs: ";
  for(short i=0; i<outputCount(); i++){
   page += "<input type='text' name='plugName" + ultos(i) + "' value='" + outputName[i] + "' style='width:70;'>";
 }page += " - <input type='button' value='Submit' onclick='submit();'></form></h2>";
  page += "<h6><a href='update' onclick=\"javascript:event.target.port=8081\">Firmware update</a>";
  page += " - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>";
  page += "</div></div>";
  page += "<table style='border:0; width:100%;'><tbody><tr>";
  page += "<td><h1>" + hostName + " - " + (WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString()) + " [" + getMyMacAddress() + "]" + " :</h1></td>";
  page += "<td style='text-align:right; vertical-align:top;'><p><span class='close' onclick='showHelp();'>?</span></p></td>";
  page += "<tr></tbody></table>";
  page += "<h3>Status :</h3>";
  page += "<form method='POST'><ul>";
  for (short i=0; i<outputCount(); i++){
   page += "<li><table><tbody><tr><td>" + outputName[i] + "</td><td style='width:220px;'>";
   page += "<INPUT type='radio' name=" + outputName[i] + " value='1' onchange='submit();'"+(outputValue[i]==HIGH ?"checked" :"") + ">ON </INPUT>";
   page += "(during <INPUT type='number'  name='" + outputName[i] + "-max-duration' value='" + ultos((unsigned long)((int)maxDuration[i])) + "' min='-1' max='720' class='duration';/>mn)</td>";
   page += "<td><INPUT type='radio' name=" + outputName[i] + " value='0' onchange='submit();'"+(outputValue[i]==LOW  ?"checked" :"") + ">OFF</INPUT>";
   page += "</td></tr></tbody></table></li>";
  }page += "</ul></body></html>";
  return page;
}

bool WiFiHost(){
  Serial.println();
  Serial.print("No custom SSID defined: ");
  Serial.println("setting soft-AP configuration ... ");
  WiFiAP=WiFi.softAP(hostName.c_str(), password[0].c_str());
  Serial.println(String("Connecting [") + WiFi.softAPIP().toString() + "] from: " + hostName + "/" + password[0]);
  return WiFiAP;
}

bool WiFiConnect(){
  WiFi.disconnect(); WiFi.softAPdisconnect(); WiFiAP=false;
  delay(10);

  if(!ssid[0][0])
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
      return true;
    }else Serial.println("WiFi Timeout.");
  } return false;
}

void writeConfig(){        //Save current config:
  short i;
  File f=SPIFFS.open("/config.txt", "w+");
  if(f){
    f.println(ResetConfig);
    f.println(hostName);                //Save hostname
    for(short j=i=0; j<SSIDMax(); j++){   //Save SSIDs
      if(!password[j].length()) ssid[j]="";
      if(ssid[j].length()){
        f.println(ssid[j]);
        f.println(password[j]);
        i++;
    }}while(i++<SSIDMax()){f.println(); f.println();}
    for(i=0; i<outputCount(); i++){     //Save output states
      f.println(outputName[i]);
      f.println(outputValue[i]);
      f.println((int)maxDuration[i]);
    }f.close();
  }if(WiFiAP && ssid[0].length()) WiFiConnect();
}

String readString(File f){ String ret=f.readStringUntil('\n'); ret.remove(ret.indexOf('\r')); return ret; }

void readConfig(){      //Get config:
  File f=SPIFFS.open("/config.txt", "r");
  if(f && ResetConfig!=atoi(readString(f).c_str())) f.close();
  if(!f){                         //Write default config:
    ssid[0]=""; password[0]=DEFAULTWIFIPASS;
    for(short i=0; i<outputCount(); i++){
      maxDuration[i]=-1; outputValue[i]=LOW;
    }SPIFFS.format(); writeConfig();
  }else{                          //Get config:
    String outputNameBackup[outputCount()]; //Save default values
    for(short i=0; i<outputCount(); i++) outputNameBackup[i]=outputName[i];

    hostName=readString(f);                //Get hostname
    for(short i=0; i<SSIDMax(); i++){        //Get SSIDs
      ssid[i]=readString(f);
      password[i]=readString(f);
    }for(short i=0; i<outputCount(); i++){   //Get output states
      outputName[i]=readString(f);
      outputValue[i]=atoi(readString(f).c_str());
      maxDuration[i]=atoi(readString(f).c_str());
    }f.close();

    if(!ssid[0].length())        //Default values
      password[0]=DEFAULTWIFIPASS;
    for(short i=0; i<outputCount(); i++)
      if(!outputName[i].length())
        for(i=0; i<outputCount(); i++)
          outputName[i]=outputNameBackup[i];
} }

void handleSubmit(int i){  //Actualise le GPIO /Update GPIO
  String v=server.arg(outputName[i]); v.toUpperCase();
  maxDuration[i]=atoi((server.arg(outputName[i]+"-max-duration")).c_str());
  setPin(i, (v=="TRUE" || v=="1" || v=="ON" || v=="UP") ?HIGH :LOW);
  writeConfig();
}

void handleSubmitSSIDConf(){           //Setting:
  int count=0;
  for(short i=0; i<SSIDMax(); i++) if(ssid[i].length()) count++;
  for(short i=0; i<count; i++)
    if(ssid[i]==server.arg("SSID")){
      password[i]=server.arg("password");
      if(!password[i].length())    //Modify password
        ssid[i]=="";               //Delete ssid
      writeConfig();
      if(!ssid[i].length()) readConfig();
      return;
  }if(count<SSIDMax()){            //Add ssid:
    ssid[count]=server.arg("SSID");
    password[count]=server.arg("password");
  }writeConfig();
}

void  handleRoot(){
  if(server.hasArg("hostname")){
    hostName=server.arg("hostname");                  //Set host name:
    writeConfig();
  }else if(server.hasArg("password")){                //Set WiFi connections:
    handleSubmitSSIDConf();
  }else for(short i=0; i<outputCount(); i++)
    if(server.hasArg(outputName[i]))                  //Set outputs values:
      handleSubmit(i);
    else if(server.hasArg("plugName"+ultos(i)))       //Set outputs names:
      if(server.arg("plugName"+ultos(i)))
        outputName[i]=server.arg("plugName"+ultos(i));
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

void swap(){    //Gestion des switchs/Switchs management
  if(inInt)
    return;
  inInt=true;
  unsigned long int n=0;for(unsigned long i=-1;--i;) while(--n);  //unrebounce...
  for(short i=0; i<inputCount(); i++){
    Serial.print(i);Serial.print("-");Serial.println(digitalRead(inputPin[i]));
    if(digitalRead(inputPin[i])==LOW){
      if(inputCount()<outputCount())
           n+=pow(2,i);
      else {n=i+1; break;}
    }
  }if(--n<outputCount()) setPin(n, (outputValue[n]==HIGH) ?LOW :HIGH);
//  inInt=false;
}

void setup(){
  Serial.begin(115200);
  delay(10);Serial.println("Hello World!");

  //Definition des URL d'entree /Input URL definition
  server.on("/", handleRoot);
  server.on("/JsonData", handleJsonData);
  //server.on("/about", [](){ server.send(200, "text/plain", getHelp()); });

  //Demarrage du serveur web /Web server start
  server.begin();
  Serial.println("Server started");

  //Open config:
  SPIFFS.begin();
  readConfig();

  //initialisation des broches /pins init
  for(short i=0; i<outputCount(); i++){   //Sorties/ouputs:
    pinMode(outputPin[i], OUTPUT);
    setPin(i, outputValue[i], true);    //Entrées/inputs:
  }for(short i=0; i<inputCount(); i++){
    pinMode(inputPin[i], INPUT_PULLUP);
    //See: https://www.arduino.cc/en/Reference/attachInterrupt
    // or: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
    attachInterrupt(inputPin[i], swap, FALLING);
  }

  //Allow OnTheAir updates:
  MDNS.begin(hostName.c_str());
  httpUpdater.setup(&updater);
  updater.begin();
}

#define LOOPDELAY 2000
unsigned int count=0;
void loop(){ inInt=false;
  updater.handleClient();

  if(!count--){ count=60000/LOOPDELAY;            //Test connexion/Check WiFi every mn
    if(WiFi.status() != WL_CONNECTED && !WiFiAP) WiFiConnect();
  }

  for(short i=0; i<outputCount(); i++)              //Check timers:
    if(outputValue[i] && maxDuration[i]!=-1 && millis()>timer[i])
      setPin(i, LOW);

  server.handleClient();                         //Traitement des requetes /HTTP treatment

  delay(LOOPDELAY);
}
