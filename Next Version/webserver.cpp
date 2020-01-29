#include "common.h"

ESP8266WebServer                          ESPWebServer(80);

extern bool                               WiFiAP;
extern String                             hostname;
extern String                             ssid[SSIDCount()];
extern String                             password[SSIDCount()];
extern struct ntpConf{
  String                                  source;
  short                                   zone;
  bool                                    dayLight;
} ntp;

extern String                             defaultScript;

extern struct pinConf{
  std::vector<String>                     gpio;
  std::map<String,String>                 name, gpioVar;
  std::map<String,bool>                   state;
  std::map<String,ushort>                 mode;
} pin;

extern PubSubClient                       mqttClient;
extern struct mqttConf{
  String                                  broker, idPrefix, user, pwd, topic;
  ushort                                  port;
} mqtt;

#ifdef DEBUG
  extern WiFiServer               telnetServer;
  extern WiFiClient               telnetClient;
#endif

String getConfig(String s)                                  {return "\""+s+"\"";}
template<typename T> String getConfig(String n, T v)        {return "\""+n+"\":"+String(v)+",";}
template<> String getConfig(String s, String v)             {return "\""+s+"\":\""+String(v)+"\",";}
template<> String getConfig(String n, std::vector<String> v){
  String s; for(ushort i(0); i<v.size(); i++) s+='\"'+v[i]+'\"'+','; if(s.length()) s.remove(s.length()-1);
  return getConfig(n)+":["+s+"],";
}template<typename T> String getConfig(String n, std::map<String,T> v){
  String s; for(typename std::map<String,T>::iterator i=v.begin(); i!=v.end(); i++) s+=getConfig(i->first, i->second);
  if(s.length()) s.remove(s.length()-1);
  return getConfig(n)+":{"+s+"},";
}String getConfig(){
  String s('{');
  s+=getConfig<String>("version", VERSION);
  s+=getConfig("hostname",        hostname);
  s+=getConfig("ipAddr",          WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString());
  s+=getConfig("macAddr",         WiFi.macAddress());
  s+=getConfig("ident",           String(ESP.getChipId()));
  s+=getConfig<String>("defaultHostname", String(DEFAULTHOSTNAME)+"-"+ESP.getChipId());
  s+=getConfig("defaultPassword", String(DEFAULTWIFIPASS));
  s+=getConfig("ssid")+":["; for(int i=0; i<SSIDCount(); i++) s+=getConfig(ssid[i])+','; if(s.length()) s.remove(s.length()-1); s+="],";
  s+=getConfig("ntpSource",       ntp.source);
  s+=getConfig("ntpZone",         ntp.zone);
  s+=getConfig("ntpDayLight",     ntp.dayLight);
  s+=getConfig("pinGpio",         pin.gpio);
  s+=getConfig("pinName",         pin.name);
  s+=getConfig("pinMode",         pin.mode);
  s+=getConfig("gpioVar",         pin.gpioVar);
  s+=getConfig("mqttBroker",      mqtt.broker);
  s+=getConfig("mqttPort",        mqtt.port);
  s+=getConfig("mqttIdPrefix",    mqtt.idPrefix);
  s+=getConfig("mqttUser",        mqtt.user);
  s+=getConfig("mqttPwd",         mqtt.pwd);
  s+=getConfig("mqttTopic",       mqtt.topic);
  s[s.length()-1]='}';
  return s;
}String getStatus(){
  String s('{');
  s+=getConfig("ipAddr",          WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString());
  s+=getConfig("uptime",          millis());
  s+=getConfig("pinState",        pin.state);
  s[s.length()-1]='}';
  return s;
}

void handleRoot(){
  ESPWebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  ESPWebServer.send(200, "text/html", F("<!DOCTYPE HTML>\n<html lang='us-US'>\n<head><meta charset='utf-8'/>\n"));
#ifdef WEBUI
ESPWebServer.sendContent(F("<meta http-equiv='refresh' content='0;URL="));
ESPWebServer.sendContent(WEBUI);
ESPWebServer.sendContent(F("'>\n<head>\n<body>\nLoading...\n"));
#else
ESPWebServer.sendContent(F("<title id=title name='hostname'>ESP8266</title>\n<style>\n\
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
<body onload='init();'>\n\
<div id='about' class='modal'><div class='modal-content'><span class='close' onClick='refresh();'>&times;</span><h1>About</h1>\
This WiFi Power Strip is a connected device that allows you to control the status of its outlets from a home automation application like Domoticz or Jeedom.<br><br>\
In addition, it also has its own WEB interface which can be used to configure and control it from a web browser (the firmware can also be upgraded from this page). \
Otherwise, its URL is used by the home automation application to control it, simply by forwarding the desired state on each of the outputs, like this:\
<a id='example1' style='padding:0 0 0 5px;'></a>...<br><br>\
The state of the electrical outlets can also be requested from the following URL: <a id='example2' style='padding:0 0 0 5px;'></a>.\
 In addition, a MQTT server can be notified of each of the state changes in order, for example, to relay the state of the switches (on manual action) to the centralized home automation interface.<br><br>\
The status of the power strip can be saved when the power is turned off and restored when it is turned on ; a power-on duration can be set on each output: (-1) no delay, (0) to disable a specific output and (number of s) to configure the power-on duration (but this timer can be temporarily disabled by holding the switch for 3s).<br><br>\
A second slave module (without any declared WiFi SSID) can be connected to the first (which thus becomes master by automatically detecting its slave) through its UART interface (USB link) in order to increase the number of outputs to a maximum of 12 on the same management interface. \
The manual action of these additional physical switches adds them automatically to the web interface (on refresh). The 'clear' button can be used to delete them when removing the slave module.<br><br>\
The following allows you to configure some parameters of the Wifi Power Strip (as long as no SSID is set and it is not connected to a master, the device acts as an access point with its own SSID and default password: \n\
'<span id='defaultHostname'>ESP8266/defaultPassword</span>' on 192.168.4.1).<br><br>\n\
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
<input id='Clear' type='button' value='Clear' onclick='clearSerialDevice);'>&nbsp;\n\
<input id='reboot' name='reboot' type='checkbox' style='display:none;' checked>\n\
</td></tr>\n</table>\n\
<br><h3>Network connection [<span id='macAddr'>00:00:00:00:00:00</span>] (device ident: <span id='ident'>Ident</span>):</h3>\n\
<table id='ssids' style='width:100%;'></table>\n\
<h6><a href='update' onclick='javascript:event.target.port=80'>Firmware update</a> - <a href='https://github.com/peychart/wifiPowerStrip'>Website here</a></h6>\n\n\
</div></div>\n\
<!-============MAIN FORM============->\n\
<table style='width:100%;'>\n<tr style='height:75px;'>\n\
<td style='width:800px;'><h1><span name='hostname'>ESP8266</span> - <span id='ipAddr'>255.255.255.255</span></h1></td>\n\
<td style='text-align:right;vertical-align:top;'><div style='text-align:right;white-space: nowrap;'><p><span class='close' onclick='showHelp();'>?</span></p></div></td>\n\
</tr></table>\n\
<h3>Status :</h3><table id='switches' style='width:100%;'></table>\n<h6>(V<span id='version'></span>, Uptime: <span id='uptime'></span>)</h6>\n\
<!-============Config POPUP->\n\
<div id='confPopup' class='confPopup'><div id='mqttConf'>\n\
<input id='plugNumber' type='text' style='display:none;'>\n\
<a title='Save configuration' class='closeconfPopup' onclick='closeConfPopup();'>X</a>\n\
<table style='width:100%;'><col width=90%><tr><td><div style='text-align:center;'><h2>'<span id='confName'></span>' configuration:</h2></div></td></tr></table>\n\
<h3 title='for this switch'>Output parameters</h3>\n\
<table title='for this switch' style='width:100%;'>\n<col width='50%'>\n<tr>\n\
<td style='text-align:center;'>Plug Name<br><input id='plugName' type='text' pattern='^[a-zA-Z][a-zA-Z0-9]*$' style='width:150px;' onchange='plugnameSubmit(this);'>\n\
</td>\n<td align=center>Reverse level<br><input id='outputReverse' type='checkbox' style='vertical-align:middle;' onclick='reverseSubmit(this);'>\n\
</td>\n</tr></table>\n<h3 title='for this switch'>MQTT parameters</h3>\n\
<!-============MQTT configuration:->\n\
<div id='mqttParams'><p align=center title='for all switches'>Broker: <input id='mqttBroker' type='text' pattern='^[a-zA-Z][a-zA-Z0-9/\\.]*$' style='width:65%;' onchange='mqttBrokerSubmit(this);'>\n\
:<input id='mqttPort' type='number' min='0' max='65535' style='width:10%;' onchange='mqttPortSubmit(this);'>\n\
</p>\n<table style='width:100%;'>\n<col width='42%'><col width='30%'><tr title='for all switches' style='white-space: nowrap;'><td>\n\
Identification: <input id='mqttIdent' type='text' pattern='^[a-zA-Z][a-zA-Z0-9]*$' style='width:120px;' onchange='mqttIdentSubmit(this);'>\n\
</td><td>\nUser: <input id='mqttUser' type='text' pattern='^[a-zA-Z][a-zA-Z0-9]*$' style='width:120px;' onchange='mqttUserSubmit(this);'>\n\
</td><td>\nPassword: <input id='mqttPwd' type='password' style='width:75px;' onchange='mqttPwdSubmit(this);'>\n\
</td></tr></table>\n<p align=center title='for all switches'>Topic: <input id='mqttTopic' type='text' style='width:80%;' onchange='mqttTopicSubmit(this);'>\n\
</p>\n</div></div></div>\n\
\n\
<!--FRAME /dev/null--><iframe name='blankFrame' height='0' width='0' frameborder='0'></iframe>\n\
<!-==========JScript==========->\n\
<script>this.timer=0; parameters={'ipAddr':'"));
ESPWebServer.sendContent(WiFiAP ?WiFi.softAPIP().toString() :WiFi.localIP().toString());
ESPWebServer.sendContent(F("'};\nfunction init(){RequestDevice('getConf');refresh(1);}\n\
function refresh(v=20){\n\
 clearTimeout(this.timer);document.getElementById('about').style.display='none';\n\
 if(v>0)this.timer=setTimeout(function(){RequestDevice('getStatus');refresh();},v*1000);}\n\
function RequestDevice(url){\n\
 var req=new XMLHttpRequest(), requestURL=location.protocol+'//'+parameters.ipAddr+'/'+url;\n\
 req.open('POST',requestURL);req.responseType='json';req.send();\n\
 if(url=='getConf')req.onload=function(){parameters=req.response;displayNTP();createSwitches();displayDelays();}\n\
 else req.onload=function(){parameters.ipAddr=req.response.ipAddr;refreshSwitches(req.response);}\n\
}\n\
function getGpioParam(name,i,p=parameters){return (p?p[name][getGpioNumber(i)]:null);}\n\
function getGpioCount(){return parameters.pinGpio.length;}\n\
function getGpioNumber(i){return parameters.pinGpio[i];}\n\
function getGpioState(i,status){return getGpioParam('pinState',i,status);}\n\
//===========Actions:\n\
function showHelp(){var v,e=document.getElementById('example1');\n\
 e.innerHTML=location.protocol+'//'+parameters.ipAddr+'/plugValues?';\n\
 v=document.getElementById('switch0');e.innerHTML+=v.name+'='+(v.checked?'true':'false');\n\
 for(var i=1; i<3 &&(v=document.getElementById('switch'+i));i++)e.innerHTML+='&'+v.name+'='+(v.checked?'true':'false');e.href=e.innerHTML;\n\
 e=document.getElementById('example2');e.innerHTML=location.protocol+'//'+parameters.ipAddr+'/plugValues';e.href=e.innerHTML;\n\
 refresh(120);document.getElementById('about').style.display='block';\n\
}\n\
function ssidSubmit(e){var f;for(f=e;f.tagName!='TABLE';)f=f.parentNode;e=f.querySelectorAll('input[type=text]');\n\
 if(e[0].value==''){alert('Empty SSID...');e[0].focus();}\n\
 else{ var p=f.querySelectorAll('input[type=password]');\n\
  if(p[0].value.length<6){alert('Incorrect password...');p[0].focus();}\n\
  else{RequestDevice('setConf?ssid='+e[0].value+','+p[0].value+'');e[0].readOnly=p[0].readOnly=true;p[0].value='************';}\n\
}}\n\
function deleteSSID(e){var f;for(f=e;f.tagName!='TABLE';)f=f.parentNode;\n\
 e=f.querySelectorAll('input[type=text]');\n\
 if(e[0].value!='' && confirm('Are you sure to remove this SSID?')){\n\
  var p=f.querySelectorAll('input[type=password]');\n\
  RequestDevice('setConf?ssid='+e[0].value);e[0].value=p[0].value='';e[0].readOnly=p[0].readOnly=false;\n\
  p=f.querySelectorAll('input[type=button]'); p[0].disabled=!(p[1].disabled=true);\n\
}}\n\
function checkHostname(e){\n\
 if(e.value.length && (e.value && e.value!=document.getElementById('title').value))\n\
  document.getElementById('hostnameSubmit').disabled=false;\n\
 else document.getElementById('hostnameSubmit').disabled=true;\n\
}\nfunction setHostName(n){var v=document.getElementsByName('hostname');\n\
 for(var i=0;i<v.length;i++) if(v[i].value) v[i].value=n;else v[i].innerHTML=n;\n\
}\nfunction hostnameSubmit(){var e=document.getElementById('hostname');\n\
 setHostName(e.value);document.getElementById('hostnameSubmit').disabled=true;\n\
 RequestDevice('script?edit=H'+e.value);\n\
}\n\
function displayNTP(){if(parameters.ntpSource!='')document.getElementById('ntpLib').style=document.getElementById('ntp').style='display:inline-block;'}\n\
function checkNTP(e){\n\
 if(document.getElementById('ntpSource').value!=parameters.source || document.getElementById('ntpZone').value!=parameters.zone || document.getElementById('ntpDayLight').checked!=parameters.dayLight)\n\
  document.getElementById('ntpSubmit').disabled=false;else document.getElementById('ntpSubmit').disabled=true;\n\
}\n\
function ntpSubmit(e){var cmd='script?edit';\n\
 cmd+='=N'+document.getElementById('ntpSource').value;\n\
 cmd+=','+document.getElementById('ntpZone').value;\n\
 cmd+=','+(document.getElementById('ntpDayLight').checked?1:0);\n\
 RequestDevice(cmd);e.disabled=true;\n\
}\n\
function switchSubmit(e){var t,b=false;\n\
 for(t=e;t.tagName!='TR';)t=t.parentNode;t=t.getElementsByTagName('input');\n\
 for(var i=0;i<t.length;i++)if(t[i].type=='number')b|=Number(t[i].value); //Check if delay!=0\n\
 if(b){var i=getGpioNumber((i=e.id).replace('switch','')), cmd='script?cmd='$'+i+',1';\n\
  if(e.checked && !document.getElementById(e.id+'-timer').disabled && document.getElementById(e.id+'-timer').checked)\n\
   cmd+=' T'+i+' T'+i+',$'+'~'+i;\n\
  RequestDevice(cmd);\n\
}}\n\
function displayDelay(v,i){var e,b=false;\n\
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
function delaySubmit(e){var cmd,v=0,w,i=parseInt(e.id.substring(6, e.id.indexOf('-')));\n\
 v+=Number(document.getElementById('switch'+i+'-d-duration').value)*86400;\n\
 w=Number(document.getElementById('switch'+i+'-h-duration').value);if(w>0||v)v+=w*3600;\n\
 w=Number(document.getElementById('switch'+i+'-mn-duration').value);if(w>0||v)v+=w*60;\n\
 v+=Number(document.getElementById('switch'+i+'-s-duration').value);\n\
 clearTimeout(checkDisplayDelay);checkDisplayDelay=setTimeout(function(){displayDelay(v,i);},500);\n\
 document.getElementById('delayOn'+i).value=((v&&(v+1))?(v*1000):-1);\n\
 cmd='S'+getGpioNumber(i)+','+getGpioParam('pinMode',i)+','+(getGpioParam('pinName',i)||('switch'+i))+','+((v&&(v+1))?(v*1000):-1);\n\
 clearTimeout(checkSubmitDelay);checkSubmitDelay=setTimeout(function(){checkSubmitDelay=0;RequestDevice('script?edit='+cmd);}, 3000);\n\
}\n\
//===========Create the page:\n\
function addParameters(i){var v,d=document.createElement('div');d.style='display:none;';\n\
 d.appendChild(v=document.createElement('input'));v.type='checkbox';v.id='outputReverse'+i;\n\
 d.appendChild(v=document.createElement('input'));v.type='text';v.id='delayOn'+i;v.value=(getGpioParam('gpioVar',i)||-1);\n\
 return d;\n\
}\nfunction addTheBullet(i){var d=document.createElement('div'),b; d.appendChild(b=document.createElement('button'));\n\
 b.id=i; b.classList.add('bulle'); b.title=(getGpioParam('pinName',i)||('switch'+i))+' configuration';b.setAttribute('onclick','initConfPopup(this);');\n\
 return d;\n\
}\nfunction addTheSwitchName(i){var d=document.createElement('div');\n\
 d.classList.add('outputName'); d.innerHTML=(getGpioParam('pinName',i)||('switch'+i));return d;\n\
}\nfunction addTheSwitch(i){var l,s,d;\n\
 d=document.createElement('div');d.classList.add('onoffswitch');d.classList.add('delayConf');\n\
 d.appendChild(l=document.createElement('input'));l.type='checkbox';l.classList.add('onoffswitch-checkbox');\n\
 l.id='switch'+i;l.name=(getGpioParam('pinName',i)||('switch'+i));l.setAttribute('onclick','switchSubmit(this);');\n\
 d.appendChild(l=document.createElement('label'));l.classList.add('onoffswitch-label');l.setAttribute('for','switch'+i);\n\
 l.appendChild(s=document.createElement('span'));s.classList.add('onoffswitch-inner');\n\
 l.appendChild(s=document.createElement('span'));s.classList.add('onoffswitch-switch');\n\
 return d;\n\
}\nfunction displayDelays(){\n\
 for(var i=0;i<getGpioCount();i++)if(getGpioParam('pinMode',i)<2)displayDelay(Number(getGpioParam('gpioVar',i))/1000,i);\n\
}\nfunction addTheDelay(i){var v,d,ret=document.createElement('div');\n\
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
}\nfunction addRawSwitch(i){var v,rawSwitch;\n\
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
}\nfunction createSwitches(){var e;setSSID();\n\
 setHostName(document.getElementById('hostname').value=parameters.hostname);\n\
 if(parameters.ipAddr.length)document.getElementById('ipAddr').innerHTML=parameters.ipAddr;\n\
 if(parameters.macAddr.length)document.getElementById('macAddr').innerHTML=parameters.macAddr;\n\
 if(parameters.ident.length)document.getElementById('ident').innerHTML=parameters.ident;\n\
 e=document.getElementById('defaultHostname');\n\
 e.innerHTML =(parameters.defaultHostname.length?parameters.defaultHostname:'defaultHostname')+'/';\n\
 e.innerHTML+=(parameters.defaultPassword.length?parameters.defaultPassword:'defaultPassword');\n\
 if(parameters.ntpSource.length)document.getElementById('ntpSource').value=parameters.ntpSource;\n\
 if(parameters.ntpZone.length)document.getElementById('ntpZone').value=parameters.ntpZone;\n\
 if(parameters.ntpDayLight.length)document.getElementById('ntpDayLight').checked=(parameters.ntpDayLight!='0');\n\
 if(parameters.version.length)document.getElementById('version').innerHTML=parameters.version;else document.getElementById('version').innerHTML='?';\n\
 for(var i=0,td,tr,e=document.getElementById('switches');i<getGpioCount(); i++)if(getGpioParam('pinMode',i)<2){\n\
  e.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
  td.classList.add('switches');td.appendChild(addRawSwitch(i));\n\
}}\nfunction refreshUptime(sec){var e;\n\
 if((e=document.getElementById('uptime'))){sec/=1000;\n\
  e.innerHTML=Math.trunc(sec/(24*3600));e.innerHTML+='d-';\n\
  e.innerHTML+=Math.trunc((sec%=24*3600)/3600);e.innerHTML+='h-';\n\
  e.innerHTML+=Math.trunc((sec%=3600)/60);e.innerHTML+='mn';\n\
}}\nfunction refreshSwitches(status){\n\
  refreshUptime(status.uptime);\n\
 for(var i=0;i<getGpioCount();i++)if(getGpioParam('pinMode',i)<2){var e;\n\
  if((e=document.getElementById('switch'+i)))e.checked=(getGpioState(i,status) ?true :false);\n\
  document.getElementById('switch'+i+'-timer').disabled=!(document.getElementById('switch'+i+'-timer').checked=!document.getElementById('switch'+i).checked);\n\
  if(document.getElementById('switch'+i+'-s-duration').value==-1)document.getElementById('switch'+i+'-timer').checked=(document.getElementById('switch'+i+'-timer').disabled=true);\n\
}}\nfunction setSSID(){var e,v,table,td,tr;\n\
 if((e=document.getElementById('ssids'))){e.appendChild(tr=document.createElement('tr'));e=tr;\n\
  for(var i=1;i<=parameters.ssid.length;i++){\n\
   e.appendChild(td=document.createElement('td'));td.appendChild(table=document.createElement('table'));\n\
   table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
   td.innerHTML='SSID '+i+':';\n\
   table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
   td.appendChild(v=document.createElement('input'));v.type='text';v.id='ssid'+i;\n\
   table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
   td.innerHTML='Password:';\n\
   table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
   td.appendChild(v=document.createElement('input'));v.type='password';v.id='pwd'+i;\n\
   table.appendChild(tr=document.createElement('tr'));tr.appendChild(td=document.createElement('td'));\n\
   td.appendChild(v=document.createElement('input'));v.id='addSSID'+i;\n\
   v.type='button';v.value='Submit';v.setAttribute('onClick','ssidSubmit(this);');\n\
   td.appendChild(v=document.createElement('span'));v.innerHTML='&nbsp;';\n\
   td.appendChild(v=document.createElement('input'));v.id='removeSSID'+i;\n\
   v.type='button';v.value='Remove';v.setAttribute('onclick','deleteSSID(this);');\n\
 }}for(var i=1;i<=parameters.ssid.length;i++)if(parameters.ssid[i-1]){var v;\n\
  (v=document.getElementById('ssid'+i)).value=parameters.ssid[i-1];v.readOnly=true;\n\
  (v=document.getElementById('pwd'+i)).value='************';v.readOnly=true;\n\
  document.getElementById('addSSID'+i).disabled=!(document.getElementById('removeSSID'+i).disabled=false);\n\
 }else{ document.getElementById('ssid'+i).value=document.getElementById('pwd'+i).value='';\n\
  document.getElementById('addSSID'+i).disabled=!(document.getElementById('removeSSID'+i).disabled=true);\n\
}}\n\
\n\
//===========MQTT management:\n\
function setDisabled(v, b){for(var i=0;v[i];i++)v[i].disabled=b;}\n\
\n\
function reverseSubmit(e){var cmd,i=document.getElementById('plugNumber').value;\n\
 cmd='S'+getGpioNumber(i)+','+(e.checked?1:0)+','+(getGpioParam('pinName',i)||('switch'+i))+','+document.getElementById('delayOn'+i).value;\n\
 parameters.pinMode[getGpioNumber(i)]=e.checked?1:0; RequestDevice('script?edit='+cmd);\n\
}\n\
function checkPlugName(e){return (e.value===''?false:true);}\n\
function plugnameSubmit(e){if(checkPlugName(e)){var cmd,i=document.getElementById('plugNumber').value;\n\
 cmd='S'+getGpioNumber(i)+','+getGpioParam('pinMode',i)+','+e.value+','+document.getElementById('delayOn'+i).value;\n\
 parameters.pinName[getGpioNumber(i)]=e.value?1:0; RequestDevice('script?edit='+cmd);\n\
}}\n\
function checkMqttBroker(e){return true;}\n\
function mqttBrokerSubmit(e){if(checkMqttBroker(e)){RequestDevice('setConf?mqttBroker='+e.value);};checkConfPopup();}\n\
function checkMqttPort(e){return(Number(e.value)!=0);}\n\
function mqttPortSubmit(e){if(checkMqttPort(e)){RequestDevice('setConf?mqttPort='+e.value);};checkConfPopup();}\n\
function checkMqttIdent(e){return true;}\n\
function mqttIdentSubmit(e){if(checkMqttIdent(e)){RequestDevice('setConf?mqttIdent='+e.value);};checkConfPopup();}\n\
function checkMqttUser(e){return true;}\n\
function mqttUserSubmit(e){if(checkMqttUser(e)){RequestDevice('setConf?mqttUser='+e.value);};checkConfPopup();}\n\
function checkMqttPwd(e){return true;}\n\
function mqttPwdSubmit(e){if(checkMqttPwd(e)){RequestDevice('setConf?mqttPwd='+e.value);};checkConfPopup();}\n\
function checkMqttTopic(e){return true;}\n\
function mqttTopicSubmit(e){if(checkMqttTopic(e)){RequestDevice('setConf?mqttTopic='+e.value);};checkConfPopup();}\n\
function checkMqttFieldName(e){return(e.value!='');}\n\
\n\
function checkConfPopup(){\n\
 if(!checkPlugName(document.getElementById('plugName')))return false;\n\
 if(!checkMqttBroker(document.getElementById('mqttBroker')))return false;\n\
 if(!checkMqttPort(document.getElementById('mqttPort')))return false;\n\
 return true;\n\
}\n\
function refreshConfPopup(){checkConfPopup();}\n\
function initConfPopup(e){var i=e.id,f;\n\
 /*document.getElementById('confPopup').setAttribute('target','blankFrame');*/window.location.href='#confPopup';\n\
 document.getElementById('plugNumber').value=e.id;\n\
 document.getElementById('confName').innerHTML='Switch'+i;\n\
 document.getElementById('plugName').value=(getGpioParam('pinName',i)||('switch'+i));\n\
 document.getElementById('outputReverse').checked=getGpioParam('pinMode',i);\n\
 document.getElementById('mqttBroker').value=parameters.mqttBroker;\n\
 document.getElementById('mqttPort').value=parameters.mqttPort;\n\
 document.getElementById('mqttIdent').value=parameters.mqttIdPrefix;\n\
 document.getElementById('mqttUser').value=parameters.mqttUser;\n\
 document.getElementById('mqttPwd').value=parameters.mqttPwd;\n\
 document.getElementById('mqttTopic').value=parameters.mqttTopic;\n\
 refreshConfPopup();\n\
}\n\n\
function closeConfPopup(){\n\
 if(checkConfPopup()){\n\
  ;\n\
 }else if(!confirm('Are you sure to cancel modifications?')) return;\n\
 window.location.href='';\n\
}\n\
</script>\n"));
#endif
  ESPWebServer.sendContent(F("</body>\n</html>\n\n"));
  ESPWebServer.sendContent(""); ESPWebServer.client().stop();
}

void setConfig(){
  if(ESPWebServer.hasArg("ssid")){
    String cmd=ESPWebServer.arg("ssid");
    short sep=cmd.indexOf(',');
    if(sep+1){  //Add SSID:
      ssid[SSIDCount()-1]=cmd.substring(0, sep);
      password[SSIDCount()-1]=cmd.substring(sep+1);
    }else for(int i=0; i<SSIDCount(); i++) if(ssid[i]==cmd) ssid[i]=password[i]=""; //SSID removed.
    shiftSSID(); if((WiFiAP && ssid[0].length()) || (!WiFiAP && !ssid[0].length())) reboot();
  }else if(ESPWebServer.hasArg("mqttBroker")){
    mqtt.broker=ESPWebServer.arg("mqttBroker");
  }else if(ESPWebServer.hasArg("mqttPort")){
    mqtt.port=ESPWebServer.arg("mqttPort").toInt();
  }else if(ESPWebServer.hasArg("mqttIdent")){
    mqtt.idPrefix=ESPWebServer.arg("mqttIdent");
  }else if(ESPWebServer.hasArg("mqttUser")){
    mqtt.user=ESPWebServer.arg("mqttUser");
  }else if(ESPWebServer.hasArg("mqttPwd")){
    mqtt.pwd=ESPWebServer.arg("mqttPwd");
  }else if(ESPWebServer.hasArg("mqttTopic")){
    mqtt.topic=ESPWebServer.arg("mqttTopic");
  }writeConfig();
}

inline void script(String s=ESPWebServer.arg("cmd")){
  if(ESPWebServer.hasArg("key")){         //Send the public key:
    ESPWebServer.send(200, "text/plain", ENCRYPTION_KEY);
    writeConfig();
  }else if(ESPWebServer.hasArg("get")){   //Send the default script:
    ESPWebServer.send(200, "text/plain", defaultScript);
  }else if(ESPWebServer.hasArg("edit")){   //Set the default script:
    String s=ESPWebServer.arg("edit");
    if(s[0]=='S'){
      String old=s.substring(0, s.indexOf(','));
      ushort i=defaultScript.indexOf(old+',');
      if(!(i+1)) return;
      old=defaultScript.substring(i, defaultScript.indexOf(';', i));
      DEBUG_print("Old command: "+old+"\n");
      DEBUG_print("New command received: "+s+"\n");
      defaultScript.replace(old, s);
      DEBUG_print("script send: "+s.substring(0,s.indexOf(','))+" "+s+"\n");
      treatment(s.substring(0,s.indexOf(','))+" "+s);
      ESPWebServer.send(200, "json/plain", getStatus());
      writeConfig();
    }else if(s[0]=='H' || s[0]=='N'){
      String old(s[0]);
      ushort i=defaultScript.indexOf(';'+old)+1;
      if(i || defaultScript[0]==s[0]){
        old=defaultScript.substring(i, defaultScript.indexOf(';', i));
        defaultScript.replace(old, s);
        DEBUG_print("Old script: "+old+"\n");
      }else defaultScript=s+';'+defaultScript;
      DEBUG_print("New script received: "+s+"\n");
      DEBUG_print("script send: "+s+"\n");
      treatment(s);
      ESPWebServer.send(200, "json/plain", getStatus());
      writeConfig();
    }
  }else if(ESPWebServer.hasArg("set")){   //Set the default script:
    String s=ESPWebServer.arg("set");
    ;
    defaultScript=s;
    ESPWebServer.send(200, "text/plain", "ok");
  }else if(ESPWebServer.hasArg("cmd")){   //Do a script commend:
    DEBUG_print("Command received: "+s+"\n");
    treatment(s); treatment(defaultScript);
    ESPWebServer.send(200, "json/plain", getStatus());
    return;
} }

void setupWebServer(){
  //Definition des URLs d'entree /Input URL definitions
  ESPWebServer.on("/",         [](){handleRoot(); ESPWebServer.client().stop();});
  ESPWebServer.on("/setConf",  [](){setConfig();  ESPWebServer.send(200, "text/plain", getStatus());});
  ESPWebServer.on("/getConf",  [](){              ESPWebServer.send(200, "json/plain", getConfig());});
  ESPWebServer.on("/getStatus",[](){              ESPWebServer.send(200, "json/plain", getStatus());});
  ESPWebServer.on("/script",   [](){script();});
  ESPWebServer.on("/restart",  [](){reboot(); handleRoot(); ESPWebServer.client().stop();});
//ESPWebServer.on("/about",    [](){ ESPWebServer.send(200, "text/plain", getHelp()); });
  ESPWebServer.onNotFound([](){ESPWebServer.send(404, "text/plain", "404: Not found");});

  ESPWebServer.begin();              //Demarrage du serveur web /Web server start
  Serial_print("HTTP server started\n");
}
