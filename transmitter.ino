/*
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
#define THROTTLE_UPPER 950
#define THROTTLE_CENTER 535
#define THROTTLE_LOWER 30
#define THROTTLE_DEADZONE 2 // %

#define STEERING_AXIS A1
#define STEERING_UPPER 950
#define STEERING_CENTER 532
#define STEERING_LOWER 50
#define STEERING_DEADZONE 2 // %

#define LEDPIN 13

boolean debug = true;

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

boolean isLeft(int value, int center, int deadzone_threshold) 
{
  return isReverse(value, center, deadzone_threshold);
}

boolean isRight(int value, int center, int deadzone_threshold)
{
  return isForward(value, center, deadzone_threshold);
}

boolean isDeadZone(int value, int center, int deadzone_threshold)
{
  float ratio = float(center) / float(value);
  float deadzone = float(deadzone_threshold) / float(100);
  return (ratio > (1 - deadzone) && (ratio < (1 + deadzone)));
}

// map(potValue, 0, 1023, 0, 179)
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

  // Check if within deadzone
  if (deadZone) {
    if (debug) Serial.print(", P: DZ, %TH: 0");
    throttleSpeed = 100;
  }

  int currentThrottle = throttleSpeed;
  if (debug) Serial.print(", CTH: " + String(currentThrottle));
  return currentThrottle;
}


// Throttle is 100 for neutral
// 0 for reverse full left.
// 200 full right
int calculateSteering() {
  int steering = 100;
  int sensorValue = analogRead(STEERING_AXIS);
  boolean deadZone = isDeadZone(sensorValue,STEERING_CENTER,STEERING_DEADZONE);
  if (debug) Serial.print(" ||| SV: " + String(sensorValue) + ", D: " + String(deadZone));

  // Right
  if (isRight(sensorValue, STEERING_CENTER, STEERING_DEADZONE)) {
    float percentileSteering = int(percentile(sensorValue, STEERING_CENTER, STEERING_UPPER));
    if (debug) Serial.print(", SP: R, %ST: " + String(percentileSteering));
    steering = 100 + percentileSteering;    
  }

  // Left
  if (isLeft(sensorValue, STEERING_CENTER, STEERING_DEADZONE)) {
    float percentileSteering = int(percentile(sensorValue, STEERING_LOWER, STEERING_CENTER));
    if (debug) Serial.print(", SP: L, %ST: " + String(percentileSteering));
    steering = percentileSteering;    
  }

  // Check if within deadzone
  if (deadZone) {
    if (debug) Serial.print(", SP: SDZ, %ST: 0");
    steering = 100;
  }

  int currentSteering = steering;
  if (debug) Serial.println(", CST: " + String(currentSteering));
  return currentSteering;
}

// Axis are 0 - 200 where 100 is the center point
// Delimiter is the | character
// Separator is :   
void sendMessage()
{
  String msg = String(calculateThrottle()) + ":" + String(calculateSteering()) + "|";
  int str_len = msg.length() + 1;
  char char_array[str_len];
  msg.toCharArray(char_array, msg.length());
  
  rf_driver.send(char_array, str_len);   
  if (!debug) Serial.println("Sending message: " + String(msg));
}

void loop()
{ 
  sendMessage();
  delay(10);
}
