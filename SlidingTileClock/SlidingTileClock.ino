
/*
  WiFi connected Sliding Tile Clock. 
  This sketch gets current time from NTP server.
  On start or reset, dials are assumed to already show the current time.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <NTP.h>
#include "credentials.h"

WebServer server(80);


WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

const char *ntpServer = "0.nl.pool.ntp.org";


// Stepper setings
int delaytime = 2;  // wait for a single step of stepper

// Motor pin assignments (names reflect physical wiring position, not digit driven)
// Actual mapping: motor1 drives minute units, motor2 drives minute tens,
//                 motor3 drives hour units, motor4 drives hour tens
int motor1[4] = {13,12,11,10};
int motor2[4] = {9,8,7,6};
int motor3[4] = {5,4,3,2};
int motor4[4] = {A3,A2,A1,A0};

// Motors indexed by digit: 0=minute unit, 1=minute tenth, 2=hour unit, 3=hour tenth
int *motors[4] = {motor1, motor2, motor3, motor4};

// sequence of stepper motor control
int seq[8][4] = {
  {  LOW, HIGH, HIGH,  LOW},
  {  LOW,  LOW, HIGH,  LOW},
  {  LOW,  LOW, HIGH, HIGH},
  {  LOW,  LOW,  LOW, HIGH},
  { HIGH,  LOW,  LOW, HIGH},
  { HIGH,  LOW,  LOW,  LOW},
  { HIGH, HIGH,  LOW,  LOW},
  {  LOW, HIGH,  LOW,  LOW}
};

// number steps to turn one Tile
const int HALFSTEPS = 2048;

// Set to true for 12-hour display, false for 24-hour
const bool TWELVE_HOUR = true;

int actualMinuteTileUnit = 0;
int actualMinuteTileTenth = 0;
int actualHourTileUnit = 0;
int actualHourTileTenth = 0;

int newMinuteTileUnit = 0;
int newMinuteTileTenth = 0;
int newHourTileUnit = 0;
int newHourTileTenth = 0;

// Per-motor stepper phase tracking
int motorPhase[4] = {0, 0, 0, 0};

// functions

int motorIndex(int motorport[4]) {
  for (int i = 0; i < 4; i++) if (motors[i] == motorport) return i;
  return 0;
}

void rotate(int step, int motorport[4]) {
  int &phase = motorPhase[motorIndex(motorport)];
  int i, j;
  int delta = (step > 0) ? 1 : 7;

  step = (step > 0) ? step : -step;
  for(j = 0; j < step; j++) {
    phase = (phase + delta) % 8;
    for(i = 0; i < 4; i++) {
      digitalWrite(motorport[i], seq[phase][i]);
    }
    delay(delaytime);
  }
  // power cut
  for(i = 0; i < 4; i++) {
    digitalWrite(motorport[i], LOW);
  }
}

void rotateTile(int steps, int motorport[4]) {
  for (int i = 0; i < steps; i++) {
    rotate(HALFSTEPS, motorport);
    server.handleClient();
  }
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  int spaceIdx = cmd.indexOf(' ');
  if (spaceIdx < 0) return;
  int motor = cmd.substring(0, spaceIdx).toInt();
  int steps = cmd.substring(spaceIdx + 1).toInt();
  if (motor < 1 || motor > 4 || steps == 0) return;
  Serial.print("Motor "); Serial.print(motor);
  Serial.print(" steps "); Serial.println(steps);
  rotate(steps, motors[motor - 1]);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head><title>Sliding Tile Clock</title>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<style>body{font-family:sans-serif;max-width:480px;margin:auto;padding:1em}
fieldset{margin:1em 0}select,button{font-size:1.2em;margin:0.2em}
button{padding:0.4em 1em}</style></head><body>
<h2>Sliding Tile Clock</h2>
<form action='/setpos' method='get'>
<fieldset><legend>Set Current Dial Position</legend>
<label>Hour tens: <select name='ht'><option>0</option><option>1</option><option>2</option></select></label>
<label>Hour units: <select name='hu'>)rawliteral";
  for (int i = 0; i <= 9; i++) html += "<option>" + String(i) + "</option>";
  html += R"rawliteral(</select></label><br>
<label>Min tens: <select name='mt'>)rawliteral";
  for (int i = 0; i <= 5; i++) html += "<option>" + String(i) + "</option>";
  html += R"rawliteral(</select></label>
<label>Min units: <select name='mu'>)rawliteral";
  for (int i = 0; i <= 9; i++) html += "<option>" + String(i) + "</option>";
  html += R"rawliteral(</select></label><br>
<button type='submit'>Set Position</button></fieldset></form>
<form action='/nudge' method='get'>
<fieldset><legend>Nudge Alignment (steps)</legend>
<label>Motor: <select name='m'>
<option value='1'>Min units</option><option value='2'>Min tens</option>
<option value='3'>Hour units</option><option value='4'>Hour tens</option>
</select></label>
<label>Steps: <input type='number' name='s' value='50' style='width:5em'></label>
<button type='submit'>Nudge</button></fieldset></form>
<p>Current position: )rawliteral";
  html += String(actualHourTileTenth) + String(actualHourTileUnit) + ":" +
          String(actualMinuteTileTenth) + String(actualMinuteTileUnit);
  html += "</p><p>Target: " + String(newHourTileTenth) + String(newHourTileUnit) + ":" +
          String(newMinuteTileTenth) + String(newMinuteTileUnit);
  html += "</p><p>NTP hour: " + String(ntp.hours()) + " min: " + String(ntp.minutes());
  html += "</p><p>IP: " + WiFi.localIP().toString() + "</p></body></html>";
  server.send(200, "text/html", html);
}

void handleSetPos() {
  if (server.hasArg("ht")) actualHourTileTenth = server.arg("ht").toInt();
  if (server.hasArg("hu")) actualHourTileUnit = server.arg("hu").toInt();
  if (server.hasArg("mt")) actualMinuteTileTenth = server.arg("mt").toInt();
  if (server.hasArg("mu")) actualMinuteTileUnit = server.arg("mu").toInt();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleNudge() {
  int m = server.arg("m").toInt();
  int s = server.arg("s").toInt();
  if (m >= 1 && m <= 4 && s != 0) rotate(s, motors[m - 1]);
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting ...");
    delay(500);
    }
  Serial.println("Connected");  
  ntp.updateInterval(60000); // update every minute
  ntp.ruleDST("PDT", Second, Sun, Mar, 2, -420); // second Sunday in March 2:00, UTC-7
  ntp.ruleSTD("PST", First, Sun, Nov, 2, -480); // first Sunday in November 2:00, UTC-8
  ntp.begin(ntpServer);
  Serial.println("start NTP");
  delay (500);

  for (int m = 0; m < 4; m++)
    for (int p = 0; p < 4; p++)
      pinMode(motors[m][p], OUTPUT);
  digitalWrite(A0, LOW);
  digitalWrite(A1, LOW);
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);

  // Assume dials already show current time at power-on
  ntp.update();
  int h = ntp.hours();
  if (TWELVE_HOUR) { h = h % 12; if (h == 0) h = 12; }
  actualMinuteTileUnit = ntp.minutes()%10;
  actualMinuteTileTenth = ntp.minutes()/10;
  actualHourTileUnit = h%10;
  actualHourTileTenth = h/10;

  server.on("/", handleRoot);
  server.on("/setpos", handleSetPos);
  server.on("/nudge", handleNudge);
  server.begin();
  MDNS.begin("st");
  Serial.print("Web server at http://st.local (");
  Serial.print(WiFi.localIP());
  Serial.println(")");
  }

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.reconnect();
    delay(5000);
    return;
  }
  server.handleClient();
  handleSerial();
  ntp.update();
  Serial.println(ntp.formattedTime("%d. %B %Y")); // dd. Mmm yyyy
  Serial.println(ntp.formattedTime("%A %T")); // Www hh:mm:ss
  
  newMinuteTileUnit = ntp.minutes()%10;
  newMinuteTileTenth = ntp.minutes()/10;
  int h = ntp.hours();
  if (TWELVE_HOUR) { h = h % 12; if (h == 0) h = 12; }
  newHourTileUnit = h%10;
  newHourTileTenth = h/10;

  Serial.print("New hours  : "); Serial.print(newHourTileTenth); Serial.println(newHourTileUnit); 
  Serial.print("New minutes: "); Serial.print(newMinuteTileTenth); Serial.println(newMinuteTileUnit); 

  // Skip motor updates if nothing changed
  if (newMinuteTileUnit == actualMinuteTileUnit &&
      newMinuteTileTenth == actualMinuteTileTenth &&
      newHourTileUnit == actualHourTileUnit &&
      newHourTileTenth == actualHourTileTenth) {
    delay(1000);
    return;
  }

  // Update dials in random order
  int order[4] = {0, 1, 2, 3};
  for (int i = 3; i > 0; i--) { int j = random(i + 1); int t = order[i]; order[i] = order[j]; order[j] = t; }

  int hourTenthWrap = 3;  // physical dial has 3 tiles (0,1,2)
  for (int i = 0; i < 4; i++) {
    switch (order[i]) {
      case 0: // minute unit
        if (newMinuteTileUnit > actualMinuteTileUnit)
          rotateTile((newMinuteTileUnit - actualMinuteTileUnit), motors[0]);
        else if (newMinuteTileUnit < actualMinuteTileUnit)
          rotateTile((newMinuteTileUnit + 10 - actualMinuteTileUnit), motors[0]);
        break;
      case 1: // minute tenth
        if (newMinuteTileTenth > actualMinuteTileTenth)
          rotateTile((newMinuteTileTenth - actualMinuteTileTenth), motors[1]);
        else if (newMinuteTileTenth < actualMinuteTileTenth)
          rotateTile((newMinuteTileTenth + 6 - actualMinuteTileTenth), motors[1]);
        break;
      case 2: // hour unit
        if (newHourTileUnit > actualHourTileUnit)
          rotateTile((newHourTileUnit - actualHourTileUnit), motors[2]);
        else if (newHourTileUnit < actualHourTileUnit)
          rotateTile((newHourTileUnit + 10 - actualHourTileUnit), motors[2]);
        break;
      case 3: // hour tenth
        if (newHourTileTenth > actualHourTileTenth)
          rotateTile((newHourTileTenth - actualHourTileTenth), motors[3]);
        else if (newHourTileTenth < actualHourTileTenth)
          rotateTile((newHourTileTenth + hourTenthWrap - actualHourTileTenth), motors[3]);
        break;
    }
  }

  actualMinuteTileUnit = newMinuteTileUnit;
  actualMinuteTileTenth = newMinuteTileTenth;
  actualHourTileUnit = newHourTileUnit;
  actualHourTileTenth = newHourTileTenth;
  
  delay(1000);
  }

