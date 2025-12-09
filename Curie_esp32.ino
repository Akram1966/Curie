// --- PREREQUISITE LIBRARIES ---
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// --- HARDWARE AND SCREEN DEFINITIONS ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
const int OLED_SDA_PIN = 21; 
const int OLED_SCL_PIN = 22; 
const int SERVO_X_PIN = 12; 
const int SERVO_Y_PIN = 13; 
const int TOUCH_SENSOR_PIN = 18; // GPIO 18 (D18)

// --- STEP 1: CREATE THE GLOBAL DISPLAY OBJECT ---
// CORRECTED LINE: The object is now correctly named I2C_OLED
TwoWire I2C_OLED = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, OLED_RESET);

// --- STEP 2: INCLUDE THE ROBOEYES LIBRARY ---
#include <FluxGarage_RoboEyes.h>

// --- STEP 3: CREATE OTHER GLOBAL OBJECTS ---
roboEyes robotEyes;
Servo servoX; 
Servo servoY; 

// --- STATE MACHINE FOR LOCAL MOOD ---
enum RobotMood { MOOD_NEUTRAL, MOOD_HAPPY, MOOD_ANGRY };
RobotMood currentMood = MOOD_NEUTRAL;

// --- Timers for Mood Changes ---
const unsigned long timeUntilAngry = 5000;  // 5 seconds
const unsigned long happyDuration = 3000;   // Stay happy for 3 seconds
unsigned long lastTouchTime = 0;
unsigned long happyStartTime = 0;
int lastTouchState = LOW;

// --- Variables for LLM "Talking" Animation ---
bool isTalking = false;
unsigned long lastTalkMoveTime = 0;
int talkMoveInterval = 150; 
int currentYAngle = 90;
int talkBobAmount = 5;

// --- State variables for Smooth Servo Movement ---
bool isMoving = false;
unsigned long movementStartTime;
unsigned long movementDuration;
const int MIN_MOVEMENT_DURATION = 1000; // ms
const int MAX_MOVEMENT_DURATION = 2500; // ms
int servoX_startAngle;
int servoX_targetAngle;
int servoY_startAngle;
int servoY_targetAngle;

void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_SENSOR_PIN, INPUT);

  // --- OLED, RoboEyes, and Servo setup ---
  I2C_OLED.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  robotEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF);
  robotEyes.setAutoblinker(ON, 3, 2);
  robotEyes.setWidth(36, 36);
  robotEyes.setHeight(36, 36);
  robotEyes.setBorderradius(8, 8);
  robotEyes.setSpacebetween(10);
  
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servoX.setPeriodHertz(50); 
  servoY.setPeriodHertz(50); 
  servoX.attach(SERVO_X_PIN, 500, 2400); 
  servoY.attach(SERVO_Y_PIN, 500, 2400); 

  // Initialize servo positions for smooth movement
  servoX_targetAngle = 90;
  servoY_targetAngle = 90;
  servoX.write(servoX_targetAngle);
  servoY.write(servoY_targetAngle);

  lastTouchTime = millis();
  currentMood = MOOD_NEUTRAL;
  robotEyes.setMood(DEFAULT);
  Serial.println("Curie is online. Moving and waiting for interaction...");
}

// Handles commands from the Python "Brain" script
void handleSerialCommand(String cmd) {
  cmd.trim();
  if (cmd.startsWith("MOOD:")) {
    String moodStr = cmd.substring(5);
    lastTouchTime = millis(); 
    if (moodStr == "HAPPY") {
      robotEyes.setMood(HAPPY);
      currentMood = MOOD_HAPPY;
      happyStartTime = millis();
    } else if (moodStr == "ANGRY") {
      robotEyes.setMood(ANGRY);
      currentMood = MOOD_ANGRY;
    } else {
      robotEyes.setMood(DEFAULT);
      currentMood = MOOD_NEUTRAL;
    }
  } else if (cmd.startsWith("TALK:")) {
    String talkCmd = cmd.substring(5);
    if (talkCmd == "START") {
      isTalking = true;
      isMoving = false; // Stop random movement to start talking
      currentYAngle = servoY.read();
    } else if (talkCmd == "STOP") {
      isTalking = false;
    }
  }
}

// Manages Curie's mood based on touch and time
void updateMood() {
  int currentTouchState = digitalRead(TOUCH_SENSOR_PIN);
  if (currentTouchState == HIGH && lastTouchState == LOW) {
    currentMood = MOOD_HAPPY;
    robotEyes.setMood(HAPPY);
    lastTouchTime = millis();
    happyStartTime = millis();
  }
  lastTouchState = currentTouchState;

  switch (currentMood) {
    case MOOD_HAPPY:
      if (millis() - happyStartTime > happyDuration) {
        currentMood = MOOD_NEUTRAL;
        robotEyes.setMood(DEFAULT);
      }
      break;
    case MOOD_NEUTRAL:
      if (millis() - lastTouchTime > timeUntilAngry) {
        Serial.println("No interaction. Becoming angry!");
        currentMood = MOOD_ANGRY;
        robotEyes.setMood(ANGRY);
      }
      break;
    case MOOD_ANGRY:
      // Stays angry until touched.
      break;
  }
}

// Manages smooth, random head movements when not talking
void updateServoMovement() {
  if (!isMoving) {
    // If not moving, start a new movement
    isMoving = true;
    movementStartTime = millis();
    movementDuration = random(MIN_MOVEMENT_DURATION, MAX_MOVEMENT_DURATION);
    servoX_startAngle = servoX_targetAngle;
    servoY_startAngle = servoY_targetAngle;
    servoX_targetAngle = random(45, 135);
    servoY_targetAngle = random(70, 110);
    return;
  }

  unsigned long elapsedTime = millis() - movementStartTime;
  if (elapsedTime >= movementDuration) {
    // Movement is complete
    servoX.write(servoX_targetAngle);
    servoY.write(servoY_targetAngle);
    isMoving = false; // Get ready to start a new movement on the next cycle
    return;
  }

  float progress = (float)elapsedTime / (float)movementDuration;
  // The Easing Function (Ease In, Ease Out using a Cosine curve)
  float easedProgress = (1.0 - cos(progress * PI)) / 2.0;
  int newAngleX = servoX_startAngle + (servoX_targetAngle - servoX_startAngle) * easedProgress;
  int newAngleY = servoY_startAngle + (servoY_targetAngle - servoY_startAngle) * easedProgress;
  servoX.write(newAngleX);
  servoY.write(newAngleY);
}

void loop() {
  robotEyes.update(); // Update eye animations

  if (Serial.available()) {
    handleSerialCommand(Serial.readStringUntil('\n')); // Check for LLM commands
  }

  updateMood(); // Update mood based on touch

  // --- MOVEMENT LOGIC ---
  if (isTalking) {
    // If talking, do the head-bob animation
    if (millis() - lastTalkMoveTime > talkMoveInterval) {
      lastTalkMoveTime = millis();
      int randomX = random(80, 100);
      int randomY = random(currentYAngle - talkBobAmount, currentYAngle + talkBobAmount);
      servoX.write(randomX);
      servoY.write(randomY);
    }
  } else {
    // If NOT talking, perform the smooth random "looking around" movement
    updateServoMovement();
  }
}
