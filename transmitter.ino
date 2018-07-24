/*
  433 MHz RF Module Transmitter Demonstration 1
  RF-Xmit-Demo-1.ino
  Demonstrates 433 MHz RF Transmitter Module
  Use with Receiver Demonstration 1

  DroneBot Workshop 2018
  https://dronebotworkshop.com
*/

#include <stdlib.h>

// Include RadioHead Amplitude Shift Keying Library
#include <RH_ASK.h>
// Include dependant SPI Library 
#include <SPI.h> 

// Create Amplitude Shift Keying Object
#define TX_PIN 2
#define RX_PIN 3
RH_ASK rf_driver(4000, RX_PIN, TX_PIN);

// Remote control stuffs
#define THROTTLE_AXIS A0
#define THROTTLE_UPPER 1000
#define THROTTLE_CENTER 532
#define THROTTLE_LOWER 20
#define THROTTLE_DEADZONE 5 // %
#define THROTTLE_SMOOTHING 3

int last_throttle_position = 100;
int throttle_sample_index = 0;
int throttle_samples[THROTTLE_SMOOTHING];

#define STEERING_AXIS A1
#define STEERING_UPPER 250
#define STEERING_CENTER 45
#define STEERING_LOWER 32
#define STEERING_DEADZONE 10 // %

int last_steering_position = 100;
int steering_sample_index = 0;
int steering_samples[3];

long randY;
boolean debug = true;

#define LEDPIN 13

void setup()
{
  pinMode(LEDPIN, OUTPUT);
   
  // Initialize ASK Object
  rf_driver.init();
  Serial.begin(9600);
}


boolean isReverse(int value, int center, int deadzone_threshold)
{
  float ratio = float(center) / float(value);
  float deadzone = float(deadzone_threshold) / float(100);
  return (ratio > (1 + deadzone));
}


boolean isForward(int value, int center, int deadzone_threshold)
{
  float ratio = float(center) / float(value);
  float deadzone = float(deadzone_threshold) / float(100);
  return (ratio < (1 - deadzone));
}


boolean isDeadZone(int value, int center, int deadzone_threshold)
{
  float ratio = float(center) / float(value);
  float deadzone = float(deadzone_threshold) / float(100);
  return (ratio > (1 - deadzone) && (ratio < (1 + deadzone)));
}


float percentile(int value, int lower, int upper, boolean reverse=false) 
{
  float percent = (float(value) - float(lower)) / (float(upper) - float(lower)) * 100;
  if (reverse) {
    percent = 100 - percent;
  }
  if (percent > float(100)) {
    return float(100);
  }
  if (percent < float(0)) {
    return float(0);
  }
  return percent;
}


int average(int * array, int len)
{
  int sum = 0;
  for (int i = 0 ; i < len ; i++)
    sum += array[i] ;

  if (debug) Serial.print(", I: " + String(throttle_sample_index) + ", SUM: " + String(int(sum / len)));
  return int(sum / len);
}

// Throttle is 100 for neutral
// 0 for reverse full speed.
// 200 full speed forward
int calculateThrottle() {

  int throttleSpeed = 100;
  int sensorValue = analogRead(THROTTLE_AXIS);
  boolean deadZone = isDeadZone(sensorValue, THROTTLE_CENTER, THROTTLE_DEADZONE);
  if (debug) Serial.print("V: " + String(sensorValue) + ", D: " + String(deadZone));

  // Forward
  if (isForward(sensorValue, THROTTLE_CENTER, THROTTLE_DEADZONE)) {
    float percentileThrottle = int(percentile(sensorValue, THROTTLE_CENTER, THROTTLE_UPPER));
    if (debug) Serial.print(", P: F, %TH: " + String(percentileThrottle));
    throttleSpeed = 100 + percentileThrottle;    
  }

  // Reverse
  if (isReverse(sensorValue, THROTTLE_CENTER, THROTTLE_DEADZONE)) {
    float percentileThrottle = int(percentile(sensorValue, THROTTLE_LOWER, THROTTLE_CENTER));
    if (debug) Serial.print(", P: R, %TH: " + String(percentileThrottle));
    throttleSpeed = percentileThrottle;    
  }

//  throttle_sample_index += 1;
//  throttle_sample_index = throttle_sample_index % int(THROTTLE_SMOOTHING);
//  throttle_samples[throttle_sample_index] = throttleSpeed;

  // Check if within deadzone
  if (deadZone) {
    if (debug) Serial.print(", P: DZ, %TH: 0");
    throttleSpeed = 100;
  }

//  int currentThrottle = average(throttle_samples, int(THROTTLE_SMOOTHING));
  int currentThrottle = throttleSpeed;
  if (debug) Serial.println(", CTH: " + String(currentThrottle));
  return currentThrottle;
}

// Axis are 0 - 200 where 100 is the center point
// Delimiter is the | character
// Separator is :   
void sendMessage()
{
  String msg = String(calculateThrottle()) + ":" + String(100) + "|";
  int str_len = msg.length() + 1;
  char char_array[str_len];
  msg.toCharArray(char_array, msg.length());
  
  rf_driver.send(char_array, str_len);   
  if (!debug) Serial.println("Sending message: " + String(msg));
//  rf_driver.waitPacketSent();
}

void loop()
{
//  digitalWrite(LEDPIN, HIGH);  
  sendMessage();
//  digitalWrite(LEDPIN, LOW);
  delay(10);
}
