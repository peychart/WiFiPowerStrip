#include "common.h"

//String defaultScript("S2,0,1;G2?Toff,2500;Toff?!G2 Toff Ton,1000;Ton?G2 Ton;");  //Blink...

//To remove:
String defaultScript("\
HLumibloc;\
S16,0,Bar,15000;\
S5,0,Couloir;\
S4,0,Salon;\
S0,0,Chambre2;\
S2,0,Chambre1;\
S15,0,Bureau;\
S14,3;\
S12,3;\
S13,3;\
TnotHold?THold TnotHold;\
!Thold !~14 ~12 ~13?T0,100;\
!T0 ~14?T0;\
!T0 !~12?T0;\
!T0 !~13?T0;\
T0 !~14 ~16?Thold,3000 TGPIO16,$~16;\
T0 !~14 Thold?TGPIO16 T0 TnotHold TnotHold,1000;\
T0 ~14 ~16?!~16 M{\"idx\":150,\"nvalue\":1,\"svalue\":\"1\"} Thold TGPIO16 T0;\
T0 ~14 !~16?~16 M{\"idx\":150,\"nvalue\":0,\"svalue\":\"0\"} TGPIO16,$~16 T0;\
TGPIO16?!~16 M{\"idx\":150,\"nvalue\":0,\"svalue\":\"0\"} TGPIO16;\
!Thold ~14 !~12 ~13?T1,100;\
!T1 !~14?T1;\
!T1 ~12?T1;\
!T1 !~13?T1;\
T1 !~12 ~5?Thold,3000 TGPIO5,$~5;\
T1 !~12 Thold?TGPIO5 T1 TnotHold TnotHold,1000;\
T1 ~12 ~5?!~5 M{\"idx\":151,\"nvalue\":1,\"svalue\":\"1\"} Thold TGPIO5 T1;\
T1 ~12 !~5?~5 M{\"idx\":151,\"nvalue\":0,\"svalue\":\"0\"} TGPIO5,$~5 T1;\
TGPIO5?!~5 M{\"idx\":151,\"nvalue\":0,\"svalue\":\"0\"} TGPIO5;\
!Thold !~14 !~12 ~13?T2,100;\
!T2 ~14?T2;\
!T2 ~12?T2;\
!T2 !~13?T2;\
T2 !~14 ~4?Thold,3000 TGPIO4,$~4;\
T2 !~14 Thold?TGPIO4 T2 TnotHold TnotHold,1000;\
T2 ~14 ~4?!~4 M{\"idx\":152,\"nvalue\":1,\"svalue\":\"1\"} Thold TGPIO4 T2;\
T2 ~14 !~4?~4 M{\"idx\":152,\"nvalue\":0,\"svalue\":\"0\"} TGPIO4,$~4 T2;\
TGPIO4?!~4 M{\"idx\":152,\"nvalue\":0,\"svalue\":\"0\"} TGPIO4;\
!Thold ~14 ~12 !~13?T3,100;\
!T3 !~14?T3;\
!T3 !~12?T3;\
!T3 ~13?T3;\
T3 !~13 ~0?Thold,3000 TGPIO0,$~0;\
T3 !~13 Thold?TGPIO0 T3 TnotHold TnotHold,1000;\
T3 ~13 ~0?!~0 M{\"idx\":153,\"nvalue\":1,\"svalue\":\"1\"} Thold TGPIO0 T3;\
T3 ~13 !~0?~0 M{\"idx\":153,\"nvalue\":0,\"svalue\":\"0\"} TGPIO0,$~0 T3;\
TGPIO0?!~0 M{\"idx\":153,\"nvalue\":0,\"svalue\":\"0\"} TGPIO0;\
!Thold !~14 ~12 !~13?T4,100;\
!T4 ~14?T4;\
!T4 !~12?T4;\
!T4 ~13?T4;\
T4 !~13 ~2?Thold,3000 TGPIO2,$~2;\
T4 !~13 Thold?TGPIO2 T4 TnotHold TnotHold,1000;\
T4 ~13 ~2?!~2 M{\"idx\":154,\"nvalue\":1,\"svalue\":\"1\"} Thold TGPIO2 T4;\
T4 ~13 !~2?~2 M{\"idx\":154,\"nvalue\":0,\"svalue\":\"0\"} TGPIO2,$~2 T4;\
TGPIO2?!~2 M{\"idx\":154,\"nvalue\":0,\"svalue\":\"0\"} TGPIO2;\
!Thold ~14 !~12 !~13?T5,100;\
!T5 !~14?T5;\
!T5 ~12?T5;\
!T5 ~13?T5;\
T5 !~13 ~15?Thold,3000 TGPIO15,$~15;\
T5 !~13 Thold?TGPIO15 T5 TnotHold TnotHold,1000;\
T5 ~13 ~15?!~15 M{\"idx\":155,\"nvalue\":1,\"svalue\":\"1\"} Thold TGPIO15 T5;\
T5 ~13 !~15?~15 M{\"idx\":155,\"nvalue\":0,\"svalue\":\"0\"} TGPIO15,$~15 T5;\
TGPIO15?!~15 M{\"idx\":155,\"nvalue\":0,\"svalue\":\"0\"} TGPIO15;\
");

std::map<String,String>           var;
std::map<String,ulong>            timer;

extern String                     hostname, ssid[SSIDCount()], serialInputString;
extern bool                       isSlave;

extern struct ntpConf{
  String                          source;
  short                           zone;
  bool                            dayLight;
} ntp;

extern struct pinConf{
  std::vector<String>             gpio;
  std::map<String,String>         name, gpioVar;
  std::map<String,bool>           state;
  std::map<String,ushort>         mode;
} pin;

extern bool                       mustResto;
extern PubSubClient               mqttClient;

extern struct mqttConf{
  String                          broker, idPrefix, user, pwd, topic;
  short                           port;
} mqtt;

#ifdef DEBUG
  extern WiFiServer               telnetServer;
  extern WiFiClient               telnetClient;
#endif


void setAllPinsOnSlave(){
  for(ushort i(0); i<pin.gpio.size(); i++)
    if(!pin.mode[pin.gpio[i]])
      Serial_print("S(" + String(i, DEC) + "):" + ((pin.state[String(i,DEC)]=((RESTO_VALUES_ON_BOOT || mustResto) ?pin.state[String(i,DEC)] :false)) ?"1\n" :"0\n"));
}

void serialSwitchsTreatment(unsigned int serialBufferLen=serialInputString.length()){
/*  if(serialBufferLen>4){
    if(isMaster()){
      if(serialInputString.startsWith("M(") && serialInputString[3]==')'){          //Setting Master from Slave:
        if(serialInputString[2]=='?'){
          setAllPinsOnSlave();
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

    }else if(serialInputString.startsWith("S(") && serialInputString[3]==')'){      //Setting Slave from Master:
      if(serialInputString[2]=='.')
        reboot();
      else if(serialInputString[4]==':' && (ushort)(serialInputString[2]-'0')<_outputPin.size())
        setPin(serialInputString[2]-'0', (serialInputString[5]=='1'), true);
      if(!isSlave()) DEBUG_print("I'm now the Slave.\n");
      isSlave()=true;

    }serialInputString=serialInputString.substring(serialBufferLen);
}*/ }

void mySerialEvent(){char inChar;
  if(Serial) while(Serial.available()){
    serialInputString += (inChar=(char)Serial.read());
    if(inChar=='\n')
      serialSwitchsTreatment();
} }

void setVar(String s){
  if(s.startsWith("$")) s.remove(0,1);
  if(s.length()){
    short sep(s.indexOf(','));
    if(sep+1 && var.find(s.substring(0,sep))==var.end())
          var[s.substring(0,sep)]=s.substring(sep+1);
    else  var.erase(s.substring(0,sep));
} }

String getVar(String s){
  if(!s.startsWith("$")) return s;
  s.remove(0, 1);
  return (var.find(s)==var.end() ?"" :var[s]);
}

bool iSNumber(String& s){
  if(!isDigit(s[0]) && s[0]!='-')           return false;
  for(ushort i(0); i<s.length(); i++){bool b(false);
    if(!b && (b=(s[i]=='.' || s[i]==',')))  continue;
    if(!isDigit(s[i]))                      return false;
  }if(!isDigit(s[s.length()-1]))            return false;
  s.replace(',','.');
  return true;
}

void incrVar(String s){
  if(s.startsWith("$")) s.remove(0, 1);
  if(s.endsWith(","))   s.remove(s.length()-1);
  if(s.length()){short sep(s.indexOf(','));
    if(sep+1){String s1(s.substring(0,sep)), s2(getVar(s.substring(++sep)));
      if(iSNumber(s1) && iSNumber(s2)){
        if((s1+s2).indexOf('.')+1)
              var[s1]=String(var[s1].toFloat()+s2.toFloat());
        else  var[s1]=String(var[s1].toInt()+s2.toInt());
      }else var[s1]+=s2;
    }else if(iSNumber(s)) incrVar(s+",1");
} }

void setHostname(String s) {hostname=getVar(s);}

void setNTP(String s){
  if(s.length()){
    short sep(s.indexOf(','));
    ntp.source=getVar(s.substring(0, sep));
    if(++sep){s.remove(sep); sep=s.indexOf(',');
      ntp.zone=getVar(s.substring(0, sep)).toInt();
      if(++sep){s.remove(sep); sep=s.indexOf(',');
        ntp.dayLight=getVar(s.substring(0, sep)).toInt();
} } } }

void setPinMode(String s){  //Format: pinNumber,mode[,G'pinNumber'_initial_value]
  short sep(s.indexOf(','));
  String p=s.substring(0, sep);
  if(!(sep+1)){
    pin.mode.erase(p);
    return;
  }if(pin.mode.find(p)==pin.mode.end()){
    s.remove(0, sep); s.remove(0, 1); pin.gpio.push_back(p);
    switch(pin.mode[p]=s.substring(0, ',').toInt()){
      case  0:                                                  //reverse_output=false
      case  1: s.remove(0, s.indexOf(',')); s.remove(0, 1);     //reverse_output=true
        pinMode(p.toInt(), OUTPUT);
        break;
      case  2: s.remove(0, s.indexOf(',')); s.remove(0, 1);
        pinMode(p.toInt(), INPUT);
        break;
      default: s.remove(0, s.indexOf(',')); s.remove(0, 1);
        pinMode(p.toInt(), INPUT_PULLUP);
    }if(s.length()){
      pin.name[p]=getVar(s.substring(0, (sep=s.indexOf(',')))); s.remove(0, sep); s.remove(0, 1);
    }if(s.length()){
//DEBUG_print("Var["); DEBUG_print("G"+pin.gpio); DEBUG_print("]="); DEBUG_print(getVar(s.substring(0,s.indexOf(",")))); DEBUG_print("\n");
      pin.gpioVar[p]=getVar(s.substring(0, s.indexOf(',')));
      setVar("~"+p+","+pin.gpioVar[p]);
} } }

void setPin(String pinout, bool state, bool withNoTimer){
  if(pin.mode.find(pinout)!=pin.mode.end() && pin.mode[pinout]<2 && pin.state[pinout]!=state){
    digitalWrite(pinout.toInt(), (pin.state[pinout]=state) xor pin.mode[pinout]);
    DEBUG_print("Set GPIO(" + pinout + ") to " + String(state, DEC) + "\n");
} }

void setTimer(String s){
  short sep=s.indexOf(',');
  ulong  value((sep+1) ?getVar(s.substring(sep+1)).toInt() :0L); if(value) value+=millis();
  s=getVar(s.substring(0, sep));
  if(s.length() && (!timer[s] || !value)){
    timer[s]=value;
} }

bool isTimer(String name){
  name=getVar(name.substring(0, name.indexOf(',')));
  if(name.length() && timer.find(name)==timer.end())
    setTimer(name);
  return( (name.length() && timer[name]) ?isNow(timer[name]) :false );
}

void doAction(String s){
  bool value;
  s=s.substring(s.indexOf(':')+1);
  for(ushort i(0), j(1); i<s.length() && j; i=j){
    j=s.indexOf(' ',i);
    value=true; if(s[i]=='!') {value=false;i++;}
    switch(s[i++]){
      case '$': //Set a variable:
        setVar(s.substring(i, j));
        break;
      case 'I': //Incr a variable:
        incrVar(s.substring(i, j));
        break;
      case 'S': //Set pin mode:
        setPinMode(s.substring(i, j));
        break;
      case '~': //Set GPIO state:
        setPin(getVar(s.substring(i, j)), value);
        break;
      case 'T': //Set Timer:
        setTimer(s.substring(i, j));
        break;
      case 'M': // Send MQTT msg:
        mqttSend(getVar(s.substring(i, j)));
        break;
      case 'H': //Set the hostname:
        setHostname(s.substring(i, j)); writeConfig();
        break;
      case 'N': //Set NTP:
        setNTP(s.substring(i, j)); writeConfig();
        break;
      case 'B': //Save config:
        writeConfig();
        break;
      case '?':
        {String sc=s.substring(i, j=-1); treatment(sc);}
        break;
    }j++;
} }

bool ifCondition(String s){
  ushort j=s.indexOf(':'); s=s.substring(0, j++);
  for(ushort i(0), reverse; i<s.length() && j; i=j){
    j=s.indexOf(' ', i);
    reverse=1; if(s[i]=='!') {reverse=0;i++;}
    switch(s[i++]){
      case '~': //Test GPIO
        if( reverse xor digitalRead(getVar(s.substring(i, j)).toInt()) )
          return false;
        break;
      case 'T': //test Timer
        if( reverse xor isTimer(s.substring(i, j)) )
          return false;
        break;
    }j++;
  }return true;
}

void treatment(String& s){
  for(ushort i(0), j(1); i<s.length() && j; i=j){
    j=s.indexOf(';', i);
    if(ifCondition(s.substring(i, j))) doAction(s.substring(i, j));
    j++;
} }
