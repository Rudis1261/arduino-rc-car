/*
  433 MHz RF Module Receiver Demonstration 1
  RF-Rcv-Demo-1.ino
  Demonstrates 433 MHz RF Receiver Module
  Use with Transmitter Demonstration 1

  DroneBot Workshop 2018
  https://dronebotworkshop.com
*/

// Include RadioHead Amplitude Shift Keying Library
#include <RH_ASK.h>
#include <SPI.h> 

// Create Amplitude Shift Keying Object
#define LEDPIN 13
#define TX_PIN 2
#define RX_PIN 3
#define MOTOR_PIN_FORWARD 11
#define MOTOR_PIN_REVERSE 12
#define MOTOR_PIN_PWM 5
#define FAILSAFE_COUNTER 50

RH_ASK rf_driver(4000, RX_PIN, TX_PIN);

int failSafeCounter = 0;
int throttleValue = 100;
int steeringValue = 100;
boolean moveClockwise = true;

void setup()
{
  pinMode(MOTOR_PIN_FORWARD, OUTPUT);
  pinMode(MOTOR_PIN_REVERSE, OUTPUT);
  pinMode(MOTOR_PIN_PWM, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  
  // Initialize ASK Object
  rf_driver.init();
  
  // Setup Serial Monitor
  Serial.begin(9600);  
}

float percentile(int value, int lower, int upper, boolean reverse=false) 
{
  float percent = (float(value) - float(lower)) / (float(upper) - float(lower)) * 255;
  if (reverse) {
    percent = 255 - percent;
  }
  if (percent > float(255)) {
    return float(20);
  }
  if (percent < float(0)) {
    return float(0);
  }
  return percent;
}

void stop()
{
  digitalWrite(MOTOR_PIN_FORWARD, LOW);
  digitalWrite(MOTOR_PIN_REVERSE, LOW);
  analogWrite(MOTOR_PIN_PWM, 0);
  failSafeCounter = 0;
}

void forward()
{
  digitalWrite(MOTOR_PIN_FORWARD, HIGH);
  digitalWrite(MOTOR_PIN_REVERSE, LOW);
}

void reverse()
{
  digitalWrite(MOTOR_PIN_FORWARD, LOW);
  digitalWrite(MOTOR_PIN_REVERSE, HIGH);

}

void runMotor()
{
  failSafeCounter = 0;
  int speed = 0;
  if (throttleValue > 100) {
    speed = throttleValue - 100;
    forward(); 
  } else {
    speed = 100 - throttleValue;
    reverse();
  }

  if (speed > 80)
      analogWrite(MOTOR_PIN_PWM, int(speed * 2.55));
  else if (speed > 70)
      analogWrite(MOTOR_PIN_PWM, int(speed * 2.10));
  else if (speed > 60)
      analogWrite(MOTOR_PIN_PWM, int(speed * 1.50));
  else
      analogWrite(MOTOR_PIN_PWM, int(speed));
}

void processMessage(String message)
{
  Serial.println(message);
  throttleValue = 100;
  steeringValue = 100;

  for (int i = 0; i < message.length(); i++) {
    if (message.substring(i, i+1) == ":") {
      Serial.println("FOUND DELIMITER");
      throttleValue = message.substring(0, i).toInt();
      steeringValue = message.substring(i+1).toInt();
      break;
    }
  }

  // Which direction the motor turns
  moveClockwise = throttleValue > 100;

  // Should it move?
  if (throttleValue == 100) {
    Serial.println("Motor Should turn");
    stop();
  } else {
    runMotor();
  }

  Serial.println("TH: " + String(throttleValue) + ", ST: " + String(steeringValue));
}

void blink(int times = 5)
{
  for(int i = 0; i < times; i++) {
    digitalWrite(LEDPIN, LOW);
    delay(100);
    digitalWrite(LEDPIN, HIGH);
    delay(100);
  }
}

void loop()
{
  uint8_t buf[11];
  uint8_t buflen = sizeof(buf);
    
  // Check if received packet is correct size
  if (rf_driver.recv(buf, &buflen))
  {
    // Message received with valid checksum
    Serial.print("Message Received: ");
    processMessage(String((char*)buf));        
  } else {
    if (failSafeCounter > FAILSAFE_COUNTER) {
      blink();
      stop();
    }
    failSafeCounter = failSafeCounter + 1;
  }

  digitalWrite(LEDPIN, LOW);
  delay(10);
}
