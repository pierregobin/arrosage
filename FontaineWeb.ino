/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "FS.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include "RTClib.h"


typedef struct  {
  int  h;  int mn; int s;
} s_time;


typedef struct {
  s_time start;
  int duration;
  int dir;
  int pwm;
} s_action;

const int actions_nb = 38;
s_action actions[38] = {
  {{6, 0, 0}, 50, 0, 800},
  {{6, 0, 10}, 30, 1, 1000},
  {{7, 0, 0}, 50, 0, 800},
  {{7, 0, 10}, 30, 1, 1000},
  {{8, 0, 0}, 50, 0, 800},
  {{8, 0, 10}, 30, 1, 1000},
  {{9, 0, 0}, 50, 0, 800},
  {{9, 0, 10}, 30, 1, 1000},
  {{10, 0, 0}, 100, 0, 800},
  {{10, 0, 10}, 60, 1, 1000},  
  {{12, 00, 00}, 100, 0, 800},
  {{12, 00, 10}, 45, 1, 1000},
  {{13, 00, 00}, 100, 0, 800},
  {{13, 00, 10}, 45, 1, 1000},
  {{14, 00, 00}, 100, 0, 800},
  {{14, 00, 10}, 45, 1, 1000},
  {{14, 30, 00}, 100, 0, 800},
  {{14, 30, 10}, 45, 1, 1000},
  {{15, 00, 00}, 100, 0, 800},
  {{15, 00, 10}, 45, 1, 1000},
  {{15, 30, 00}, 100, 0, 800},
  {{15, 30, 10}, 45, 1, 1000},
  {{16, 00, 00}, 100, 0, 800},
  {{16, 00, 10}, 45, 1, 1000},
  {{16, 30, 00}, 100, 0, 800},
  {{16, 30, 10}, 45, 1, 1000},
  {{17, 00, 00}, 100, 0, 800},
  {{17, 00, 10}, 45, 1, 1000},
  {{18, 00, 00}, 100, 0, 800},
  {{18, 00, 10}, 60, 1, 1000},
  {{19, 00, 00}, 100, 0, 800},
  {{19, 00, 10}, 60, 1, 1000},
  {{20, 00, 00}, 100, 0, 800},
  {{20, 00, 10}, 60, 1, 1000},
  {{20, 40, 00}, 300, 0, 800},
  {{20, 40, 10}, 270, 1, 1000},
  {{21, 00, 00}, 150, 0, 800},
  {{21, 00, 10}, 120, 1, 1000}
  };
RTC_PCF8523 rtc;
bool rtcFound = false;
bool rtcInitialized = false;
const char *ssid = SSID;
const char *password = PASSWD;
const int D1 = 5;
const int D2 = 4;

const int D5 = 14;
const int D6 = 12;
const int D7 = 13;
const int D8 = 15;

const int PWMA = D5;
const int BRAKEA = D6;
const int DIRA = D7;
int dir, pwm, brake = 0;
int Dir, Pwm = 0;

int applyCompute = 1;

ESP8266WebServer server ( 80 );

const int led = 13;


void readActions () {
  bool success_read = false;
  String s;
  int i = 0;
  SPIFFS.begin();
  File f = SPIFFS.open("/rules.txt", "r");
  if (!f) {
    Serial.println("fail to open rules.txt");
  } else {
    Serial.println("rules.txt open");
    Serial.println("rules.txt :");
    while (s = f.readStringUntil('\n')) {
      Serial.println("[" + s + "]");
    }
  }


  f.close();
  SPIFFS.end();
}

String displayActions() {
  int i;
  char temp[400];
  applyCompute = 1;
  String result = "";
  for (i = 0;  i < actions_nb; i++) {
    snprintf ( temp, 400,
               "<p>action[%2d] : [%02d:%02d:%02d] - [%02d] secondes : dir=[%1d], vitesse=%4d</p>",
               i, actions[i].start.h, actions[i].start.mn, actions[i].start.s, actions[i].duration, actions[i].dir, actions[i].pwm);
    result += temp;
  }
  return result;
}

void handleRoot() {
  digitalWrite ( led, 1 );
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf ( temp, 400,

             "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

             hr, min % 60, sec % 60
           );
  server.send ( 200, "text/html", temp );
  digitalWrite ( led, 0 );
}

void handleClock() {
  digitalWrite ( led, 1 );
  char temp[400];
  DateTime now = rtc.now();
  snprintf ( temp, 400,

             "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Clock</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Clock from RTC</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
  </body>\
</html>",

             now.hour(), now.minute(), now.second()
           );

  server.send ( 200, "text/html", temp );
  digitalWrite ( led, 0 );

}


void handleSetClock() {
  digitalWrite(led, 1);
  int year, month, day, hour, minute, second;
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    if (server.argName (i) == "y") {
      year = server.arg(i).toInt();
    }
    if (server.argName (i) == "month") {
      month = server.arg(i).toInt();
    }
    if (server.argName (i) == "d") {
      day = server.arg(i).toInt();
    }

    if (server.argName ( i ) == "h") {
      hour = server.arg( i ).toInt();
    }
    if (server.argName ( i ) == "m") {
      minute = server.arg( i ).toInt();
    }
    if (server.argName ( i ) == "s") {
      second = server.arg( i ).toInt();
    }
  }
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
  handleClock();
  delay(100);
  digitalWrite(led, 0);

}

void handleMotor() {
  char temp[400];
  applyCompute = 0;
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    if (server.argName ( i ) == "brake") {
      if (server.arg ( i ).toInt() != 0) {
        brake = 1;
      } else {
        brake = 0;
      }
    }
    if (server.argName ( i ) == "pwm") {
      pwm = server.arg ( i ).toInt();
    }
    if (server.argName ( i ) == "dir") {
      if (server.arg( i ) .toInt() != 0) {
        dir = 1;
      } else {
        dir = 0;
      }
    }
  }
    analogWrite(PWMA, pwm);
  digitalWrite(BRAKEA, brake);
  digitalWrite(DIRA,dir);
  snprintf ( temp, 400,
             "<html>\
  <head>\
    <title>Motor</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Motor </h1>\
    <p>brake : %01d</p>\
    <p>pwm : %4d</p>\
    <p>dir  : %01d</p>\
  </body>\
</html>",

             brake, pwm, dir
           );

  server.send ( 200, "text/html", temp );
}

void handleMotorStatus() {
    char temp[400];
  snprintf ( temp, 400,
             "<html>\
  <head>\
    <title>Motor Status</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Motor </h1>\
    <p>pwm : %4d</p>\
    <p>dir  : %01d</p>\
  </body>\
</html>",

             Pwm, Dir
           );

  server.send ( 200, "text/html", temp );    
}
void handleDisplayActions() {
  // String message = "<html>\<head>\<title>Motor</title>\<style>\body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\</style>\</head>\<body>\";
  String message;
  message = "<html><head><title>Actions</title><style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style></head><body>";
  message += displayActions();
  message += "</body></html>";
  server.send(200, "text/html", message);
}

void handleReadRules() {
  readActions();
  server.send(200, "text/html", "<html><head><title>read rules</title></head><body>done !</body></html>");
}
void handleNotFound() {
  digitalWrite ( led, 1 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
  digitalWrite ( led, 0 );
}

void setup ( void ) {
  pinMode ( led, OUTPUT );
  digitalWrite ( led, 0 );
  pinMode (D5, OUTPUT);
  pinMode (D6, OUTPUT);
  pinMode (D7, OUTPUT);
  Serial.begin ( 115200 );
  WiFi.mode ( WIFI_STA );
  WiFi.begin ( ssid, password );
  Serial.println ( "" );

  rtcFound = rtc.begin();
  rtcInitialized = rtc.initialized();


  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );

  if ( MDNS.begin ( "arrosoir" ) ) {
    Serial.println ( "MDNS responder started" );
  }

  server.on ( "/", handleRoot );

  server.on ( "/clock", handleClock);
  server.on ( "/setclock", handleSetClock);
  server.on ( "/motor", handleMotor);
  server.on ( "/displayactions", handleDisplayActions);
  server.on ( "/readRules", handleReadRules);
  server.on ( "/status", handleMotorStatus);
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );
}




int toSeconds (s_time t) {
  return (t.h * 3600 + t.mn * 60 + t.s);
}
int toSeconds (DateTime t) {
  return (t.hour() * 3600 + t.minute() * 60 + t.second());
}

bool inTime (s_action action, DateTime current) {
  int start_s, current_s;
  char temp[400];
  start_s = toSeconds(action.start);
  current_s = toSeconds(current);
  bool result = (start_s <= current_s) && (current_s <= (start_s + action.duration));
  snprintf(temp,400,"start_s = %d, end = %d",start_s,start_s+action.duration);
  Serial.print(temp);
  snprintf(temp,400,"current_s = %d - inTime = %s",current_s, result?"True":"False");
  Serial.println(temp);  
  Serial.println("");
  return  result;
}

void computeRule() {
  int i;
  DateTime now = rtc.now();
  Pwm = 0;
  Dir = 0;
  for (i = 0; i < actions_nb; i++) {
    if (inTime(actions[i], now)) {
      Pwm = actions[i].pwm;
      Dir = actions[i].dir;
    }
  }
  if (applyCompute) {
  analogWrite(PWMA, Pwm);
  digitalWrite(BRAKEA, 0);
  digitalWrite(DIRA,Dir);
  }
}

void loop ( void ) {

  server.handleClient();
  computeRule();

}
