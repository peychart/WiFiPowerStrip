/* ESP8266-WiFi-Manager C++ (Version 0.1 - 2020/07)
    <https://github.com/peychart/WiFiPowerStrip>

    Copyright (C) 2020  -  peychart

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this program.
    If not, see <http://www.gnu.org/licenses/>.

    Details of this licence are available online under:
                        http://www.gnu.org/licenses/gpl-3.0.html
*/
#include "WiFiManager.h"

namespace WiFiManagement {

  WiFiManager::WiFiManager() :  _enabled(false),     _ap_connected(false),  _apMode_enabled(-1),
                                _trialNbr(3),        _apTimeout(10),        _on_connect(0),
                                _on_apConnect(0),    _on_staConnect(0),     _on_disconnect(0),
                                _on_apDisconnect(0), _on_staDisconnect(0),  _if_connected(0),
                                _if_apConnected(0),  _if_staConnected(0),   _on_memoryLeak(0),
                                _changed(false),     _next_connect(0UL),
                                _trial_counter(_trialNbr),   _apTimeout_counter(0) {
    json();
    operator[](ROUTE_VERSION)       = "0.0.0";
    operator[](ROUTE_HOSTNAME) = "ESP8266";
    operator[](ROUTE_PIN_VALUE)  = 30000UL;
    operator[](ROUTE_SSID)     = untyped();
    operator[](ROUTE_PWD)      = untyped();
    disconnect(0L);
  }

  WiFiManager& WiFiManager::hostname(std::string s) {
    if( !s.empty() ) {
      _changed|=(hostname()!=s);
      at(ROUTE_HOSTNAME)=( s.substr(0,NAME_MAX_LEN-1) );
    }if(hostname().empty()) hostname("ESP8266");
    if(enabled()) disconnect(1L); // reconnect now...
    return *this;
  }

  WiFiManager& WiFiManager::push_back(std::string s, std::string p) {
  if (ssidCount()+1 < ssidMaxCount()) {
      size_t i=indexOf(s);
      if( i != -1UL ){
        _changed|=(password(i)!=p);
        password( i, p );
        return *this;
      }_changed=true;
      at(ROUTE_PWD ).operator[](ssidCount()) = p;
      at(ROUTE_SSID).operator[](ssidCount()) = s;
      DEBUG_print(String(("ssid: \"" + ssid(ssidCount()-1)).c_str()) + "\" added...\n");
    }if(enabled() && ssidCount()==1) disconnect(1L); // reconnect now...
    return *this;
  }

  WiFiManager& WiFiManager::erase(size_t n) {
    untyped s, p;
    for(size_t i(0), j(0); i<ssidCount(); i++) {
      if(j!=n) {
        _changed=true;
        s[j]=ssid(i);
        p[j]=password(i);
    } }
    at(ROUTE_SSID)=s;
    at(ROUTE_PWD )=p;
    if(enabled()) disconnect(1L); // reconnect now...
    return *this;
  }

  size_t WiFiManager::indexOf(std::string s) {
    size_t i(0); for(; i<ssidCount(); i++) {
      if( ssid(i)==s )
        return i;
    }return -1;
  }

  bool WiFiManager::_apConnect(){  // Connect AP mode:
    if( _apMode_enabled ) {
      if( _apMode_enabled!=-1UL ) _apMode_enabled--;
      DEBUG_print("\nNo custom SSID found: setting soft-AP configuration ... \n");
      _apTimeout_counter=_apTimeout; _trial_counter=_trialNbr;
      WiFi.forceSleepWake(); delay(1L); WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(AP_IPADDR, AP_GATEWAY, AP_SUBNET);
      if( (_ap_connected=WiFi.softAP((hostname()+"-").c_str()+String(ESP.getChipId()), DEFAULTWIFIPASS)) ){
        DEBUG_print( ("Connecting \"" + hostname()+ "\" [").c_str() + WiFi.softAPIP().toString() + ("] from: " + hostname()).c_str() + "-" + String(ESP.getChipId()) + "/" + DEFAULTWIFIPASS + "\n\n" );

#ifdef DEBUG
        telnetServer.begin();
        telnetServer.setNoDelay(true);
#endif
        if(_on_apConnect) (*_on_apConnect)();
        if(_on_connect)   (*_on_connect)();

        return true;
      }DEBUG_print("WiFi Timeout.\n\n");
    }return false;
  }

  bool WiFiManager::connect() {
    if ( !(_enabled=std::string(DEFAULTWIFIPASS).size()) )
      return false;

    disconnect(); WiFi.forceSleepWake(); delay(1L);
    DEBUG_print("\n");

    for(size_t i(0); i<ssidCount(); i++) {
      //Connection au reseau Wifi /Connect to WiFi network
      WiFi.mode(WIFI_STA); WiFi.begin( ssid(i).c_str(), password(i).c_str() );
      DEBUG_print(("Connecting \"" + hostname()+ "\" [").c_str() + String(WiFi.macAddress()) + "] to: " + ssid(i).c_str());

      //Attendre la connexion /Wait for connection
      for(size_t j(0); j<12 && !staConnected(); j++) {
        delay(500L);
        DEBUG_print(".");
      } DEBUG_print("\n");
      if(staConnected()) { // Now connected:
        _trial_counter=_trialNbr;
        MDNS.begin(hostname().c_str());
        MDNS.addService("http", "tcp", 80);
        #ifdef DEBUG
          telnetServer.begin();
          telnetServer.setNoDelay(true);
        #endif

        //Affichage de l'adresse IP /print IP address:
        DEBUG_print("WiFi connected\n");
        DEBUG_print("IP address: "); DEBUG_print(WiFi.localIP()); DEBUG_print(", dns: "); DEBUG_print(WiFi.dnsIP()); DEBUG_print("\n\n");

        // onConnect:
        if(_on_staConnect) (*_on_staConnect)();
        if(_on_connect)    (*_on_connect)();

        return true;
      } WiFi.disconnect();
    }

    if( ssidCount() && --_trial_counter ) {
      DEBUG_print("WiFi Timeout (" + String(_trial_counter, DEC) + " more attempt" + (_trial_counter>1 ?"s" :"") + ").\n\n");
      return false;
    }
    return WiFiManager::_apConnect();
  }

  WiFiManager& WiFiManager::disconnect(ulong duration) {
    bool previouslyConnected(false);
    _next_connect = millis() + ((_enabled=duration) ?duration :reconnectionTime());

    if ( WiFiManager::apConnected() ) {
      previouslyConnected=true;
      WiFi.softAPdisconnect(); _ap_connected=false;
      DEBUG_print("Wifi AP disconnected!...\n");
      if(_on_apDisconnect) (*_on_apDisconnect)();

    }else if( WiFiManager::staConnected() ) {
      previouslyConnected=true;
      MDNS.end();
      WiFi.disconnect();
      DEBUG_print("Wifi STA disconnected!...\n");
      if(_on_staDisconnect) (*_on_staDisconnect)();
    }

    // Sleep mode:
    if(duration >= reconnectionTime()){
      WiFi.mode(WIFI_OFF); WiFi.forceSleepBegin(); delay(1L);
    }

    if(previouslyConnected && _on_disconnect) (*_on_disconnect)();
    return *this;
  }

  void WiFiManager::_memoryTest(){
  #ifdef MEMORYLEAKS     //oberved on DNS server (bind9/NTP) errors -> reboot each ~30mn
    ulong f=ESP.getFreeHeap();
    if(f<MEMORYLEAKS && _on_memoryLeak) (*_on_memoryLeak)();
    DEBUG_print("FreeMem: " + String(f, DEC) + "\n");
  #endif
  }

  void WiFiManager::loop() {                              //Test connexion/Check WiFi every mn:
    if(enabled() && _isNow(_next_connect)) {
      WiFiManager::_memoryTest();
      _next_connect = millis() + reconnectionTime();

      if( !WiFiManager::connected() || (WiFiManager::apConnected() && ssidCount() && !_apTimeout_counter--) ){
        WiFiManager::connect();
      }else{  // Already connected:
        if( staConnected() )
          MDNS.update();

        #ifdef DEBUG
        if(telnetServer.hasClient()) {                  //Telnet client connection:
          if (!telnetClient || !telnetClient.connected()) {
            if(telnetClient){
              telnetClient.stop();
              DEBUG_print("Telnet Client Stop\n");
            }telnetClient=telnetServer.available();
            telnetClient.flush();
            DEBUG_print("New Telnet client connected...\n");
            DEBUG_print("ChipID(" + String(ESP.getChipId(), DEC) + ") says to "); DEBUG_print(telnetClient.remoteIP()); DEBUG_print(": Hello World, Telnet!\n\n");
        } }
        #endif

        if( apConnected()  && _if_apConnected ) (*_if_apConnected)();
        if( staConnected() && _if_staConnected) (*_if_staConnected)();
        if(_if_connected) (*_if_connected)();
  } } }

  WiFiManager& WiFiManager::set( untyped v ) {
    for(auto &x :v.map())
      if( _isInWiFiManager( x.first ) )
        this->operator+=( x );
    return *this;
  }

  bool WiFiManager::saveToSD() {
    bool ret(false);
    if( !_changed ) return true;
    if( LittleFS.begin() ) {
      File file( LittleFS.open("/wifi.cfg", "w") );
      if( file ) {
            if( (ret = file.println( this->serializeJson().c_str() )) )
              _changed=false;
            file.close();
            DEBUG_print("wifi.cfg writed.\n");
      }else DEBUG_print("Cannot write wifi.cfg!...\n");
      LittleFS.end();
    }else{DEBUG_print("Cannot open SD!...\n");}
    return ret;
  }

  bool WiFiManager::restoreFromSD() {
    bool ret(false);
    if( LittleFS.begin() ) {
      File file( LittleFS.open("/wifi.cfg", "r") );
      if( file ) {
            if( (ret = !this->deserializeJson( file.readStringUntil('\n').c_str() ).empty()) )
              _changed=false;
            file.close();
            DEBUG_print("wifi.cfg restored.\n");
      }else{DEBUG_print("Cannot read wifi.cfg!...\n");}
      LittleFS.end();
    }else{DEBUG_print("Cannot open SD!...\n");}
    return ret;
  }
}
