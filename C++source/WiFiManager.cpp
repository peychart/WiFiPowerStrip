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
                                _on_connect(0),      _on_apConnect(0),      _on_staConnect(0),
                                _on_disconnect(0),   _on_apDisconnect(0),   _on_staDisconnect(0),
                                _if_connected(0),    _if_apConnected(0),    _if_staConnected(0),
                                _changed(false),     _next_connect(0UL),    _trial_counter(_trialNbr),
                                _apTimeout_counter(0) {
    json();
    operator[](G(ROUTE_VERSION))   = G("0.0.0");
    operator[](G(ROUTE_HOSTNAME))  = G("ESP8266");
    operator[](G(ROUTE_PIN_VALUE)) = 30000UL;
    operator[](G(ROUTE_WIFI_SSID)) = untyped();
    operator[](G(ROUTE_WIFI_PWD))  = untyped();
    disconnect(0L);
  }

  WiFiManager& WiFiManager::hostname(std::string s) {
    if( !s.empty() ) {
      _changed|=(hostname()!=s);
      at(G(ROUTE_HOSTNAME))=( s.substr(0,NAME_MAX_LEN-1) );
    }if(hostname().empty()) hostname( G("ESP8266") );
    if(enabled()) disconnect(1L); // reconnect now...
    return *this;
  }

  WiFiManager& WiFiManager::push_back(std::string s, std::string p) {
    size_t i( indexOf(s) );
    if( i != size_t(-1) ){
      _changed|=(password(i)!=p);
      if( p.size() )
        password( i, p );
      else{
        at(G(ROUTE_WIFI_PWD) ).vector().erase( at(G(ROUTE_WIFI_PWD) ).vector().begin()+i );
        at(G(ROUTE_WIFI_SSID)).vector().erase( at(G(ROUTE_WIFI_SSID)).vector().begin()+i );
        DEBUG_print(F("ssid: \"")); DEBUG_print(s.c_str()); DEBUG_print(F("\" removed...\n"));
      }return *this;
    }_changed=true;
    at(G(ROUTE_WIFI_PWD) )[ssidCount()] = p;
    at(G(ROUTE_WIFI_SSID))[ssidCount()] = s;
    DEBUG_print(F("ssid: \"")); DEBUG_print(ssid(ssidCount()-1).c_str());DEBUG_print(F("\" added...\n"));
    if(enabled() && ssidCount()==1) disconnect(1L); // reconnect now...
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
    at(G(ROUTE_WIFI_SSID))=s;
    at(G(ROUTE_WIFI_PWD) )=p;
    if(enabled()) disconnect(1L); // reconnect now...
    return *this;
  }

  bool WiFiManager::_apConnect(){ // Connect AP mode:
    if( _apMode_enabled ) {
      if( _apMode_enabled!=-1UL ) _apMode_enabled--;
      DEBUG_print(F("\nNo custom SSID found: setting soft-AP configuration ... \n"));
      _apTimeout_counter=_apTimeout; _trial_counter=_trialNbr;
      WiFi.forceSleepWake(); delay(1L); WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(AP_IPADDR, AP_GATEWAY, AP_SUBNET);
      if( (_ap_connected=WiFi.softAP((hostname()+"-").c_str()+String(ESP.getChipId()), DEFAULTWIFIPASS)) ){
        DEBUG_print((G("Connecting \"") + hostname()+ G("\" [")).c_str() + WiFi.softAPIP().toString() + (G("] from: ") + hostname()).c_str() + G("-") + String(ESP.getChipId()) + G("/" DEFAULTWIFIPASS "\n\n"));

        if(_on_apConnect) (*_on_apConnect)();
        if(_on_connect)   (*_on_connect)();
        return true;
      }DEBUG_print(F("WiFi Timeout.\n\n"));
    }return false;
  }

  bool WiFiManager::connect() {
    if ( !(_enabled=std::string(G(DEFAULTWIFIPASS)).size()) )
      return false;

    disconnect(); WiFi.forceSleepWake(); delay(1L);
    DEBUG_print(F("\n"));

    for(size_t i(0); i<ssidCount(); i++) {
      //Connection au reseau Wifi /Connect to WiFi network
      WiFi.mode(WIFI_STA); WiFi.begin( ssid(i).c_str(), password(i).c_str() );
      DEBUG_print((G("Connecting \"") + hostname()+ G("\" [")).c_str() + String(WiFi.macAddress()) + G("] to: ") + ssid(i).c_str());

      //Attendre la connexion /Wait for connection
      for(size_t j(0); j<12 && !staConnected(); j++) {
        delay(500L);
        DEBUG_print(F("."));
      } DEBUG_print(F("\n"));
      if(staConnected()) { // Now connected:
        _trial_counter=_trialNbr;
        MDNS.begin(hostname().c_str());
        MDNS.addService(F("http"), F("tcp"), 80);
        //Affichage de l'adresse IP /print IP address:
        DEBUG_print(F("WiFi connected\nIP address: ")); DEBUG_print(WiFi.localIP()); DEBUG_print(F(", dns: ")); DEBUG_print(WiFi.dnsIP()); DEBUG_print(F("\n\n"));

        if(_on_staConnect) (*_on_staConnect)();
        if(_on_connect)    (*_on_connect)();
        return true;
      } WiFi.disconnect();
    }

    if( ssidCount() && _trial_counter-- ) {
      DEBUG_print(F("WiFi Timeout (")); DEBUG_print( _trial_counter ); DEBUG_print(F(" more attempt")); DEBUG_print((_trial_counter>1 ?"s" :"")); DEBUG_print(F(").\n\n"));
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
      DEBUG_print(F("Wifi AP disconnected!...\n"));
      if(_on_apDisconnect) (*_on_apDisconnect)();

    }else if( WiFiManager::staConnected() ) {
      previouslyConnected=true;
      MDNS.end();
      WiFi.disconnect();
      DEBUG_print(F("Wifi STA disconnected!...\n"));
      if(_on_staDisconnect) (*_on_staDisconnect)();
    }

    // Sleep mode:
    if(duration >= reconnectionTime()){
      WiFi.mode(WIFI_OFF); WiFi.forceSleepBegin(); delay(1L);
    }

    if(previouslyConnected && _on_disconnect) (*_on_disconnect)();
    return *this;
  }

  void WiFiManager::loop() {                              //Test connexion/Check WiFi every mn:
    if(enabled() && _isNow(_next_connect)) {
      _next_connect = millis() + reconnectionTime();

      if( !WiFiManager::connected() || (WiFiManager::apConnected() && ssidCount() && !_apTimeout_counter--) ){
        WiFiManager::connect();
      }else{  // Already connected:
        if( staConnected() )
          MDNS.update();

        if( apConnected()  && _if_apConnected ) (*_if_apConnected)();
        if( staConnected() && _if_staConnected) (*_if_staConnected)();
        if(_if_connected) (*_if_connected)();
  } } }

  WiFiManager& WiFiManager::set( untyped v ) {
    bool modified(false);
    for(auto &x :v.map())
      if( _isInWiFiManager( x.first ) ){
        if( x.first==ROUTE_WIFI_SSID ){
          for(size_t i(0); i<x.second.vector().size(); i++)
            push_back( x.second.vector()[i].c_str(), v.map()[ROUTE_WIFI_PWD].vector()[i].c_str() );
          modified=changed();
        }else if( x.first==ROUTE_WIFI_PWD ){;
        }else{
          modified|=( at( x.first ) != x.second );
          this->operator+=( x );
      } }
    return changed( modified );
  }

  bool WiFiManager::saveToSD() {
    if( !_changed ) return true;
    if( LittleFS.begin() ) {
      File file( LittleFS.open(F("/wifi.cfg"), "w" ));
      if( file ) {
            _changed = !file.println( this->serializeJson().c_str() );
            file.close();
            DEBUG_print(F("wifi.cfg writed.\n"));
      }else DEBUG_print(F("Cannot write wifi.cfg!...\n"));
      LittleFS.end();
    }else{DEBUG_print(F("Cannot open SD!...\n"));}
    return !_changed;
  }

  bool WiFiManager::restoreFromSD() {
    if( LittleFS.begin() ) {
      File file( LittleFS.open(F("/wifi.cfg"), "r" ));
      if( file ) {
            _changed = this->deserializeJson( file.readStringUntil('\n').c_str() ).empty();
            file.close();
            DEBUG_print(F("wifi.cfg restored.\n"));
      }else{DEBUG_print(F("Cannot read wifi.cfg!...\n"));}
      LittleFS.end();
    }else{DEBUG_print(F("Cannot open SD!...\n"));}
    return !_changed;
  }
  
}
