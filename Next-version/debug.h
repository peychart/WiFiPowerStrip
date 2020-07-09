
#ifndef HEADER_A43236928626356
#define HEADER_A43236928626356

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define DEBUG

#define Serial_print(m)          {if(Serial) Serial.print (m);}
#define Serial_printf(m,n)       {if(Serial) Serial.printf(m,n);}
#ifdef DEBUG
  extern WiFiServer               telnetServer;
  extern WiFiClient               telnetClient;
  #ifdef DEFAULTWIFIPASS
    #define DEBUG_print(m)       {if(telnetClient && telnetClient.connected()) telnetClient.print(m);    Serial_print(m);}
    #define DEBUG_printf(m,n)    {if(telnetClient && telnetClient.connected()) telnetClient.printf(m,n); Serial_printf(m,n);}
  #else
    #define DEBUG_print(m)        Serial_print(m);
    #define DEBUG_printf(m,n)     Serial_printf(m,n);
  #endif
#else
  #define DEBUG_print(m)          ;
  #define DEBUG_printf(m,n)       ;
#endif

#endif
