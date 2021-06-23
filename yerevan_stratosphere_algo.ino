#include <OneWire.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <string.h>
#include "MS5611.h"
#include <avr/wdt.h>

/* ====================================================================== */
#define SEALEVELPRESSURE_HPA (1027.00)
#define BALLOON_BURST_PIN   4
#define CHUTE_RLS_PIN     7
#define CHARGE_PWR_PIN    2
#define LED_PIN           41
#define CS_PIN            53
#define BEEP_PIN          26

#define BALLOON_BURST_LED_PIN 22
#define CHUTE_RLS_LED_PIN     24
#define SETUP_LED_PIN         6
#define LOOP_LED_PIN          5

#define CHUTE_ENGAGE_LOG_PIN  31 
#define BALLOON_BURST_LOG_PIN 33 
#define CHUTE_RLS_LOG_PIN  35 

#define ENABLE_SERIAL 0
#if ENABLE_SERIAL == 1
#define SERIAL_BEGIN(x)    Serial.begin(x)
#define SERIAL_PRINT(x)    Serial.print(x)
#define SERIAL_PRINTLN(x)    Serial.println(x)
#else
#define SERIAL_BEGIN(x)    
#define SERIAL_PRINT(x)    
#define SERIAL_PRINTLN(x)    
#endif

/* Sensors */

Adafruit_MPU6050 mpu;
MS5611 ms5611_baro;

File logFile;
String filename;
//int log_write_count=0;

bool burst = false;
bool free_fall = false;

unsigned long start = 0;
unsigned long stop = 0;
//int counter = 0;
double prev_acc = -1;

// the array we want to find the median
double acc_list[5] = {10, 10, 10, 10, 10};
double altitude_list[5] = {0, 0, 0, 0, 0};

unsigned int curr_acc_ind = 0;
unsigned int curr_alt_ind = 0;

int PARACHUTE_OPEN_ALTITUDE = 5000;
int PARACHUTE_ENGAGE_ALTITUDE = 5500;
int BALLOON_BURST_ALTITUDE = 30000;
double BAILOUT_TOP_ACCEL = 4.5;
int BAILOUT_BOTTOM_ACCEL = 0;
double FREE_FALL_TIME = 800;

//  parachute realese logic
bool parachute_engage = false;
bool parachute_rel = false;

void setup() {
  wdt_enable(WDTO_15MS);
  SERIAL_BEGIN(9600);   //  PC
  //******** Initialize SD CARD *************
  SERIAL_PRINTLN("Starting AYAS Balloon v0.2");
  //delay(5000);
  pinMode(BALLOON_BURST_LED_PIN, OUTPUT);
  pinMode(CHUTE_RLS_LED_PIN, OUTPUT);
  pinMode(SETUP_LED_PIN, OUTPUT);
  pinMode(LOOP_LED_PIN, OUTPUT);

  pinMode(CHUTE_ENGAGE_LOG_PIN, OUTPUT);
  pinMode(BALLOON_BURST_LOG_PIN, OUTPUT);
  pinMode(CHUTE_RLS_LOG_PIN, OUTPUT);

  pinMode(BALLOON_BURST_PIN, OUTPUT);
  pinMode(CHUTE_RLS_PIN, OUTPUT);
  pinMode(CHARGE_PWR_PIN, OUTPUT);

  pinMode(CS_PIN, OUTPUT);
  pinMode(BEEP_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);  //   Delete

  digitalWrite(SETUP_LED_PIN, HIGH);
  SERIAL_PRINTLN();
  SERIAL_PRINT("Initializing SD card...");
  if (!SD.begin(CS_PIN)) {
    SERIAL_PRINTLN("failed!");
    while (1);
  }
  SERIAL_PRINT("done. ");
  delay(500);
  SERIAL_PRINT("Log file: ");

  String filename = getFilename();
  SERIAL_PRINT(filename);
  if (logFile = SD.open(filename, FILE_WRITE)) {
    logFile.println("flight_millis,temp,baro,alti,acc_x,acc_y,acc_z,acc_3d,rot_x,rot_y,rot_z,acc_tmp,event_engage,event_burst,event_chute");
    logFile.close();
  } else {
    SERIAL_PRINTLN(" Failed!");
    while (1);  
  }
    SERIAL_PRINTLN(" Created!");
  
   /* initializing sensors */
  SERIAL_PRINT("MPU6050...");
  if (!mpu.begin()) {
    SERIAL_PRINTLN("Failed!");
    while (1) {
      delay(10);
    }
  }
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  SERIAL_PRINTLN("setup and done!");
  SERIAL_PRINT("MS5611... ");
  while(!ms5611_baro.begin(MS5611_ULTRA_HIGH_RES)) {
    delay(500);
  }
  SERIAL_PRINTLN("done. ");
  
  digitalWrite(BEEP_PIN, HIGH);
  delay(50);
  digitalWrite(BEEP_PIN, LOW);
  delay(50);
  digitalWrite(BEEP_PIN, HIGH);
  delay(50);
  digitalWrite(BEEP_PIN, LOW);
  delay(2000);
  logFile = SD.open(filename, FILE_WRITE);
  digitalWrite(SETUP_LED_PIN, LOW);
  digitalWrite(CHARGE_PWR_PIN, HIGH);
}


/* THE LOOP IS IN THE END :) */



/* seting up the filename*/
String getFilename() {
  String file_name = "blun_";
  int file_count;
  file_count = EEPROM.read(0);
  file_name.concat(file_count); 
  file_name.concat(".csv"); 
  file_count++;
  EEPROM.write(0, file_count);
  return file_name;
}






void data_log() {

  double realTemperature = ms5611_baro.readTemperature(true);
  long realPressure = ms5611_baro.readPressure(true);
  double realAltitude = ms5611_baro.getAltitude(realPressure);

  sensors_event_t a, g, mpu_temp;
  mpu.getEvent(&a, &g, &mpu_temp);
  double accel_3d = sqrt(accel_3d_sum(a.acceleration.x, a.acceleration.y, a.acceleration.z));
  
//  logFile = SD.open(filename, FILE_WRITE);
  logFile.print(millis());
  logFile.print(",");
  logFile.print(realTemperature);
  logFile.print(",");
  logFile.print(realPressure);
  logFile.print(",");
  logFile.print(realAltitude);
  logFile.print(",");
  logFile.print(a.acceleration.x);
  logFile.print(",");
  logFile.print(a.acceleration.y);
  logFile.print(",");
  logFile.print(a.acceleration.z);
  logFile.print(",");
  logFile.print(accel_3d);
  logFile.print(",");
  logFile.print(g.gyro.x);
  logFile.print(",");
  logFile.print(g.gyro.y);
  logFile.print(",");
  logFile.print(g.gyro.z);
  logFile.print(",");
  logFile.print(mpu_temp.temperature);
  logFile.print(",");
  logFile.print(burst);
  logFile.print(",");
  logFile.print(parachute_rel);
  logFile.print(",");
  logFile.print(parachute_engage);
  logFile.println();
  logFile.flush();
}


void serial_debug() {

  double realTemperature = ms5611_baro.readTemperature(true);
  long realPressure = ms5611_baro.readPressure(true);
  double realAltitude = ms5611_baro.getAltitude(realPressure);

  sensors_event_t a, g, mpu_temp;
  mpu.getEvent(&a, &g, &mpu_temp);
  double accel_3d = sqrt(accel_3d_sum(a.acceleration.x, a.acceleration.y, a.acceleration.z));
  
  SERIAL_PRINT(millis());
  SERIAL_PRINT(",");
  SERIAL_PRINT(realTemperature);
  SERIAL_PRINT(",");
  SERIAL_PRINT(realPressure);
  SERIAL_PRINT(",");
  SERIAL_PRINT(realAltitude);
  SERIAL_PRINT(",");
  SERIAL_PRINT(a.acceleration.x);
  SERIAL_PRINT(",");
  SERIAL_PRINT(a.acceleration.y);
  SERIAL_PRINT(",");
  SERIAL_PRINT(a.acceleration.z);
  SERIAL_PRINT(",");
  SERIAL_PRINT(accel_3d);
  SERIAL_PRINT(",");
  SERIAL_PRINT(g.gyro.x);
  SERIAL_PRINT(",");
  SERIAL_PRINT(g.gyro.y);
  SERIAL_PRINT(",");
  SERIAL_PRINT(g.gyro.z);
  SERIAL_PRINT(",");
  SERIAL_PRINT(mpu_temp.temperature);
  SERIAL_PRINT(",");
  SERIAL_PRINT(burst);
//  SERIAL_PRINT(",");
//  SERIAL_PRINT(chute_ref);
  SERIAL_PRINTLN();
  
}



/* counting 3D acceleration */
  double accel_3d_sum(double x, double y, double z) {
  double result = x*x + y*y + z*z;
  return result;
}




/* ================================================================================ */
/*                                                                                  */
/*                     A L G O R I T H M      S E C T I O N                         */
/*                                                                                  */
/* ================================================================================ */


// Function to sort an array to find the median of an array
void array_sort(double *array, int n)
{
    // sorts array ascending way
    // declare some local variables
    int i = 0, j = 0, temp = 0;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n - 1; j++)
        {
            if (array[j] > array[j + 1])
            {
                temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
}

// function to calculate the median of the array
float find_median(double array[], int n)
{
    float median = 0;

    // if number of elements are even
    if (n % 2 == 0)
        median = (array[(n - 1) / 2] + array[n / 2]) / 2.0;
    // if number of elements are odd
    else
        median = array[n / 2];

    return median;
}


void is_burst(double altitude, double acc)
{
    int n = 5; // the lenght of an array
    acc_list[curr_acc_ind++%n] = acc;
    float median = 0;
    
    // ##############################
    // # if the ballon goes too high#
    // ##############################
    if (altitude >= BALLOON_BURST_ALTITUDE )
    {
        SERIAL_PRINTLN("balloon released ");
        burst = true;
    }
    if (parachute_engage && (altitude <= PARACHUTE_OPEN_ALTITUDE))
    {
        SERIAL_PRINTLN("parachute is open now ");
        burst =true; 
    }
    // Sort the array in ascending order
    double copied_acc[n];
    int loop = 0;
    for (loop = 0; loop < n; loop++)
    {
        copied_acc[loop] = acc_list[loop];
    }
    array_sort(copied_acc, n);

    // Now pass the sorted array to calculate
    // the median of your array.
    double curr_acc = find_median(copied_acc, n);
//    SERIAL_PRINT("curr acc = ");
//    SERIAL_PRINTLN(curr_acc);
//    SERIAL_PRINT("prev acc = ");
//    SERIAL_PRINTLN(prev_acc);
    if ((BAILOUT_BOTTOM_ACCEL <= curr_acc && curr_acc <= BAILOUT_TOP_ACCEL) && (BAILOUT_BOTTOM_ACCEL <= prev_acc && prev_acc <= BAILOUT_TOP_ACCEL))
    {
        prev_acc = curr_acc;

        if (false == free_fall)
        {
            start = millis();
            free_fall = true;
        }

        else if (free_fall)
        {
            stop = millis();
            //  if 1 sec continious free falling
            SERIAL_PRINT("time diff:");
            SERIAL_PRINTLN(stop - start);
            if (stop - start > FREE_FALL_TIME )
            {

                SERIAL_PRINTLN("descending 1 second in a row of ");

                burst = true;
            }
        }
    }
    else
    {
        SERIAL_PRINTLN("not right acc condition----------------------- ");
        prev_acc = curr_acc;
        free_fall = false;
    }
    SERIAL_PRINT("free fall ");
    SERIAL_PRINTLN(free_fall);
  
}


void parachute_relief(double altitude, bool burst)
{ // if ballon goes up from 6000 m then comes down to 5000 m than the parachute is realesed

    // # condition for descending open up the parachute
    SERIAL_PRINT("curr altitude: ");
    SERIAL_PRINTLN( altitude);
    if (parachute_engage && (altitude <= PARACHUTE_OPEN_ALTITUDE))
    {
        SERIAL_PRINTLN("parachute is open now ");
        parachute_rel = true;
    }
    // # check if you once passed 6000 metre means you are ascending
    if ((not parachute_engage) && (altitude >= PARACHUTE_ENGAGE_ALTITUDE))
    {
        SERIAL_PRINTLN("you have passed 6000 m, parachute is closed ");
        parachute_engage = true;
    }
    // # additional condition
    if (burst && (altitude <= PARACHUTE_OPEN_ALTITUDE))
        parachute_rel = true;

  
}

double curr_alt = 0;
bool beep_status = false;
unsigned long last_time =0;
bool keep_balloon_burst_power_pin_high = true;
bool keep_parachute_rel_power_pin_high = true;
//int count = 0;
long log_count = 0;

void loop() {
  wdt_reset();
//  if (count > 10)
//    parachute_engage = true;
//  if (count > 20)
//    burst = true;
//  if (count > 30)
//    parachute_rel = true;
//  count +=1;
  digitalWrite(LOOP_LED_PIN, HIGH);
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  

  // Read accel values
  double ax = a.acceleration.x;
  double ay = a.acceleration.y;
  double az = a.acceleration.z;
  double accel = sqrt(ax*ax + ay*ay +az*az);

  // Read barometre values
 
   
//  SERIAL_PRINT("accel: ");
//  SERIAL_PRINT(accel);
//  SERIAL_PRINTLN("");
//  SERIAL_PRINT("altitide: ");
//  SERIAL_PRINT(altitude);
//  SERIAL_PRINTLN("");

//reading altitude value every 50 iteration
  if (log_count%20 == 0)
  {
        long press = ms5611_baro.readPressure();
       
        double altitude = ms5611_baro.getAltitude(press);
        int n = 5; // the lenght of an array
        altitude_list[curr_alt_ind++%n] = altitude;
        double copied_alt[n];
        int loop;
      //  
        digitalWrite(LOOP_LED_PIN, LOW);
        for (loop = 0; loop < n; loop++)
        {
            copied_alt[loop] = altitude_list[loop];
        }
        
        array_sort(copied_alt, n);
        // Now pass the sorted array to calculate
        // the median of your array.
        curr_alt = find_median(copied_alt, n);
  }
//  SERIAL_PRINTLN(curr_alt);
  
////////////////////////////////////////////////////

  if (not burst)
      is_burst(curr_alt, accel);
//  SERIAL_PRINT("burst: ");
//  SERIAL_PRINTLN(burst);
  if (not parachute_rel)
    parachute_relief(curr_alt, burst);

//serial_debug();
   if(burst)
       { 
//     keep power of balloon rel high 5 sec, then turn off

         if (keep_balloon_burst_power_pin_high)
         {    digitalWrite(BALLOON_BURST_LED_PIN, HIGH);
              digitalWrite(BALLOON_BURST_LOG_PIN, HIGH);
              digitalWrite(BALLOON_BURST_PIN, HIGH);
              data_log();
              delay(2000);
              digitalWrite(BALLOON_BURST_PIN, LOW);
              keep_balloon_burst_power_pin_high = false;
         }
       }

   
   if (parachute_rel )
    { 

     
//     keep power of parachute rel high 5 sec, then turn off
     if (keep_parachute_rel_power_pin_high)
         {    digitalWrite(CHARGE_PWR_PIN, LOW);
              digitalWrite(CHUTE_RLS_LED_PIN, HIGH);
              digitalWrite(CHUTE_RLS_LOG_PIN, HIGH);
              digitalWrite(CHUTE_RLS_PIN, HIGH); 
              data_log();
              delay(2000);
              digitalWrite(CHUTE_RLS_PIN, LOW);
              keep_parachute_rel_power_pin_high = false;
         }

     // BEEP BEEP every 200 milisecond 
      if (beep_status == false && log_count % 51 == 0) {
        digitalWrite(BEEP_PIN, HIGH);
        beep_status == true;
      } else if(log_count % 47 == 0) {
        digitalWrite(BEEP_PIN, LOW);
        beep_status == false;
      }
    
    } 
    
    
   if (parachute_engage)
       digitalWrite(CHUTE_ENGAGE_LOG_PIN, HIGH);
   
   if (log_count % 100 == 0) { 
    data_log();
   }
  
   log_count++;


}
