#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Servo.h>

// Blynk Configuration
#define BLYNK_TEMPLATE_ID "TMPL6VXacET4h"
#define BLYNK_TEMPLATE_NAME "LibroBot"
#define BLYNK_AUTH_TOKEN "ybteh_v-TAIWmJrvCi2oSPrj2Yg0butB"
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "wifi_s";
char pass[] = "12345678";

// Motor Pins
#define IN1 13
#define IN2 12
#define IN3 14
#define IN4 27

// Sensor Pins
#define TRIG_PIN 16
#define ECHO_PIN 17
#define SERVO_PIN 5

// Constants
#define OBSTACLE_DISTANCE 20  
#define TURN_DELAY 1000       
#define SCAN_DELAY 500        

Servo servo;
int currentPath = 0;
bool obstacleDetected = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize Motor Driver
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();
  
  // Initialize Sensors
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  servo.attach(SERVO_PIN);
  servo.write(90);  // Center position
  
  // Connect to Blynk
  Blynk.begin(auth, ssid, pass);
  Serial.println("System Initialized");
}

void loop() {
  Blynk.run();
  
  if (currentPath != 0) {
    executePath(currentPath);
    currentPath = 0; // Reset after completion
    Blynk.virtualWrite(V0, 0); // Reset Blynk button
  }
}

// Blynk Path Selection Handler (V0)
BLYNK_WRITE(V0) {
  currentPath = param.asInt();
  Serial.print("Executing Path: ");
  Serial.println(currentPath);
}

// Path Execution Function
void executePath(int path) {
  switch (path) {
    case 1: // Forward 5s → Left → Forward 2s
      moveForward(5000);
      turnLeft();
      moveForward(2000);
      break;
      
    case 2: // Forward 3s → Right → Forward 4s → Left → Forward 2s
      moveForward(3000);
      turnRight();
      moveForward(4000);
      turnLeft();
      moveForward(2000);
      break;
      
    case 3: // Forward 2s → Right → Forward 3s → Right → Forward 3s
      moveForward(2000);
      turnRight();
      moveForward(3000);
      turnRight();
      moveForward(3000);
      break;
      
    case 4: // Forward 6s → Left → Forward 1s → Right → Forward 2s
      moveForward(6000);
      turnLeft();
      moveForward(1000);
      turnRight();
      moveForward(2000);
      break;
      
    case 5: // Square pattern (Forward 2s → Right) x4
      for(int i = 0; i < 4; i++) {
        moveForward(2000);
        turnRight();
      }
      break;
      
    case 6: // Complex path with multiple turns
      moveForward(4000);
      turnLeft();
      moveForward(1500);
      turnRight();
      moveForward(3000);
      turnLeft();
      moveForward(2000);
      turnRight();
      moveForward(1000);
      break;
      
    default:
      Serial.println("Invalid path selected");
      break;
  }
}

// Movement Functions
void moveForward(int duration) {
  unsigned long startTime = millis();
  
  while (millis() - startTime < duration) {
    if (checkObstacle()) {
      avoidObstacle();
      startTime += millis() - startTime; // Adjust timing
    }
    
    // Move forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    
    // Send distance data to Blynk
    Blynk.virtualWrite(V1, getDistance());
    delay(100); // Short delay to prevent Blynk timeout
  }
  stopMotors();
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(TURN_DELAY);
  stopMotors();
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(TURN_DELAY);
  stopMotors();
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// Obstacle Handling Functions
bool checkObstacle() {
  int distance = getDistance();
  Blynk.virtualWrite(V1, distance); // Update Blynk
  return (distance < OBSTACLE_DISTANCE && distance > 0);
}

void avoidObstacle() {
  stopMotors();
  Blynk.virtualWrite(V2, "Obstacle Detected!"); // Blynk notification
  
  // Scan directions
  int leftDist = scanDirection(0);   // Left scan (0°)
  int rightDist = scanDirection(180); // Right scan (180°)
  
  // Decide turn direction
  if (leftDist > rightDist) {
    turnLeft();
  } else {
    turnRight();
  }
  
  servo.write(90); // Return to center
  delay(SCAN_DELAY);
  Blynk.virtualWrite(V2, "Resuming Path"); // Clear notification
}

int scanDirection(int angle) {
  servo.write(angle);
  delay(SCAN_DELAY);
  return getDistance();
}

// Distance Measurement Function
int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2; // Convert to cm
}