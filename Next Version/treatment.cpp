#include "common.h"

//String defaultScript("S2,0,1;G2?Toff,2500;Toff?!G2 Toff Ton,1000;Ton?G2 Ton;");  //Blink...

//To remove:
String defaultScript("\
HLumibloc;\
S5,1,Bar,15000;\
S4,1,Couloir;\
S0,1,Salon;\
S2,1,Chambre2;\
S16,1,Chambre1;\
S15,1,Bureau;\
S14,3;\
S12,3;\
S13,3;\
|TnotHold?THold TnotHold;\
|!Thold !~13 !~12 ~14?T0,100;\
|!T0 !~14?T0;\
|!T0 ~12?T0;\
|!T0 ~13?T0;\
|T0 ~14 !~5?Thold,3000 T5,$~5;\
|T0 ~14 Thold?T5 T0 TnotHold TnotHold,1000;\
|T0 !~14?$5,1;\
|$5 !~5?~5 M{\"idx\":250,\"nvalue\":1,\"svalue\":\"1\"} T5,$~5 T0 $5;\
|$5 ~5?!~5 M{\"idx\":250,\"nvalue\":0,\"svalue\":\"0\"} Thold T5 T0 $5;\
|T5?!~5 M{\"idx\":250,\"nvalue\":0,\"svalue\":\"0\"} T5;\
|!Thold !~13 ~12 !~14?T1,100;\
|!T1 ~14?T1;\
|!T1 !~12?T1;\
|!T1 ~13?T1;\
|T1 ~12 !~4?Thold,3000 T4,$~4;\
|T1 ~12 Thold?T4 T1 TnotHold TnotHold,1000;\
|T1 !~12?$4,1;\
|$4 !~4?~4 M{\"idx\":251,\"nvalue\":1,\"svalue\":\"1\"} T4,$~4 T1 $4;\
|$4 ~4?!~4 M{\"idx\":251,\"nvalue\":0,\"svalue\":\"0\"} Thold T4 T1 $4;\
|T4?!~4 M{\"idx\":251,\"nvalue\":0,\"svalue\":\"0\"} T4;\
|!Thold !~13 ~12 ~14?T2,100;\
|!T2 !~14?T2;\
|!T2 !~12?T2;\
|!T2 ~13?T2;\
|T2 ~14 !~0?Thold,3000 T0,$~0;\
|T2 ~14 Thold?T0 T2 TnotHold TnotHold,1000;\
|T2 !~14?$0,1;\
|$0 !~0?~0 M{\"idx\":252,\"nvalue\":1,\"svalue\":\"1\"} T0,$~0 T2 $0;\
|$0 ~0?!~0 M{\"idx\":252,\"nvalue\":0,\"svalue\":\"0\"} Thold T0 T2 $0;\
|T0?!~0 M{\"idx\":252,\"nvalue\":0,\"svalue\":\"0\"} T0;\
|!Thold ~13 !~12 !~14?T3,100;\
|!T3 ~14?T3;\
|!T3 ~12?T3;\
|!T3 !~13?T3;\
|T3 !~13 ~2?Thold,3000 T2,$~2;\
|T3 !~13 Thold?T2 T3 TnotHold TnotHold,1000;\
|T3 !~13?$2,1;\
|$2 !~2?~2 M{\"idx\":253,\"nvalue\":1,\"svalue\":\"1\"} T2,$~2 T3 $2;\
|$2 ~2?!~2 M{\"idx\":253,\"nvalue\":0,\"svalue\":\"0\"} Thold T2 T3 $2;\
|T2?!~2 M{\"idx\":253,\"nvalue\":0,\"svalue\":\"0\"} T2;\
|!Thold ~13 !~12 ~14?T4,100;\
|!T4 !~14?T4;\
|!T4 ~12?T4;\
|!T4 !~13?T4;\
|T4 !~14 ~16?Thold,3000 T16,$~16;\
|T4 !~14 Thold?T16 T4 TnotHold TnotHold,1000;\
|T4 !~14?$16,1;\
|$16 !~16?~16 M{\"idx\":254,\"nvalue\":1,\"svalue\":\"1\"} T16,$~16 T4 $16;\
|$16 ~16?!~16 M{\"idx\":254,\"nvalue\":0,\"svalue\":\"0\"} Thold T16 T4 $16;\
|T16?!~16 M{\"idx\":254,\"nvalue\":0,\"svalue\":\"0\"} T16;\
|!Thold ~13 ~12 !~14?T5,100;\
|!T5 ~14?T5;\
|!T5 !~12?T5;\
|!T5 !~13?T5;\
|T5 !~12 ~15?Thold,3000 T15,$~15;\
|T5 !~12 Thold?T15 T5 TnotHold TnotHold,1000;\
|T5 !~12?$15,1;\
|$15 !~15?~15 M{\"idx\":255,\"nvalue\":1,\"svalue\":\"1\"} T15,$~15 T5 $15;\
|$15 ~15?!~15 M{\"idx\":255,\"nvalue\":0,\"svalue\":\"0\"} Thold T15 T5 $15;\
|T15?!~15 M{\"idx\":255,\"nvalue\":0,\"svalue\":\"0\"} T15;\
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

#define IF    '|'
#define THEN  '?'
#define ELSE  '^'
#define FI    ';'


void setAllPinsOnSlave(){
  for(ulong i(0); i<pin.gpio.size(); i++)
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

ulong getOp(String& s, ulong i, ulong& sep, ulong& last){
  for(sep=last=-1UL; !(last+1UL) && i<s.length(); i++) switch(s[i]){
    case  ' ':
    case   IF:
    case THEN:
    case ELSE:
    case   FI: last=i;
      break;
    case ',': if(!(sep+1UL))sep=i;
  }return ((last+1UL) ?last : s.length()+1UL);
}

String getVar(String& s, ulong& p){
  String z; bool isVar=(s[p]=='$');
  ulong first(isVar?++p:p), last, sep; p=getOp(s, first, sep, last);
  z=s.substring(first, (sep+1UL)?sep:last);
  return (isVar ?(var.find(z)==var.end() ?"" :var[z]) :z);
}String getVar(String s){ulong v(0); return(getVar(s, v));}

void setVar(String& s, ulong& p){
  ulong first((s[p]=='$')?++p:p), last, sep; p=getOp(s, first, sep, last);
  String name(s.substring(first, (sep+1UL)?sep:last));
  if(sep+1UL){
    if(var.find(name)==var.end())
      var[name]=getVar(s.substring(sep+1UL, last));
  }else var.erase(name);
}void setVar(String s){ulong v(0); setVar(s, v);}

bool isNumber(String& s){
  if(!isDigit(s[0]) && s[0]!='-')           return false;
  for(ulong i(0); i<s.length(); i++){bool b(false);
    if(!b && (b=(s[i]=='.' || s[i]==',')))  continue;
    if(!isDigit(s[i]))                      return false;
  }if(!isDigit(s[s.length()-1]))            return false;
  s.replace(',','.');
  return true;
}

void incrVar(String& s, ulong& p){
  ulong first((s[p]=='$')?++p:p), last, sep; p=getOp(s, first, sep, last);
  if(sep+1UL){String s1(s.substring(first, sep)), s2(getVar(s.substring(++sep, last)));
    if(isNumber(s1) && isNumber(s2)){
      if((s1+s2).indexOf('.')+1)
            var[s1]=String(var[s1].toFloat()+s2.toFloat());
      else  var[s1]=String(var[s1].toInt()+s2.toInt());
    }else var[s1]+=s2;
  }else{String s1(s.substring(first, last));
    if(isNumber(s1)) incrVar(s1+=",1", p);
  }
}void incrVar(String s){ulong v(0); incrVar(s, v);}

void setNTP(String& s, ulong& i){
  struct ntpConf oldNtp(ntp);
  ulong first(i), last, sep; i=getOp(s, i, sep, last);
  if(s.substring(first, (sep+1UL)?sep:last).length()){
    ntp.source=getVar(s.substring(first, (sep+1UL)?sep:last));
    if(sep+1UL){i=getOp(s, (first=++sep), sep, last);
      ntp.zone=getVar(s.substring(first, (sep+1UL)?sep:last)).toInt();
      if(++sep)
        ntp.dayLight=getVar(s.substring(sep, last)).toInt();
  } }
  if(ntp.source!=oldNtp.source || ntp.zone!=oldNtp.zone || ntp.dayLight!=oldNtp.dayLight )
    writeConfig();
}

void setHostname(String& s, ulong& p){
  ulong first(p), last, sep; p=getOp(s, first, sep, last);
  String newName(getVar(s.substring(first, (sep+1UL)?sep:last)));
  if(hostname!=newName){hostname=newName; writeConfig();}
}void setHostname(String s){ulong v(0); setHostname(s, v);}

void setPinMode(String& s, ulong& p){  //Format: pinNumber,mode[,G'pinNumber'_initial_value]
  ulong first(p), last, sep; p=getOp(s, first, sep, last);
  String g(s.substring(first, (sep+1UL)?sep:last));
  if(!(sep+1UL)){
DEBUG_print("SETPIN="+g+"\n");
    pin.mode.erase(g);
    return;
  }if(pin.mode.find(g)==pin.mode.end()){
    bool alreadyExist=false; for(auto& x : pin.gpio) if(x==g) alreadyExist=true;
    if(!alreadyExist) pin.gpio.push_back(g);
    p=getOp(s, first=++sep, sep, last);
    switch(pin.mode[g]=s.substring(first, (sep+1UL)?sep:last).toInt()){
      case  0: //reverse_output=false
      case  1: //reverse_output=true
        pinMode(g.toInt(), OUTPUT); pin.state[g]=true; setPin(g, false);
        break;
      case  2: pinMode(g.toInt(), INPUT);
        break;
      default: pinMode(g.toInt(), INPUT_PULLUP);
    }if(sep+1UL){p=getOp(s, first=++sep, sep, last);
      pin.name[g]=getVar(s.substring(first, (sep+1UL)?sep:last));
    }if(sep+1UL){
      pin.gpioVar[g]=getVar(s.substring(++sep, last));
//DEBUG_print("GPIO["+g+"]: mode="+pin.mode[g]+", name="+pin.name[g]+", val="+pin.gpioVar[g]+"\n");
      setVar(g="~"+g+","+pin.gpioVar[g]);
} } }

void setPin(String pinout, bool state){
  if(pin.mode.find(pinout)!=pin.mode.end() && pin.mode[pinout]<2 && pin.state[pinout]!=state){
    digitalWrite(pinout.toInt(), (pin.state[pinout]=state) xor pin.mode[pinout]);
    DEBUG_print("Set GPIO(" + pinout + ") to " + String(state, DEC) + "\n");
} }

void setTimer(String& s, ulong& p){
  ulong first(p), last, sep; p=getOp(s, first, sep, last);
  String name(getVar(s.substring(first, (sep+1UL)?sep:last)));
  ulong value(++sep ?getVar(s.substring(sep, last)).toInt() :0L); if(value) value+=millis();
  if(name.length() && (!timer[name] xor !value)) timer[name]=value;
}void setTimer(String s){ulong v(0); setTimer(s, v);}

bool isTimer(String& s, ulong& p){
  ulong first(p), last, sep; p=getOp(s, first, sep, last);
  String name(getVar(s.substring(first, (sep+1UL)?sep:last)));
  if(name.length() && timer.find(name)==timer.end())
    setTimer(name);
  return( (name.length() && timer[name]) ?isNow(timer[name]) :false );
}

void mqttString(String& s, ulong& i){ ulong first(i);
  for(short n(0); i<s.length();) switch(s[i++]){
    case '{': n++;      break;
    case '}': if(--n>0) break;
      mqttSend(s.substring(first, i));
      return;
} }

bool condition(String& s, ulong& i){bool isNot;
  while(i<s.length()){
    isNot=true; if(s[i]=='!') {isNot=false;i++;}
    switch(s[i++]){
      case '~':{ //GPIO state?
        String p(getVar(s, i));
        if( isNot xor digitalRead(p.toInt()) xor (pin.mode[p]%2) )
          return false;
        }break;
      case 'T': //Timer reached?
        if(isTimer(s, i) xor isNot)
          return false;
        break;
      case '$': //Var ?= true/false
        if(!getVar(s, --i).toInt())
          return false;
        break;
      case THEN:
      case FI:
        return true;
      case ELSE:
        return false;
  } }
  return true;
}

ulong indexOfFI(String& s, ulong i=0, bool fi=true){
  for(ulong n(0); i<s.length(); i++) switch(s[i]){
    case IF :
      n++;                      break;
    case ELSE:
      if(!fi && !n) return ++i; break;
    case FI:
      if(!n--) return i;
  }return -1;
}ulong indexOfELSE(String& s, ulong i=0){return indexOfFI(s, i, false);}

void treatment(String& s){bool isNot;
  for(ulong i(0); i<s.length();){
    isNot=false; if(s[i]=='!') {isNot=true;i++;}
    switch(s[i++]){
      case '$': //Set a variable:
        setVar(s, i);
        break;
      case 'I': //Incr a variable:
        incrVar(s, i);
        break;
      case 'S': //Set pin mode:
        setPinMode(s, i);
        break;
      case '~': //Set GPIO state:
        setPin(getVar(s, i), !isNot);
        break;
      case 'T': //Set Timer:
        setTimer(s, i);
        break;
      case 'M': // Send MQTT msg:
        mqttString(s, i);
        break;
      case 'H': //Set the hostname:
        setHostname(s, i);
        break;
      case 'N': //Set NTP:
        setNTP(s, i);
        break;
      case IF:
        if(!condition(s, i)) i=indexOfELSE(s, i);
        break;
      case ELSE:
        i=indexOfFI(s, i);
        break;
      case 'B': //Save config:
        writeConfig();
        break;
} } }
