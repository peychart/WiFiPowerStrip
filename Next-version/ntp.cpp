/* ESP8266-NTP-Manager C++ (Version 0.1 - 2020/07)
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
#include "ntp.h"

namespace _NTP
{
  ntp::ntp() : _changed(false) {
    json();
    operator[](G(ROUTE_NTP_DISABLED)) = false;
    operator[](G(ROUTE_NTP_SOURCE))   = "";
    operator[](G(ROUTE_NTP_ZONE))     = 0;
    operator[](G(ROUTE_NTP_DAYLIGHT)) = false;
    operator[](G(ROUTE_NTP_INTERVAL)) = 3600UL;
  }

  void ntp::begin(){
    if( !disabled() ){
      NTP.begin( source().c_str(), zone(), dayLight() );
      NTP.setInterval( interval() );
#ifdef DEBUG
      NTP.onNTPSyncEvent([]( NTPSyncEvent_t error ) {
        if (error) {
          DEBUG_print(F("Time Sync error: "));
          if (error == noResponse){
            DEBUG_print(F("NTP server not reachable\n"));
          }else if (error == invalidAddress){
            DEBUG_print(F("Invalid NTP server address\n"));
          }else{
            DEBUG_print(error); DEBUG_print(F("\n"));
        } }
        else {
          DEBUG_print(F("Got NTP time: "));
          DEBUG_print(NTP.getTimeDateString(NTP.getLastNTPSync()));
          DEBUG_print(F("\n"));
      } });
#endif
    } }

  ntp& ntp::set( untyped v ) {
    bool modified(false);
    for(auto &x :v.map()) if(_isInNTP( x.first ) ){
      modified|=( at( x.first ) != x.second );
      this->operator+=( x );
    }return changed( modified );
  }

  bool ntp::saveToSD(){
    if( !_changed ) return true;
    if( LittleFS.begin() ) {
      File file( LittleFS.open(F("/ntp.cfg"), "w" ));
      if( file ) {
            _changed = !file.println( this->serializeJson().c_str() );
            file.close();
            DEBUG_print(F("ntp.cfg writed.\n"));
      }else{DEBUG_print(F("Cannot write ntp.cfg !...\n"));}
      LittleFS.end();
    }else{DEBUG_print(F("Cannot open SD!...\n"));}
    return !_changed;
  }

  bool ntp::restoreFromSD(){
    if( LittleFS.begin() ) {
      File file( LittleFS.open(F("/ntp.cfg"), "r" ));
      if( file ) {
            _changed = this->deserializeJson( file.readStringUntil('\n').c_str() ).empty();
            file.close();
            DEBUG_print(F("ntp.cfg restored.\n"));
      }else{DEBUG_print(F("Cannot read ntp.cfg !...\n"));}
      LittleFS.end();
    }else{DEBUG_print(F("Cannot open SD!...\n"));}
    return !_changed;
  }
}
