/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/mqtt-cpp>

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

  mqtt::mqtt(WiFiClient& client) : _changed(false) {
    json();
    operator[](_MQTT_BROKER_)   = "";
    operator[](_MQTT_PORT_)     = 1883;
    operator[](_MQTT_IDENT_)    = "";
    operator[](_MQTT_USER_)     = "";
    operator[](_MQTT_PWD_)      = "";
    operator[](_MQTT_INTOPIC_)  = "";
    operator[](_MQTT_OUTTOPIC_) = "";
    setClient(client);
  }

  void mqtt::reconnect() {
    setServer(broker().c_str(), port() );
    subscribe( inputTopic().c_str() );
    if( !broker().size() || !connect( ident().c_str(), user().c_str(), password().c_str() ) ) {
      DEBUG_print( ("Trying mqtt connection: Cannot connect to MQTT broker: \"" + broker() + "\"!\n").c_str() );
    }else{
      DEBUG_print( ("Connect to MQTT broker: \"" + broker() + "\"!\n").c_str() );
  } }

bool mqtt::send( std::string s, std::string msg ) {
    if(!s.length()){
      DEBUG_print( ("Nothing to published to \"" + broker() + "\"!\n").c_str() );
      return true;
    }if( !connected() ){
      DEBUG_print( "mqtt not connected : retry... \n" );
      reconnect();
    }if( !connected() )
      return false;
     publish(outputTopic().c_str(), s.c_str());
     DEBUG_print( ("'" + std::string(msg.empty() ?"Hidden" :msg.c_str()) + "' published to \"" + broker().c_str() + "\".\n").c_str() );
     return true;
  }

  bool mqtt::saveToSD(){
    bool ret(false);
    if( !_changed ) return true;
    if( LittleFS.begin() ) {
      File file( LittleFS.open( "/mqtt.cfg", "w" ) );
      if( file ) {
            if( (ret = file.println( this->serializeJson().c_str() )) )
              _changed=false;
            file.close();
            DEBUG_print("mqtt.cfg writed.\n");
      }else{DEBUG_print("Cannot write mqtt.cfg !...\n");}
      LittleFS.end();
    }else{DEBUG_print("Cannot open SD!...\n");}
    return ret;
  }

  bool mqtt::restoreFromSD(){
    bool ret(false);
    if( LittleFS.begin() ) {
      File file( LittleFS.open( "/mqtt.cfg", "r" ) );
      if( file ) {
            if( (ret = !this->deserializeJson( file.readStringUntil('\n').c_str() ).empty()) )
              _changed=false;
            file.close();
            DEBUG_print("mqtt.cfg restored.\n");
      }else{DEBUG_print("Cannot read mqtt.cfg !...\n");}
      LittleFS.end();
    }else{DEBUG_print("Cannot open SD!...\n");}
    return ret;
  }

}


