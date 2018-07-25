/*
  https://dronebotworkshop.com
*/

#include <RH_ASK.h>
#include <SoftwareServo.h>
#include <SPI.h> 

// Create Amplitude Shift Keying Object
#define LEDPIN 13
#define TX_PIN 2
#define RX_PIN 3
#define MOTOR_PIN_FORWARD 11
#define MOTOR_PIN_REVERSE 12
#define MOTOR_PIN_PWM 5
#define STEERING_PIN_PWM 6
#define FAILSAFE_COUNTER 100

RH_ASK rf_driver(4000, RX_PIN, TX_PIN);
SoftwareServo servo;

int failSafeCounter = 0;
int throttleValue = 100;
int steeringValue = 100;

void setup()
{
  pinMode(MOTOR_PIN_FORWARD, OUTPUT);
  pinMode(MOTOR_PIN_REVERSE, OUTPUT);
  pinMode(MOTOR_PIN_PWM, OUTPUT);
  pinMode(STEERING_PIN_PWM, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  
  // Initialize ASK Object
  rf_driver.init();

  // Servo
  servo.attach(STEERING_PIN_PWM);
  
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
  servo.write(90);
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
      analogWrite(MOTOR_PIN_PWM, int(speed * 1.80));
  else
      analogWrite(MOTOR_PIN_PWM, int(speed * 1.5));
}

void runSteering()
{
  failSafeCounter = 0;
  if (steeringValue == 100) {
    Serial.print(", S: CENTER");
    servo.write(90);
    return;
  }

  int steering = int((steeringValue / 2) * 1.8);
  Serial.print(", S: " + String(steering));
  servo.write(steering);
}

void processMessage(String message)
{
  throttleValue = 100;
  steeringValue = 100;

  for (int i = 0; i < message.length(); i++) {
    if (message.substring(i, i+1) == ":") {
      throttleValue = message.substring(0, i).toInt();
      steeringValue = message.substring(i+1).toInt();
      break;
    }
  }

  // Should it move?
  if (throttleValue == 100) {
    Serial.print(", MOTOR: Stopped");
    stop();
  } else {
    Serial.print(", MOTOR: Running");
    runMotor();
  }

  runSteering();
  Serial.print(", TH: " + String(throttleValue) + ", ST: " + String(steeringValue));
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
    Serial.print("MSG: " + String((char*)buf));
    processMessage(String((char*)buf));  
    Serial.println("");
  } else {
    if (failSafeCounter > FAILSAFE_COUNTER) {
      blink();
      stop();
    }
    failSafeCounter = failSafeCounter + 1;
  }

  delay(10);
  SoftwareServo::refresh();
}
