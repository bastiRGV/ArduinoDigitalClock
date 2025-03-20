#include "Arduino.h"
#include "uRTCLib.h"
#include <RGBmatrixPanel.h>


// uRTCLib DS3231;
uRTCLib rtc(0x68);
char weekday[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


// Adafruit LED Matrix 
#define CLK  8
#define OE   9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);


//Custom Variables
// 0 = Winterzeit, 1 = Sommerzeit
int istSommerzeit = 0;

int positionHourOneX = 1;
int positionHourTwoX = 8;
int positionMinuteOneX = 18;
int positionMinuteTwoX = 25;

int positionHoursMinutesY = 1;

int positionTemperaturOneX = 16;
int positionTemperaturTwoX = 21;

int positionTemperaturY = 10;

//rgb für Textfarbe je 4 Bit
int colorR = 0;
int colorG = 0;
int colorB = 0;


int letzteDargestellteUhrzeit[2] = { 0, 0};

int doodleCycle = 0;

//############################################################################################
//Setup & Loop
//############################################################################################


void setup() {

  //Serial.begin(9600);

  URTCLIB_WIRE.begin();
  rtc.enableBattery();
  
  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
  // set day of week (1=Sunday, 7=Saturday)
  // Winterzeit/Standardzeit
  // rtc.set(20, 38, 18, 5, 20, 3, 25);

  //Alarm 1 jede Stunde zum triggern der Farb-/Iconumstellung
  rtc.alarmClearFlag(URTCLIB_ALARM_1);
  rtc.alarmSet(URTCLIB_ALARM_TYPE_1_FIXED_MS, 0, 0, 0, 0);

  //Alarm 2 jeden Sonntag um 2:00 zum Prüfen auf Sommer-/Winterzeitumstellung
  rtc.alarmClearFlag(URTCLIB_ALARM_2);
  rtc.alarmSet(URTCLIB_ALARM_TYPE_2_FIXED_DOWHM, 0, 0, 2, 1);


  checkSommerzeitOnSetup(); 

  aktualisiereLetzteUhrzeit(letzteDargestellteUhrzeit);

  farbumstellung();

  matrix.begin();

  printDisplay();

  drawDoodle();

}


//--------------------------------------------------------------------------------------------


void loop() {
  
  rtc.refresh();

  checkAlarm();
  
  if(letzteDargestellteUhrzeit[0] != rtc.hour() || letzteDargestellteUhrzeit[1] != rtc.minute()){

    printDisplay();
    aktualisiereLetzteUhrzeit(letzteDargestellteUhrzeit);
  
  }

  if(doodleCycle == 0){
      doodleCycle++;
  }else{
      doodleCycle--;
  }

  drawDoodle();

  delay(1000);

}


//############################################################################################
//ZeitManagement
//############################################################################################


/*
void debugTime(){

  Serial.print("Current Date & Time: ");
  Serial.print(rtc.day());
  Serial.print('/');
  Serial.print(rtc.month());
  Serial.print('/');
  Serial.print(rtc.year());

  Serial.print(" (");
  Serial.print(weekday[rtc.dayOfWeek()-1]);
  Serial.print(") ");

  Serial.print(rtc.hour() + istSommerzeit);
  Serial.print(':');
  Serial.print(rtc.minute());
  Serial.print(':');
  Serial.println(rtc.second());

  Serial.print("Temperature: ");
  Serial.print(rtc.temp()  / 100);
  Serial.print("\xC2\xB0");   //shows degrees character
  Serial.println("C");  

  Serial.println();

}
*/


//--------------------------------------------------------------------------------------------


void checkAlarm(){

  if(rtc.alarmTriggered(URTCLIB_ALARM_ANY)){
    if(rtc.alarmTriggered(URTCLIB_ALARM_1)){

      farbumstellung();

      //reset Doodle am Stundenanfang
      matrix.fillRect(3, 10, 11, 5, matrix.Color444(0, 0, 0));

      //reset Display einmal täglich
      if((rtc.hour() + istSommerzeit) == 4){
        
        matrix.fillScreen(matrix.Color333(0, 0, 0));
        printDisplay();
        drawDoodle();

      }

      rtc.alarmClearFlag(URTCLIB_ALARM_1);

    }

    if(rtc.alarmTriggered(URTCLIB_ALARM_2)){

      checkZeitumstellung();
      rtc.alarmClearFlag(URTCLIB_ALARM_2);

    }

  }

}


//--------------------------------------------------------------------------------------------


void checkZeitumstellung(){

  rtc.refresh();

  //31 ist die Länge von März und Oktober

  if(rtc.month() == 3       &&
     rtc.dayOfWeek() == 1   &&
     rtc.day() + 7 > 31     &&
     rtc.hour() >= 2          ){

      istSommerzeit = 1;

  }

  if(rtc.month() == 10      &&
     rtc.dayOfWeek() == 1   &&
     rtc.day() + 7 > 31     &&
     rtc.hour() >= 2                ){

      istSommerzeit = 0;

  }

}


int checkSommerzeitOnSetup(){

  rtc.refresh();

  if(rtc.month() > 3 && rtc.month() < 10){
    istSommerzeit = 1;
  }

  if(rtc.month() == 3       &&
     rtc.dayOfWeek() == 1   &&
     rtc.day() + 7 > 31     &&
     rtc.hour() >= 2          ){

    istSommerzeit = 1;

  }
  if(rtc.month() == 10      &&
     rtc.dayOfWeek() == 1   &&
     rtc.day() + 7 > 31     &&
     rtc.hour() < 2           ){

    istSommerzeit = 1;

  }

  //prüft, ob nach dem jetzigen Tag noch ein Sonntag im März kommmt
  //setzt istSommerzeit, falls nein
  if(rtc.month() == 3                        &&
     rtc.day() + (8 - rtc.dayOfWeek()) > 31    ){
    
    istSommerzeit = 1;

  }

  //prüft, ob nach dem jetzigen Tag noch ein Sonntag im Oktober kommmt
  //setzt istSommerzeit, falls ja
  if(rtc.month() == 10                        &&
     rtc.day() + (8 - rtc.dayOfWeek()) <= 31    ){
    
    istSommerzeit = 1;

  }

}


//--------------------------------------------------------------------------------------------


void aktualisiereLetzteUhrzeit(int letzteDargestellteUhrzeit[2]){

  rtc.refresh();

  letzteDargestellteUhrzeit[0] = rtc.hour();
  letzteDargestellteUhrzeit[1] = rtc.minute();

}


//############################################################################################
//DisplayManagement
//############################################################################################


void printDisplay(){

  rtc.refresh();

  //-1 zum clearen der Stelle

  int stunde = rtc.hour() + istSommerzeit;
  int minute = rtc.minute();

  int temperatur = rtc.temp() / 100;


  if(stunde > 23){
    stunde = 0;
  }

  if(stunde < 10){

    drawUhrzeit( -1, positionHourOneX, positionHoursMinutesY);
    drawUhrzeit( ((stunde / 1U) % 10), positionHourTwoX, positionHoursMinutesY);
 
  }else{

    drawUhrzeit( ((stunde / 10U) % 10), positionHourOneX, positionHoursMinutesY);
    drawUhrzeit( ((stunde / 1U) % 10), positionHourTwoX, positionHoursMinutesY);

  }

  drawSeperator();

  drawUhrzeit( (minute / 10U) % 10, positionMinuteOneX, positionHoursMinutesY);
  drawUhrzeit( (minute / 1U) % 10, positionMinuteTwoX, positionHoursMinutesY);

  //(rtc.hour() / 10U) % 10 = 10er stelle
  //(rtc.hour() / 1U) % 10 = 1er stelle


  if(temperatur < 10){

    drawTemperatur(-1, positionTemperaturOneX, positionTemperaturY);
    drawTemperatur(temperatur, positionTemperaturTwoX, positionTemperaturY);

  }else{

    drawTemperatur( (temperatur / 10U) % 10, positionTemperaturOneX, positionTemperaturY);
    drawTemperatur( (temperatur / 1U) % 10, positionTemperaturTwoX, positionTemperaturY);

  }

  drawCelsius();

}


//--------------------------------------------------------------------------------------------


void drawSeperator(){

  matrix.drawPixel(15, 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(15, 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(16, 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(16, 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(15, 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(15, 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(16, 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(16, 7, matrix.Color444(colorR, colorG, colorB));

}


//--------------------------------------------------------------------------------------------

void drawUhrzeit(int zahl, int x, int y){

  switch(zahl){

    case 0:

      drawZero(x, y);
      break;


    case 1:

      drawOne(x, y);
      break;


    case 2:

      drawTwo(x, y);
      break;


    case 3:

      drawThree(x, y);
      break;


    case 4:

      drawFour(x, y);
      break;


    case 5:

      drawFive(x, y);
      break;


    case 6:

      drawSix(x, y);
      break;


    case 7:

      drawSeven(x, y);
      break;


    case 8:

      drawEight(x, y);
      break;


    case 9:

      drawNine(x, y);
      break;


    default:

    matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));
    break;

  }

}

//Die Fonts waren erst über jeweils eine Matrix mit loop zur Darstellung angelegt,
//was jedoch beim Aufruf zu einem Overflow des Arbeitsspeichers des UNO R3 führte

void drawZero(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawOne(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawTwo(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawThree(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 2, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawFour(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawFive(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawSix(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawSeven(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawEight(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));

}

void drawNine(int x, int y){

  matrix.fillRect(x, y, 6, 8, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 4, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 4, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 5, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 5, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 6, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 6, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 4, y + 7, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 5, y + 7, matrix.Color444(colorR, colorG, colorB));

}


//--------------------------------------------------------------------------------------------


void drawTemperatur(int zahl, int x, int y){

  switch(zahl){

    case 0:

      drawTempZero(x, y);
      break;

    case 1:

      drawTempOne(x, y);
      break;


    case 2:

      drawTempTwo(x, y);
      break;


    case 3:

      drawTempThree(x, y);
      break;


    case 4:

      drawTempFour(x, y);
      break;


    case 5:

      drawTempFive(x, y);
      break;


    case 6:

      drawTempSix(x, y);
      break;


    case 7:

      drawTempSeven(x, y);
      break;


    case 8:

      drawTempEight(x, y);
      break;


    case 9:

      drawTempNine(x, y);
      break;


    default:

    matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));
    break;

  }

}

void drawTempZero(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempOne(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempTwo(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempThree(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempFour(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempFive(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempSix(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempSeven(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 0, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempEight(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 3, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 4, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawTempNine(int x, int y){

  matrix.fillRect(x, y, 4, 5, matrix.Color333(0, 0, 0));

  matrix.drawPixel(x + 1, y + 0, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 0, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 0, y + 1, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 1, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 1, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 2, y + 2, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(x + 3, y + 2, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 3, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(x + 3, y + 4, matrix.Color444(colorR, colorG, colorB));

}

void drawCelsius(){
  
  matrix.drawPixel(28, 10, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(29, 10, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(30, 10, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(27, 11, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(27, 12, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(27, 13, matrix.Color444(colorR, colorG, colorB));

  matrix.drawPixel(28, 14, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(29, 14, matrix.Color444(colorR, colorG, colorB));
  matrix.drawPixel(30, 14, matrix.Color444(colorR, colorG, colorB));

}


//--------------------------------------------------------------------------------------------


void farbumstellung(){

  rtc.refresh();

  int stunde = rtc.hour() + istSommerzeit;

  if((stunde >= 6 && stunde < 9)   ||
     (stunde >= 18 && stunde < 21)   ){

    //Morgens/Abends
    colorR = 15;
    colorG = 6;
    colorB = 0;

  }else if(stunde >= 9 && stunde < 18){

    //Tagüber
    colorR = 0;
    colorG = 15;
    colorB =9;

  }else{

    //Nachts
    colorR = 3;
    colorG = 0;
    colorB = 6;

  }

}


//--------------------------------------------------------------------------------------------


void drawDoodle(){

  rtc.refresh();

  int stunde = rtc.hour() + istSommerzeit;

  if(stunde >= 6 && stunde < 9){

    drawDoodleMorning();

  }else if(stunde >= 9 && stunde < 18){

    drawDoodleDay();

  }else if(stunde >= 18 && stunde < 21){

    drawDoodleEvening();

  }else{

    drawDoodleNight();

  }

}

void drawDoodleMorning(){

  matrix.drawPixel(4, 14, matrix.Color444(6, 15, 0));
  matrix.drawPixel(5, 14, matrix.Color444(6, 15, 0));
  matrix.drawPixel(6, 14, matrix.Color444(6, 15, 0));
  matrix.drawPixel(7, 14, matrix.Color444(6, 15, 0));
  matrix.drawPixel(8, 14, matrix.Color444(6, 15, 0));
  matrix.drawPixel(9, 14, matrix.Color444(6, 15, 0));
  matrix.drawPixel(10, 14, matrix.Color444(6, 15, 0));
  matrix.drawPixel(11, 14, matrix.Color444(6, 15, 0));
  matrix.drawPixel(12, 14, matrix.Color444(6, 15, 0));

  matrix.drawPixel(6, 13, matrix.Color444(15, 10, 0));
  matrix.drawPixel(7, 13, matrix.Color444(15, 10, 0));
  matrix.drawPixel(8, 13, matrix.Color444(15, 10, 0));
  matrix.drawPixel(9, 13, matrix.Color444(15, 10, 0));
  matrix.drawPixel(10, 13, matrix.Color444(15, 10, 0));

  matrix.drawPixel(7, 12, matrix.Color444(15, 10, 0));
  matrix.drawPixel(8, 12, matrix.Color444(15, 10, 0));

  matrix.drawPixel(9, 12, matrix.Color444(15, 10, 0));

  matrix.drawPixel(11, 12, matrix.Color444(15, 15, 15));
  matrix.drawPixel(12, 12, matrix.Color444(15, 15, 15));

  matrix.drawPixel(4, 11, matrix.Color444(15, 15, 15));
  matrix.drawPixel(5, 11, matrix.Color444(15, 15, 15));

  if(doodleCycle == 0){

    matrix.drawPixel(7, 11, matrix.Color444(15, 10, 0));
    matrix.drawPixel(8, 11, matrix.Color444(15, 10, 0));
    matrix.drawPixel(9, 11, matrix.Color444(15, 10, 0));

    matrix.drawPixel(6, 12, matrix.Color444(15, 10, 0));

    matrix.drawPixel(10, 12, matrix.Color444(15, 10, 0));
    matrix.drawPixel(13, 12, matrix.Color444(15, 15, 15));

    matrix.drawPixel(3, 11, matrix.Color444(0, 0, 0));
    matrix.drawPixel(6, 11, matrix.Color444(15, 15, 15));

    matrix.drawPixel(5, 10, matrix.Color444(15, 15, 15));
    matrix.drawPixel(4, 10, matrix.Color444(0, 0, 0));

    matrix.drawPixel(12, 11, matrix.Color444(15, 15, 15));
    matrix.drawPixel(11, 11, matrix.Color444(0, 0, 0));

  }else{

    matrix.drawPixel(7, 11, matrix.Color444(0, 0, 0));
    matrix.drawPixel(8, 11, matrix.Color444(0, 0, 0));
    matrix.drawPixel(9, 11, matrix.Color444(0, 0, 0));

    matrix.drawPixel(6, 12, matrix.Color444(0, 0, 0));

    matrix.drawPixel(10, 12, matrix.Color444(15, 15, 15));
    matrix.drawPixel(13, 12, matrix.Color444(0, 0, 0));

    matrix.drawPixel(3, 11, matrix.Color444(15, 15, 15));
    matrix.drawPixel(6, 11, matrix.Color444(0, 0, 0));

    matrix.drawPixel(4, 10, matrix.Color444(15, 15, 15));
    matrix.drawPixel(5, 10, matrix.Color444(0, 0, 0));

    matrix.drawPixel(11, 11, matrix.Color444(15, 15, 15));
    matrix.drawPixel(12, 11, matrix.Color444(0, 0, 0));

  }

}

void drawDoodleDay(){

  matrix.drawPixel(4, 11, matrix.Color444(15, 15, 15));
  matrix.drawPixel(5, 11, matrix.Color444(15, 15, 15));
  matrix.drawPixel(6, 11, matrix.Color444(15, 15, 15));

  matrix.drawPixel(5, 12, matrix.Color444(15, 15, 15));

  matrix.drawPixel(7, 12, matrix.Color444(15, 11, 0));
  matrix.drawPixel(8, 12, matrix.Color444(15, 11, 0));

  matrix.drawPixel(5, 13, matrix.Color444(15, 11, 0));
  matrix.drawPixel(6, 13, matrix.Color444(15, 11, 0));
  matrix.drawPixel(7, 13, matrix.Color444(15, 11, 0));
  matrix.drawPixel(8, 13, matrix.Color444(15, 11, 0));

  matrix.drawPixel(6, 14, matrix.Color444(15, 11, 0));
  matrix.drawPixel(7, 14, matrix.Color444(15, 11, 0));

 if(doodleCycle == 0){

    matrix.drawPixel(3, 11, matrix.Color444(15, 15, 15));

    matrix.drawPixel(4, 12, matrix.Color444(15, 15, 15));

    matrix.drawPixel(7, 11, matrix.Color444(15, 11, 0));

    matrix.drawPixel(6, 12, matrix.Color444(15, 11, 0));

    matrix.drawPixel(10, 13, matrix.Color444(15, 15, 15));
    matrix.drawPixel(11, 13, matrix.Color444(15, 15, 15));
    matrix.drawPixel(12, 13, matrix.Color444(0, 0, 0));

    matrix.drawPixel(9, 14, matrix.Color444(15, 15, 15));
    matrix.drawPixel(10, 14, matrix.Color444(15, 15, 15));
    matrix.drawPixel(11, 14, matrix.Color444(15, 15, 15));
    matrix.drawPixel(12, 14, matrix.Color444(15, 15, 15));
    matrix.drawPixel(13, 14, matrix.Color444(0, 0, 0));

  }else{

    matrix.drawPixel(3, 11, matrix.Color444(0, 0, 0));

    matrix.drawPixel(4, 12, matrix.Color444(0, 0, 0));

    matrix.drawPixel(7, 11, matrix.Color444(15, 15, 15));

    matrix.drawPixel(6, 12, matrix.Color444(15, 15, 15));

    matrix.drawPixel(10, 13, matrix.Color444(0, 0, 0));
    matrix.drawPixel(11, 13, matrix.Color444(15, 15, 15));
    matrix.drawPixel(12, 13, matrix.Color444(15, 15, 15));

    matrix.drawPixel(9, 14, matrix.Color444(0, 0, 0));
    matrix.drawPixel(10, 14, matrix.Color444(15, 15, 15));
    matrix.drawPixel(11, 14, matrix.Color444(15, 15, 15));
    matrix.drawPixel(12, 14, matrix.Color444(15, 15, 15));
    matrix.drawPixel(13, 14, matrix.Color444(15, 15, 15));

  }

}

void drawDoodleEvening(){

  matrix.drawPixel(4, 14, matrix.Color444(7, 0, 15));
  matrix.drawPixel(5, 14, matrix.Color444(7, 0, 15));
  matrix.drawPixel(6, 14, matrix.Color444(7, 0, 15));
  matrix.drawPixel(7, 14, matrix.Color444(7, 0, 15));
  matrix.drawPixel(8, 14, matrix.Color444(7, 0, 15));
  matrix.drawPixel(9, 14, matrix.Color444(7, 0, 15));
  matrix.drawPixel(10, 14, matrix.Color444(7, 0, 15));
  matrix.drawPixel(11, 14, matrix.Color444(7, 0, 15));
  matrix.drawPixel(12, 14, matrix.Color444(7, 0, 15));

  matrix.drawPixel(6, 13, matrix.Color444(15, 2, 0));
  matrix.drawPixel(7, 13, matrix.Color444(15, 2, 0));
  matrix.drawPixel(8, 13, matrix.Color444(15, 2, 0));
  matrix.drawPixel(9, 13, matrix.Color444(15, 2, 0));
  matrix.drawPixel(10, 13, matrix.Color444(15, 2, 0));

  matrix.drawPixel(7, 12, matrix.Color444(15, 2, 0));
  matrix.drawPixel(8, 12, matrix.Color444(15, 2, 0));
  matrix.drawPixel(9, 12, matrix.Color444(15, 2, 0));


  matrix.drawPixel(4, 10, matrix.Color444(15, 9, 0));
  matrix.drawPixel(5, 10, matrix.Color444(15, 9, 0));

  matrix.drawPixel(12, 11, matrix.Color444(15, 9, 0));

  if(doodleCycle == 0){

    matrix.drawPixel(7, 11, matrix.Color444(15, 2, 0));
    matrix.drawPixel(8, 11, matrix.Color444(15, 2, 0));
    matrix.drawPixel(9, 11, matrix.Color444(15, 2, 0));

    matrix.drawPixel(6, 12, matrix.Color444(15, 2, 0));
    matrix.drawPixel(10, 12, matrix.Color444(15, 2, 0));


    matrix.drawPixel(3, 10, matrix.Color444(15, 9, 0));
    matrix.drawPixel(6, 10, matrix.Color444(0, 0, 0));

    matrix.drawPixel(11, 11, matrix.Color444(15, 9, 0));
    matrix.drawPixel(13, 11, matrix.Color444(0, 0, 0));

  }else{

    matrix.drawPixel(7, 11, matrix.Color444(0, 0, 0));
    matrix.drawPixel(8, 11, matrix.Color444(0, 0, 0));
    matrix.drawPixel(9, 11, matrix.Color444(0, 0, 0));

    matrix.drawPixel(6, 12, matrix.Color444(0, 0, 0));
    matrix.drawPixel(10, 12, matrix.Color444(0, 0, 0));


    matrix.drawPixel(3, 10, matrix.Color444(0, 0, 0));
    matrix.drawPixel(6, 10, matrix.Color444(15, 9, 0));

    matrix.drawPixel(13, 11, matrix.Color444(15, 9, 0));
    matrix.drawPixel(11, 11, matrix.Color444(0, 0, 0));

  }

}

void drawDoodleNight(){

  matrix.drawPixel(6, 10, matrix.Color444(15, 15, 8));
  matrix.drawPixel(7, 10, matrix.Color444(15, 15, 8));
  matrix.drawPixel(8, 10, matrix.Color444(15, 15, 8));

  matrix.drawPixel(5, 11, matrix.Color444(15, 15, 8));
  matrix.drawPixel(6, 11, matrix.Color444(15, 15, 8));
  matrix.drawPixel(7, 11, matrix.Color444(15, 15, 8));
  matrix.drawPixel(9, 11, matrix.Color444(15, 15, 8));

  matrix.drawPixel(5, 12, matrix.Color444(15, 15, 8));
  matrix.drawPixel(6, 12, matrix.Color444(15, 15, 8));

  matrix.drawPixel(5, 13, matrix.Color444(15, 15, 8));
  matrix.drawPixel(6, 13, matrix.Color444(15, 15, 8));
  matrix.drawPixel(7, 13, matrix.Color444(15, 15, 8));
  matrix.drawPixel(9, 13, matrix.Color444(15, 15, 8));

  matrix.drawPixel(6, 14, matrix.Color444(15, 15, 8));
  matrix.drawPixel(7, 14, matrix.Color444(15, 15, 8));
  matrix.drawPixel(8, 14, matrix.Color444(15, 15, 8));

  if(doodleCycle == 0){

    matrix.drawPixel(3, 13, matrix.Color444(15, 10, 0));
    matrix.drawPixel(3, 12, matrix.Color444(0, 0, 0));

    matrix.drawPixel(11, 11, matrix.Color444(0, 0, 0));
    matrix.drawPixel(12, 11, matrix.Color444(15, 10, 0));

    matrix.drawPixel(11, 13, matrix.Color444(15, 10, 0));
    matrix.drawPixel(12, 13, matrix.Color444(0, 0, 0));

  }else{

    matrix.drawPixel(3, 13, matrix.Color444(0, 0, 0));
    matrix.drawPixel(3, 12, matrix.Color444(15, 10, 0));

    matrix.drawPixel(11, 11, matrix.Color444(15, 10, 0));
    matrix.drawPixel(12, 11, matrix.Color444(0, 0, 0));

    matrix.drawPixel(11, 13, matrix.Color444(0, 0, 0));
    matrix.drawPixel(12, 13, matrix.Color444(15, 10, 0));

  }

}
