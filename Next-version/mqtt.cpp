/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/untyped-cpp>

    Copyright (C) 2017  -  peychart

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
    operator[](_BROKER_)   = "";
    operator[](_PORT_)     = 1889;
    operator[](_IDENT_)    = "";
    operator[](_USER_)     = "";
    operator[](_PWD_)      = "";
    operator[](_INTOPIC_)  = "";
    operator[](_OUTTOPIC_) = "";
    at(_PORT_)=1883; setClient(client); setCallback( callback );
    restoreFromSD();
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
     DEBUG_print( ("'" + msg + "' published to \"" + broker().c_str() + "\".\n").c_str() );
     return true;
  }

  void mqtt::callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);

    Serial.print("Message:");
    for (unsigned int i(0); i<length; i++) {
      Serial.print((char)payload[i]);
  } }

  bool mqtt::saveToSD(){
    bool ret(false);
    if( !_changed ) return true;
    if( LittleFS.begin() ) {
      std::ostringstream buff;
      File file( LittleFS.open( "mqtt.cfg", "w" ) );
      if( file ) {
            this->serializeJson( buff );
            ret = file.println( buff.str().c_str() );
            file.close();
            DEBUG_print("mqtt.cfg writed.\n");
      }else{DEBUG_print("Cannot write mqtt.cfg !...\n");}
      LittleFS.end();
    }else{
      DEBUG_print("Cannot open SD!...\n");
    }return ret;
  }

  bool mqtt::restoreFromSD(){
    bool ret(false);
    if( LittleFS.begin() ) {
      String buff;
      File file( LittleFS.open( "mqtt.cfg", "r" ) );
      if( file && (buff=file.readStringUntil('\n')).length()
               && (ret=!this->deserializeJson( buff.c_str() ).empty()) ) {
            file.close();
            DEBUG_print("mqtt.cfg restored.\n");
      }else{DEBUG_print("Cannot read mqtt.cfg !...\n");}
      LittleFS.end();
    }else{
      DEBUG_print("Cannot open SD!...\n");
    }return ret;
  }

}


