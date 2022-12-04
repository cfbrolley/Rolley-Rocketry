//Libraries. This flight computer uses a BMP280 and ADXL345 over I2C and an SD reader over SPI.
#include <Adafruit_BMP280.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <SPI.h>

//Define some things that never change. Like war... war... never changes...
#define chargepin 6
#define buzzer 5
#define buzztime 3000
#define flushtime 10000
#define armcheck 50 //<--this is the minimum height the deployment charge can go off. change this for whatever needed.


//Variables. I've been sloppy with this bit tbh.
float temp, pres, alti, bmpcal1, bmpcal2, bmpcal3, altioffset, corectedalt, checkalti, accelx, accely, accelz;
float alticheck = 0;
int deploycount = 0;
long buzzclock, flushclock;
unsigned long timer;
bool armed = false;

//BMP, ADXL and SD things - IDK how these work please dont ask.
Adafruit_BMP280 bmp;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();
File logfile;

void setup() {
//Initial probably annoying after a while long beep to indicate logger has turned on. 
//thankfully these don't work if you haven't even soldered a buzzer to the board anyway lol.  
  pinMode(buzzer, OUTPUT);
  tone(buzzer, 3700, 500);
  delay(1000);   

//Set up initial values for some variables. don't know if this actually needs to be done 
//but I compare the top two before giving them values and I don't know if comparing to a null value breaks shit or not.

  pinMode (chargepin, OUTPUT);
//Initialisation of serial port, BMP sensor and SD card - IDK how this while thing works either but I'll figure it out I guess
  Serial.begin(9600);
  bmp.begin(0x76); //<--I2C address is needed here otherwise altimeter just spits out garbage.
  accel.begin();
  accel.setRange(ADXL345_RANGE_16_G);
  delay(1000);
  
//Hit the brakes if there's no SD card in the reader, create log.txt to write to if there isn't one already
  if (SD.begin(10)){
     logfile = SD.open("log.txt", FILE_WRITE);
     logfile.println();
     logfile.println("~~ NEW FLIGHT ~~");
     logfile.println();
  }
  if (!SD.begin(10)) {
     Serial.println("SD card initialisation failed");
     while (1);
  }

//Figure out an offest for height above ground
  Wire.begin();
  bmpcal1 = bmp.readAltitude();
  delay(500);  
  bmpcal2 = bmp.readAltitude();
  delay(500); 
  bmpcal3 = bmp.readAltitude();
  altioffset = ((bmpcal1 + bmpcal2 + bmpcal3) / 3);
  Wire.endTransmission();

//Double beep to indicate setup has run. Good, more beeps. 
  tone(buzzer, 3700, 220);
  delay(200);
  tone(buzzer, 5700, 120);
  delay(2000); 
}

void readbmp (void) {
//Read stuff from the BMP sensor, store it in variables
  Wire.begin();
  temp = bmp.readTemperature();
  pres = bmp.readPressure() / 100.0;
  alti = bmp.readAltitude();
  corectedalt = alti - altioffset;
  Wire.endTransmission();
}

void readadxl (void) {
//Read stuff from the accelerometer, store it in variables
  Wire.begin();
  sensors_event_t event; 
  accel.getEvent(&event);
  accelx = event.acceleration.x - 0.143;
  accely = event.acceleration.y + 0.776;
  accelz = event.acceleration.z + 0.969;
  Wire.endTransmission();
}

void arming (void) {
//Run arming check. Altitude above ground must be above armcheck before the deployment check runs.
  if (armed){
     deployment();
     }
  else {
       if (corectedalt > armcheck) {
          armed = true;
          logfile.println("~~ EJECTION CHARGE ARMED ~~");
          Serial.println("~~ EJECTION CHARGE ARMED ~~");
          }
       }
}

void deployment (void) {
//Run the deployment checks and fire deployment charge if condition is met 
//(5 consecutive reductions in altitude to indicate rocket has passed apogee and is starting to descend). 
//Turns off deployment charge for safety if altitude falls below armcheck.

//****     this section of code is can't be tested without a flight,****
//****     so no deployment charge hooked up for initial flight.    **** 
//****     Prints to serial and logfile instead.                    ****

  if (corectedalt < alticheck) {
     deploycount = deploycount + 1;
     alticheck = corectedalt;
     }
  else {
       deploycount = 0;
       alticheck = corectedalt;
       }
  if (deploycount > 3) {
     digitalWrite(chargepin, HIGH);
     logfile.println("~~ EJECTION CHARGE FIRED ~~");
//     Serial.println("~~ EJECTION CHARGE FIRED ~~");
     }
  if (corectedalt < armcheck) {
     armed = false;
     deploycount = 0;
     digitalWrite(chargepin, LOW);
     logfile.println("~~ EJECTION CHARGE TURNED OFF ~~");
//     Serial.println("~~ EJECTION CHARGE TURNED OFF ~~");
     }
}

//void debugging (void) {
//Only needed if plugged in to PC for debugging. Comment it out if all working fine.
//  #define COMMA Serial.print(",");
//  Serial.print(millis()); COMMA;         
//  Serial.print(temp); COMMA;
//  Serial.print(pres); COMMA;
//  Serial.print(alti); COMMA;
//  Serial.print(corectedalt); COMMA;
//  Serial.print(accelx); COMMA;
//  Serial.print(accely); COMMA;
//  Serial.print(accelz);
//  Serial.println(); 
//}

void loop() {
//what's the time mister wolf?
  timer = millis();

//do the thing with the beep
  if  (timer - buzzclock >= buzztime) {
      tone(buzzer, 4700, 300);
      buzzclock = timer;
      }

//run through all the reads and checks
  readbmp();
  readadxl();
  arming();
//  debugging(); //<-- comment out if debugging not needed

//Log the stuff to the file. Keep doing this until I say so. No, we're not having pizza for lunch.
  #define COMMA logfile.print(",");
  logfile.print(timer); COMMA;         
  logfile.print(temp); COMMA;
  logfile.print(pres); COMMA;
  logfile.print(alti); COMMA;
  logfile.print(corectedalt); COMMA;
  logfile.print(accelx); COMMA;
  logfile.print(accely); COMMA;
  logfile.print(accelz); COMMA;
  logfile.println();  

//WHICH ONE OF YAS DIDNE FLUSH THE TOILET WHEN THEY DONE A SHHHIT?! WELL IT WAS ONE OF YAS!!
//required ti write to the text file properly. Doesn't write anything if it's not included for some reason.
   if ((timer - flushclock >= flushtime))
      {
       logfile.flush();
       flushclock = timer;
      }      
}
