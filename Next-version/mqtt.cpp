/* ESP8266-MQTT-Manager C++ (Version 0.1 - 2020/07)
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
#include "mqtt.h"

namespace MQTT
{
  mqtt::mqtt( void ) : _changed(false) {
    setClient( _ethClient );
    operator[](ROUTE_MQTT_BROKER)   = "";
    operator[](ROUTE_MQTT_PORT)     = 1883;
    operator[](ROUTE_MQTT_IDENT)    = "";
    operator[](ROUTE_MQTT_USER)     = "";
    operator[](ROUTE_MQTT_PWD)      = "";
    operator[](ROUTE_MQTT_INTOPIC)  = "";
    operator[](ROUTE_MQTT_OUTOPIC)  = "";
    json();
  }

  bool mqtt::reconnect() {
    if( !disabled() && !this->connected() ){
      this->setServer( broker().c_str(), port() );
      if( (!_ethClient.connected() && !_ethClient.connect(broker().c_str(), port()))
        || !this->connect( ident().c_str(), user().c_str(), password().c_str() ) ){
        return false;
      }DEBUG_print( F("Connected to MQTT broker: \"") ); DEBUG_print( (broker() + "\".\n").c_str() );
      for(auto x :_inTopic) PubSubClient::subscribe( x.c_str() );
    }return true;
  }

  void mqtt::loop( void ) {
    static ulong last(0L);
    if( !disabled() ){
      ulong now(millis());
      PubSubClient::loop();
      if( now-last > 500 ){
        reconnect(); last=now;
  } } }

  bool mqtt::send( std::string s, std::string topic ) {
    if( !disabled() && s.length()){
      //ulong len(1); while(len<s.size()) len<<=1; this->setBufferSize( len );
      if( !this->beginPublish(topic.c_str(), s.size(), true ) ){
        DEBUG_print(F("Cannot write to MQTT broker: \"")); DEBUG_print(broker().c_str());
        DEBUG_print(F("\" on topic \"")); DEBUG_print(topic.c_str());
        DEBUG_print(F(".\n"));
        return false;
      }uint8_t c; for(ulong i(0); (i+=write(&(c=s[i]),1)) < s.size(); ); endPublish();
      DEBUG_print(F("\"")); DEBUG_print(s.c_str()); DEBUG_print(F("\" published to \"")); DEBUG_print(broker().c_str());
      DEBUG_print(F("\"on topic \"")); DEBUG_print(topic.c_str()); DEBUG_print(F("\".\n"));
    }return true;
  }

  mqtt& mqtt::set( untyped v ) {
    bool modified(false);
    for(auto &x :v.map()) if(_isInMqtt( x.first ) ){
      modified|=( at( x.first ) != x.second );
      this->operator+=( x );
    }return changed( modified );
  }

  bool mqtt::saveToSD(){
    if( !_changed ) return true;
    if( LittleFS.begin() ) {
      File file( LittleFS.open( F("/mqtt.cfg"), "w" ) );
      if( file ) {
            _changed = !file.println( this->serializeJson().c_str() );
            file.close();
            DEBUG_print( F("mqtt.cfg writed.\n") );
      }else{DEBUG_print( F("Cannot write mqtt.cfg !...\n") );}
      LittleFS.end();
    }else{DEBUG_print( F("Cannot open SD!...\n") );}
    return !_changed;
  }

  bool mqtt::restoreFromSD(){
    if( LittleFS.begin() ) {
      File file( LittleFS.open( F("/mqtt.cfg"), "r" ) );
      if( file ) {
            _changed = this->deserializeJson( file.readStringUntil('\n').c_str() ).empty();
            file.close();
            DEBUG_print( F("mqtt.cfg restored.\n") );
      }else{DEBUG_print( F("Cannot read mqtt.cfg !...\n") );}
      LittleFS.end();
    }else{DEBUG_print( F("Cannot open SD!...\n") );}
    return !_changed;
  }
}
