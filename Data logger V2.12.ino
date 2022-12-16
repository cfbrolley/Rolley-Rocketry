//Libraries. This flight computer uses a BMP280 and ADXL345 over I2C and an SD reader over SPI.
#include <Adafruit_BMP280.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <SPI.h>

//Define constants
#define chargepin 6
#define buzzer 5
#define logtime 30 //<--slows down time between logs by this much. current loop can run every 10-11ms which is 100 lines of data per second, but adding a delay between loops reduces this. For example, adding 10ms slows it down to 50 lines per second. 
#define buzztime 3000
#define flushtime 10000
#define armcheck 50 //<--This is the minimum height in meters that the deployment charge can go off. Change this for whatever needed.

//Variables
float temp, pres, alti, bmpcal1, bmpcal2, bmpcal3, altioffset, correctedalt, checkalti, accelx, accely, accelz;
float alticheck = 0;
int deploycount = 0;
long buzzclock, flushclock;
unsigned long timer;
bool armed = false;

//BMP, ADXL and SD things
Adafruit_BMP280 bmp;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();
File logfile;

void setup() {
//setup begin tone
  pinMode(buzzer, OUTPUT);
  tone(buzzer, 3700, 500);
  delay(1000);   
//setup BMP, ADXL and SD
  pinMode (chargepin, OUTPUT);
  Serial.begin(9600);
  bmp.begin(0x76); //<--I2C address is needed apparently otherwise altimeter just spits out garbage.
  accel.begin();
  accel.setRange(ADXL345_RANGE_16_G);
  if (SD.begin(SPI_FULL_SPEED, 10)){
     logfile = SD.open("log.txt", FILE_WRITE);
     logfile.println();
     logfile.println("~~ Logger version 2.12 ~~");
     logfile.println();
     logfile.println("~~ NEW FLIGHT ~~");
     logfile.println();
  }
  if (!SD.begin(10)) {
     Serial.println("SD card initialisation failed");
     while (1);
  }

//Figure out an offest for height above ground. There's a better way to do this with the BMP probably.
  Wire.begin();
  bmpcal1 = bmp.readAltitude();
  delay(500);  
  bmpcal2 = bmp.readAltitude();
  delay(500); 
  bmpcal3 = bmp.readAltitude();
  altioffset = ((bmpcal1 + bmpcal2 + bmpcal3) / 3);

//Double tone to indicate setup is done. 
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
  correctedalt = alti - altioffset;
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
       if (correctedalt > armcheck) {
          armed = true;
          logfile.println("~~ EJECTION CHARGE ARMED ~~");
          }
       }
}

void deployment (void) {
//Run the deployment checks and allows firing of deployment charge if condition is met. 
//Apogee is detected when there have been at least 3 consecutive reductions in altitude. 
//Disables deployment charge for safety if altitude falls below armcheck.


//****     This section of code is can't be properly tested without a flight,     ****
//****     so, no deployment charge will be hooked up for initial flight.         **** 
//****     Prints a line to logfile instead to indicate events.                   ****

  if (correctedalt < alticheck) {
     deploycount = deploycount + 1;
     alticheck = correctedalt;
     }
  else {
       deploycount = 0;
       alticheck = correctedalt;
       }
  if (deploycount > 3) {
     digitalWrite(chargepin, HIGH);
     logfile.println("~~ EJECTION CHARGE FIRED ~~");
     }
  if (correctedalt < armcheck) {
     armed = false;
     deploycount = 0;
     digitalWrite(chargepin, LOW);
     logfile.println("~~ EJECTION CHARGE DISABLED ~~");
     }
}

void loop() {
//Timestamp
  timer = millis();
  while (millis() - timer < logtime) {  
  }
//Tone indicating logging is happening
  if  (timer - buzzclock >= buzztime) {
      tone(buzzer, 4700, 300);
      buzzclock = timer;
      }

//run through all the reads and checks
  readbmp();
  readadxl();
  arming();

//Log the stuff to the file.
  #define COMMA logfile.print(",");
  logfile.print(timer); COMMA;         
  logfile.print(temp); COMMA;
  logfile.print(pres); COMMA;
  logfile.print(alti); COMMA;
  logfile.print(correctedalt); COMMA;
  logfile.print(accelx); COMMA;
  logfile.print(accely); COMMA;
  logfile.print(accelz); COMMA;
  logfile.println();  

//Required to write to the text file properly, otherwise the file comes back empty.
//Loop runs quicker this way compared to opening and then closing the file every loop, allowing more frequent logging.
   if ((timer - flushclock >= flushtime))
      {
       logfile.flush();
       flushclock = timer;
      }      
}
