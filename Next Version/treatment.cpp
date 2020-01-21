#include "common.h"

//String defaultScript("S2,0,1;G2?Toff,2500;Toff?!G2 Toff Ton,1000;Ton?G2 Ton;");  //Blink...

//To remove:
String defaultScript("\
HLumibloc;\
S16,1,Bar,15000;\
S5,1,Couloir;\
S4,1,Salon;\
S0,1,Chambre2;\
S2,1,Chambre1;\
S15,1,Bureau;\
S14,3;\
S12,3;\
S13,3;\
TnotHold@THold TnotHold;\
!Thold !~13 !~12 ~14@T0,50;\
!T0 !~14@T0;\
!T0 ~12@T0;\
!T0 ~13@T0;\
T0 ~14 !~16@Thold,3000 T~16,$~16;\
T0 ~14 Thold@T~16 T0 TnotHold TnotHold,1000;\
T0 ~14@$G16,1;\
$G16 !~16@~16 M{\"idx\":150,\"nvalue\":1,\"svalue\":\"1\"} T~16,$~16 T0 $G16;\
$G16 ~16@!~16 M{\"idx\":150,\"nvalue\":0,\"svalue\":\"0\"} Thold T~16 T0 $G16;\
T~16@!~16 M{\"idx\":150,\"nvalue\":0,\"svalue\":\"0\"} T~16;\
!Thold !~13 ~12 ~14@T1,50;\
!T1 !~14@T1;\
!T1 !~12@T1;\
!T1 ~13@T1;\
T1 ~14 !~5@Thold,3000 T~5,$~5;\
T1 ~14 Thold@T~5 T1 TnotHold TnotHold,1000;\
T0 ~14@$G5,1;\
$G5 !~5@~5 M{\"idx\":151,\"nvalue\":1,\"svalue\":\"1\"} T~5,$~5 T1 $G5;\
$G5 ~5@!~5 M{\"idx\":151,\"nvalue\":0,\"svalue\":\"0\"} Thold T~5 T1 $G5;\
T~5@!~5 M{\"idx\":151,\"nvalue\":0,\"svalue\":\"0\"} T~5;\
!Thold ~13 !~12 !~14@T2,50;\
!T2 !~14@T2;\
!T2 !~12@T2;\
!T2 ~13@T2;\
T2 ~13 !~4@Thold,3000 T~4,$~4;\
T2 ~13 Thold@T~4 T2 TnotHold TnotHold,1000;\
T0 ~13@$G4,1;\
$G4 !~4@~4 M{\"idx\":152,\"nvalue\":1,\"svalue\":\"1\"} T~4,$~4 T2 $G4;\
$G4 ~4@!~4 M{\"idx\":152,\"nvalue\":0,\"svalue\":\"0\"} Thold T~4 T2 $G4;\
T~4@!~4 M{\"idx\":152,\"nvalue\":0,\"svalue\":\"0\"} T~4;\
!Thold ~13 !~12 ~14@T3,50;\
!T3 !~14@T3;\
!T3 ~12@T3;\
!T3 !~13@T3;\
T3 !~13 ~0@Thold,3000 T~0,$~0;\
T3 !~13 Thold@T~0 T3 TnotHold TnotHold,1000;\
T0 ~13@$G0,1;\
$G0 !~0@~0 M{\"idx\":153,\"nvalue\":1,\"svalue\":\"1\"} T~0,$~0 T3 $G0;\
$G0 ~0@!~0 M{\"idx\":153,\"nvalue\":0,\"svalue\":\"0\"} Thold T~0 T3 $G0;\
T~0@!~0 M{\"idx\":153,\"nvalue\":0,\"svalue\":\"0\"} T~0;\
!Thold ~13 ~12 !~14@T4,50;\
!T4 ~14@T4;\
!T4 !~12@T4;\
!T4 !~13@T4;\
T4 !~13 ~2@Thold,3000 T~2,$~2;\
T4 !~13 Thold@T~2 T4 TnotHold TnotHold,1000;\
T0 ~13@$G2,1;\
$G2 !~2@~2 M{\"idx\":154,\"nvalue\":1,\"svalue\":\"1\"} T~2,$~2 T4 $G2;\
$G2 ~2@!~2 M{\"idx\":154,\"nvalue\":0,\"svalue\":\"0\"} Thold T~2 T4 $G2;\
T~2@!~2 M{\"idx\":154,\"nvalue\":0,\"svalue\":\"0\"} T~2;\
!Thold ~13 ~12 ~14@T5,50;\
!T5 !~14@T5;\
!T5 !~12@T5;\
!T5 !~13@T5;\
T5 !~13 ~15@Thold,3000 T~15,$~15;\
T5 !~13 Thold@T~15 T5 TnotHold TnotHold,1000;\
T0 ~13@$G15,1;\
$G15 !~15@~15 M{\"idx\":155,\"nvalue\":1,\"svalue\":\"1\"} T~15,$~15 T5 $G15;\
$G15 ~15@!~15 M{\"idx\":155,\"nvalue\":0,\"svalue\":\"0\"} Thold T~15 T5 $G15;\
T~15@!~15 M{\"idx\":155,\"nvalue\":0,\"svalue\":\"0\"} T~15;\
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
    if(sep+1 && var.find(s.substring(0,sep))==var.end()){
          var[s.substring(0,sep)]=s.substring(sep+1);
    }else var.erase(s.substring(0,sep));
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
        pinMode(p.toInt(), OUTPUT); pin.state[p]=true; setPin(p, false);
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

void setPin(String pinout, bool state){
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
  s=s.substring(s.indexOf('@')+1);
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
  ushort j=s.indexOf('@'); s=s.substring(0, j++);
  for(ushort i(0), isNot; i<s.length() && j; i=j){
    j=s.indexOf(' ', i);
    isNot=1; if(s[i]=='!') {isNot=0;i++;}
    switch(s[i++]){
      case '~': //Test GPIO
        if( isNot xor digitalRead( getVar(s.substring(i, j)).toInt() ) xor (pin.mode[getVar(s.substring(i, j))]%2) )
          return false;
        break;
      case 'T': //test Timer
        if( isNot xor isTimer(s.substring(i, j)) )
          return false;
        break;
      case '$': //Var ?= true/false
        if(!getVar(s.substring(i-1, j)).toInt())
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
