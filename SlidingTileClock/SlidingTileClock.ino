
/*
  WiFi connected Sliding Tile Clock. 
  This sketch gets current time from NTP server.
  On start or reset all sliding tiles should be zero 00:00  
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <NTP.h>
#include "credentials.h"


WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

const char *ntpServer = "0.nl.pool.ntp.org";


// Stepper setings
int delaytime = 2;  // wait for a single step of stepper

// ports used to control the stepper motor
// if your motor rotate to the opposite direction, 
// change the order as {4, 3, 2, 1};
//int hourunitdigitmotor[4] = {10,11,12,13};  // minute motor units
//int minutetenthdigitmotor[4] = {6,7,8,9};      // minute motor tenth
//int minuteunitdigitmotor[4] = {2,3,4,5};      // hour motor units
//int hourtenthdigitmotor[4] = {A0,A1,A2,A3};  // hour motor tenth

int hourunitdigitmotor[4] = {13,12,11,10};  // minute motor units
int minutetenthdigitmotor[4] = {9,8,7,6};      // minute motor tenth
int minuteunitdigitmotor[4] = {5,4,3,2};      // hour motor units
int hourtenthdigitmotor[4] = {A3,A2,A1,A0};  // hour motor tenth

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

int actualMinuteTileUnit = 0;
int actualMinuteTileTenth = 0;
int actualHourTileUnit = 0;
int actualHourTileTenth = 0;

int newMinuteTileUnit = 0;
int newMinuteTileTenth = 0;
int newHourTileUnit = 0;
int newHourTileTenth = 0;

// functions


void rotate(int step, int motorport[4]) {
  static int phase = 0;
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
  }
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

  pinMode(hourunitdigitmotor[0], OUTPUT);
  pinMode(hourunitdigitmotor[1], OUTPUT);
  pinMode(hourunitdigitmotor[2], OUTPUT);
  pinMode(hourunitdigitmotor[3], OUTPUT);
  pinMode(minutetenthdigitmotor[0], OUTPUT);
  pinMode(minutetenthdigitmotor[1], OUTPUT);
  pinMode(minutetenthdigitmotor[2], OUTPUT);
  pinMode(minutetenthdigitmotor[3], OUTPUT);
  pinMode(minuteunitdigitmotor[0], OUTPUT);
  pinMode(minuteunitdigitmotor[1], OUTPUT);
  pinMode(minuteunitdigitmotor[2], OUTPUT);
  pinMode(minuteunitdigitmotor[3], OUTPUT);
  pinMode(hourtenthdigitmotor[0], OUTPUT);
  pinMode(hourtenthdigitmotor[1], OUTPUT);
  pinMode(hourtenthdigitmotor[2], OUTPUT);
  pinMode(hourtenthdigitmotor[3], OUTPUT);
  digitalWrite(A0, LOW);
  digitalWrite(A1, LOW);
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW); 
  }

void loop() {
  ntp.update();
  Serial.println(ntp.formattedTime("%d. %B %Y")); // dd. Mmm yyyy
  Serial.println(ntp.formattedTime("%A %T")); // Www hh:mm:ss
  
  if (ntp.hours()!=hour() || ntp.minutes()!=minute()) { 
    setTime(ntp.hours(),ntp.minutes(),ntp.seconds(),ntp.day(),ntp.month(),ntp.year());};  // synchroniseer clock when ther is a time diverence
  
  newMinuteTileUnit = minute()%10;
  newMinuteTileTenth = minute()/10;
  newHourTileUnit = hour()%10;
  newHourTileTenth = hour()/10;

  Serial.print("New hours  : "); Serial.print(newHourTileTenth); Serial.println(newHourTileUnit); 
  Serial.print("New minutes: "); Serial.print(newMinuteTileTenth); Serial.println(newMinuteTileUnit); 
  
  //set minute unit tile  
  if (newMinuteTileUnit > actualMinuteTileUnit) {
    rotateTile((newMinuteTileUnit-actualMinuteTileUnit), hourunitdigitmotor);
  }
  else if (newMinuteTileUnit < actualMinuteTileUnit){
    rotateTile((newMinuteTileUnit + 10 - actualMinuteTileUnit), hourunitdigitmotor);
  }

  //set minute tenth tile 
  if (newMinuteTileTenth > actualMinuteTileTenth) {
    rotateTile((newMinuteTileTenth - actualMinuteTileTenth), minutetenthdigitmotor);
  }
  else if (newMinuteTileTenth < actualMinuteTileTenth) {
    rotateTile((newMinuteTileTenth + 6 - actualMinuteTileTenth), minutetenthdigitmotor);
  }

  //set hour unit tile
  if (newHourTileUnit > actualHourTileUnit){
   rotateTile((newHourTileUnit-actualHourTileUnit), minuteunitdigitmotor); 
  }
  else if (newHourTileUnit < actualHourTileUnit){
    rotateTile((newHourTileUnit + 10 - actualHourTileUnit), minuteunitdigitmotor);
  }

  //set hour tenth tile
  if (newHourTileTenth > actualHourTileTenth) {
    rotateTile((newHourTileTenth - actualHourTileTenth), hourtenthdigitmotor);
  }
  else if (newHourTileTenth < actualHourTileTenth) {
    rotateTile((newHourTileTenth + 3 - actualHourTileTenth), hourtenthdigitmotor);
  }

  actualMinuteTileUnit = newMinuteTileUnit;
  actualMinuteTileTenth = newMinuteTileTenth;
  actualHourTileUnit = newHourTileUnit;
  actualHourTileTenth = newHourTileTenth;
  
  delay(1000);
  }

