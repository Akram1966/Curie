#include <Wire.h>          // Must be included before Adafruit_GFX
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // IMPORTANT: Re-include for constants and the global 'display' object

// Define display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 

// --- ESP32 Specific Pin Definitions ---
const int OLED_SDA_PIN = 21; 
const int OLED_SCL_PIN = 22; 
const int SERVO_X_PIN = 13; 
const int SERVO_Y_PIN = 12; 

// GLOBAL DISPLAY OBJECT
TwoWire I2C_OLED = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, OLED_RESET);

// Include RoboEyes library
#include <FluxGarage_RoboEyes.h>

// RoboEyes instance
roboEyes robotEyes;

// Use the ESP32-specific servo library
#include <ESP32Servo.h>

// Servos
Servo servoX; 
Servo servoY; 

// --- Timing and Movement Parameters ---
// How long to wait between starting new movements
const unsigned long ACTION_INTERVAL = 5000; 
unsigned long lastActionTime = 0;

// Parameters for smooth servo movement
const int MIN_MOVEMENT_DURATION = 800;  // ms
const int MAX_MOVEMENT_DURATION = 2000; // ms

// State variables for managing the smooth movement
bool isMoving = false;
unsigned long movementStartTime;
unsigned long movementDuration;

int servoX_startAngle;
int servoX_targetAngle;
int servoY_startAngle;
int servoY_targetAngle;

// Enum for eye expressions (using the ones confirmed to work)
enum EyeExpression {
  EXPRESSION_IDLE,
  EXPRESSION_DEFAULT_MOOD,
  EXPRESSION_HAPPY_MOOD,
  EXPRESSION_ANGRY_MOOD,
  EXPRESSION_LAUGH_ANIM,
  NUM_EXPRESSIONS 
};

// Function to set a random eye expression
void setRandomExpression() {
  int randomExpression = random(NUM_EXPRESSIONS);

  robotEyes.setIdleMode(OFF);
  robotEyes.setMood(DEFAULT);

  switch (randomExpression) {
    case EXPRESSION_IDLE:
      Serial.println("Setting expression: IDLE");
      robotEyes.setIdleMode(ON, 2, 2);
      break;
    case EXPRESSION_DEFAULT_MOOD:
      Serial.println("Setting expression: DEFAULT_MOOD");
      robotEyes.setMood(DEFAULT);
      break;
    case EXPRESSION_HAPPY_MOOD:
      Serial.println("Setting expression: HAPPY_MOOD");
      robotEyes.setMood(HAPPY);
      break;
    case EXPRESSION_ANGRY_MOOD:
      Serial.println("Setting expression: ANGRY_MOOD");
      robotEyes.setMood(ANGRY);
      break;
    case EXPRESSION_LAUGH_ANIM:
      Serial.println("Setting expression: LAUGH_ANIM");
      robotEyes.setMood(HAPPY);
      robotEyes.anim_laugh(); 
      break;
  }
}

// Function to update the servo positions smoothly over time
void updateServoMovement() {
  // If we are not currently in a movement, check if it's time to start a new one.
  if (!isMoving) {
    if (millis() - lastActionTime >= ACTION_INTERVAL) {
      // It's time to start a new movement!
      isMoving = true;
      movementStartTime = millis();
      movementDuration = random(MIN_MOVEMENT_DURATION, MAX_MOVEMENT_DURATION);

      // The new start angle is whatever the previous target was
      servoX_startAngle = servoX_targetAngle;
      servoY_startAngle = servoY_targetAngle;

      // Pick a new random target angle
      servoX_targetAngle = random(30, 150);
      servoY_targetAngle = random(30, 150);

      Serial.printf("New movement! X: %d -> %d, Y: %d -> %d over %lu ms\n", 
                    servoX_startAngle, servoX_targetAngle, 
                    servoY_startAngle, servoY_targetAngle, movementDuration);

      // Also change the expression at the start of a new movement
      setRandomExpression();
    }
    return; // Nothing to do if not moving and not time to start
  }

  // --- If we are here, we are in the middle of a movement ---
  unsigned long elapsedTime = millis() - movementStartTime;

  if (elapsedTime >= movementDuration) {
    // Movement is complete
    servoX.write(servoX_targetAngle);
    servoY.write(servoY_targetAngle);
    isMoving = false;
    lastActionTime = millis(); // Reset the timer for the next idle period
    Serial.println("Movement complete.");
    return;
  }

  // Calculate movement progress as a fraction from 0.0 to 1.0
  float progress = (float)elapsedTime / (float)movementDuration;

  // --- The Easing Function (Ease In, Ease Out using a Cosine curve) ---
  // This formula transforms the linear progress into a smooth curve
  float easedProgress = (1.0 - cos(progress * PI)) / 2.0;

  // Calculate the new angle based on the eased progress
  int newAngleX = servoX_startAngle + (servoX_targetAngle - servoX_startAngle) * easedProgress;
  int newAngleY = servoY_startAngle + (servoY_targetAngle - servoY_startAngle) * easedProgress;

  // Write the new intermediate angle to the servos
  servoX.write(newAngleX);
  servoY.write(newAngleY);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- Starting RoboEyes ESP32 Companion (Smooth Movement Edition) ---");

  // OLED Initialization
  I2C_OLED.begin(OLED_SDA_PIN, OLED_SCL_PIN, 400000); 

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false)) { 
    Serial.println("❌ OLED not detected at 0x3C, trying 0x3D...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D, false)) {
      Serial.println("❌ OLED not detected at 0x3D either. Halting.");
      for (;;) delay(1000); 
    } else {
      Serial.println("✅ OLED connected and working at 0x3D!");
    }
  } else {
    Serial.println("✅ OLED connected and working at 0x3C!");
  }
  
  display.clearDisplay();
  display.display();

  // RoboEyes Setup
  robotEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF); 
  robotEyes.setAutoblinker(ON, 3, 2);
  robotEyes.setIdleMode(OFF);
  robotEyes.setWidth(36, 36);
  robotEyes.setHeight(36, 36);
  robotEyes.setBorderradius(8, 8);
  robotEyes.setSpacebetween(10);
  robotEyes.setMood(DEFAULT);

  // Servo Setup
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3); 
  servoX.setPeriodHertz(50); 
  servoY.setPeriodHertz(50); 
  servoX.attach(SERVO_X_PIN, 500, 2400); 
  servoY.attach(SERVO_Y_PIN, 500, 2400); 

  // Initialize servo positions
  servoX_targetAngle = 90;
  servoY_targetAngle = 90;
  servoX.write(servoX_targetAngle);
  servoY.write(servoY_targetAngle);

  // Initialize timing
  lastActionTime = millis();
}

void loop() {
  // Always update the eyes for animations and blinking
  robotEyes.update(); 
  
  // Constantly update the servo movement. This function handles all the logic.
  updateServoMovement();
}