#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <MPU6050.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MPU6050 mpu;

// Romantic messages for your crush (6-8 lines)
const char* loveMessages[] = {
  "Preethi\n = \npretty!",
  
  "I wish I'm\none reason\nyou smile",
  
  "You're way\ncuter\nthan me",
  
  "Are you C++?\nBecause\nyou're cute++",
  
  "My battery\ncharged up\nbecause of you",
  
  "Sorry if this\nseems quirky\nhehe",
};

const int numMessages = 8;

// Motion detection thresholds
const float SHAKE_THRESHOLD = 1.2;      // Strong shake (lowered for easier detection)
const float MOVEMENT_THRESHOLD = 0.8;   // Gentle movement
const unsigned long SHAKE_COOLDOWN = 3000; // 3 seconds between messages

// State tracking
unsigned long lastShakeTime = 0;
int currentMessageIndex = 0;
enum DisplayState { IDLE, HEART_EYES, WINKING, SHOWING_MESSAGE };
DisplayState currentState = IDLE;
unsigned long stateStartTime = 0;

// Eye animation parameters
const int leftX = 16;
const int rightX = 76;
const int eyeTopY = 18;
const int eyeW_neutral = 36;
const int eyeH_neutral = 26;
const int eyeH_happy = 18;
const int eyeH_blink = 4;
const int cornerR = 6;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting Love Display initialization...");
  Wire.begin(21, 22); // SDA=21, SCL=22
  
  // Initialize OLED
  if(!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SH1106 allocation failed"));
    for(;;);
  }
  Serial.println("OLED initialized!");
  
  // Initialize MPU6050
  mpu.initialize();
  if(!mpu.testConnection()) {
    Serial.println(F("MPU6050 connection failed!"));
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println(F("MPU6050 Error"));
    display.println(F("Check wiring!"));
    display.display();
    for(;;);
  }
  Serial.println("MPU6050 connected!");
  
  // Calibrate MPU6050 (keep device still)
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 25);
  display.println(F("Calibrating..."));
  display.println(F("Keep still!"));
  display.display();
  
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  delay(2000);
  
  // Welcome animation
  showWelcomeAnimation();
  
  stateStartTime = millis();
  Serial.println("Ready! Shake for messages, gentle movement for reactions!");
}

void loop() {
  // Read accelerometer
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  // Convert to g-force (approximate)
  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  
  // Calculate total acceleration magnitude (removing gravity baseline ~1g)
  float accelMagnitude = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
  float movement = abs(accelMagnitude - 1.0); // Subtract gravity
  
  unsigned long currentTime = millis();
  
  // Debug: Print movement value every 500ms
  static unsigned long lastDebugPrint = 0;
  if(currentTime - lastDebugPrint > 500) {
    Serial.print("Movement: ");
    Serial.println(movement, 3);
    lastDebugPrint = currentTime;
  }
  
  // SHAKE DETECTION - Show love message
  if(movement > SHAKE_THRESHOLD && (currentTime - lastShakeTime > SHAKE_COOLDOWN)) {
    Serial.print("SHAKE DETECTED! Movement value: ");
    Serial.println(movement);
    lastShakeTime = currentTime;
    showLoveMessage(currentMessageIndex);
    currentMessageIndex = (currentMessageIndex + 1) % numMessages;
    currentState = SHOWING_MESSAGE;
    stateStartTime = currentTime;
    
    // Wait 10 seconds, then smoothly transition back to eyes
    delay(10000);
    transitionToEyes();
    currentState = IDLE;
    stateStartTime = millis();
    return;
  }
  
  // GENTLE MOVEMENT - Show heart eyes or wink
  if(movement > MOVEMENT_THRESHOLD && movement < SHAKE_THRESHOLD) {
    if(currentState == IDLE && (currentTime - stateStartTime > 3000)) {
      // Randomly choose between heart eyes and wink
      if(random(0, 2) == 0) {
        Serial.println("Movement detected - Heart eyes!");
        currentState = HEART_EYES;
        showHeartEyes(true);
        delay(1500);
      } else {
        Serial.println("Movement detected - Wink!");
        currentState = WINKING;
        quickWink();
        delay(800);
      }
      currentState = IDLE;
      stateStartTime = currentTime;
    }
  }
  
  // IDLE STATE - Breathing eyes animation
  if(currentState == IDLE) {
    gentleBreathing();
  }
  
  delay(50);
}

void showWelcomeAnimation() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(25, 15);
  display.println(F("Hello"));
  display.setTextSize(1);
  display.setCursor(20, 45);
  display.println(F("Beautiful <3"));
  display.display();
  delay(2000);
}

void showLoveMessage(int index) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  
  // Draw small hearts in corners
  drawSmallHeart(5, 5, 4);
  drawSmallHeart(118, 5, 4);
  
  // Display message centered
  display.setCursor(8, 15);
  
  // Parse and display multi-line message
  String msg = String(loveMessages[index]);
  int yPos = 15;
  int startIdx = 0;
  
  for(unsigned int i = 0; i <= msg.length(); i++) {
    if(msg.charAt(i) == '\n' || i == msg.length()) {
      String line = msg.substring(startIdx, i);
      // Center the line
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(line, 0, 0, &x1, &y1, &w, &h);
      int xPos = (SCREEN_WIDTH - w) / 2;
      display.setCursor(xPos, yPos);
      display.println(line);
      yPos += 9;
      startIdx = i + 1;
    }
  }
  
  display.display();
}

// Draw a small decorative heart
void drawSmallHeart(int cx, int cy, int size) {
  int r = size / 2;
  display.fillCircle(cx - r/2, cy, r, SH110X_WHITE);
  display.fillCircle(cx + r/2, cy, r, SH110X_WHITE);
  display.fillTriangle(cx - r, cy, cx + r, cy, cx, cy + r + 2, SH110X_WHITE);
}

// Gentle breathing animation for idle state
void gentleBreathing() {
  static int breathPhase = 0;
  static unsigned long lastBreathUpdate = 0;
  
  if(millis() - lastBreathUpdate > 50) { // Faster update for smoother animation
    breathPhase = (breathPhase + 1) % 60; // More steps for smoother sine wave
    
    // Smooth sine wave for breathing
    float breathScale = sin(breathPhase * 3.14159 / 30.0);
    int eyeHeight = eyeH_neutral + (int)(breathScale * 4);
    
    int adjustedY = eyeTopY + ((eyeH_neutral - eyeHeight) / 2);
    
    display.clearDisplay();
    display.fillRoundRect(leftX, adjustedY, eyeW_neutral, eyeHeight, cornerR, SH110X_WHITE);
    display.fillRoundRect(rightX, adjustedY, eyeW_neutral, eyeHeight, cornerR, SH110X_WHITE);
    display.display();
    
    lastBreathUpdate = millis();
  }
}

// Quick wink animation
void quickWink() {
  int steps = 8; // More steps for smoother animation
  
  // Close right eye
  for(int s = 0; s <= steps; s++) {
    float t = (float)s / steps;
    int h = eyeH_neutral - (int)((eyeH_neutral - eyeH_blink) * t);
    display.clearDisplay();
    display.fillRoundRect(leftX, eyeTopY, eyeW_neutral, eyeH_neutral, cornerR, SH110X_WHITE);
    int rightY = eyeTopY + ((eyeH_neutral - h) / 2);
    display.fillRoundRect(rightX, rightY, eyeW_neutral, h, cornerR, SH110X_WHITE);
    display.display();
    delay(25);
  }
  
  delay(150);
  
  // Open right eye
  for(int s = 0; s <= steps; s++) {
    float t = (float)s / steps;
    int h = eyeH_blink + (int)((eyeH_neutral - eyeH_blink) * t);
    display.clearDisplay();
    display.fillRoundRect(leftX, eyeTopY, eyeW_neutral, eyeH_neutral, cornerR, SH110X_WHITE);
    int rightY = eyeTopY + ((eyeH_neutral - h) / 2);
    display.fillRoundRect(rightX, rightY, eyeW_neutral, h, cornerR, SH110X_WHITE);
    display.display();
    delay(25);
  }
}

// Draw full heart eye
void drawHeart(int cx, int cy, int size) {
  int r = size / 3;
  int topY = cy - r / 2;
  int leftLobeX = cx - r;
  int rightLobeX = cx + r;
  
  display.fillCircle(leftLobeX, topY, r, SH110X_WHITE);
  display.fillCircle(rightLobeX, topY, r, SH110X_WHITE);
  
  int triTopY = topY + r;
  int triLeftX = cx - (size / 2);
  int triRightX = cx + (size / 2);
  int triBottomY = topY + size;
  display.fillTriangle(triLeftX, triTopY, triRightX, triTopY, cx, triBottomY, SH110X_WHITE);
}

void showHeartEyes(bool animate) {
  const int heartCY = 30;
  const int heartLeftCX = leftX + eyeW_neutral / 2;
  const int heartRightCX = rightX + eyeW_neutral / 2;
  const int heartSize = 16;
  
  display.clearDisplay();
  drawHeart(heartLeftCX, heartCY, heartSize);
  drawHeart(heartRightCX, heartCY, heartSize);
  display.display();
  
  if(animate) {
    // Smoother pulse with multiple steps
    for(int p = 0; p < 2; p++) {
      for(int s = 0; s <= 4; s++) {
        float scale = 1.0 - (s * 0.025); // Shrink to 90%
        int size = (int)(heartSize * scale);
        display.clearDisplay();
        drawHeart(heartLeftCX, heartCY, size);
        drawHeart(heartRightCX, heartCY, size);
        display.display();
        delay(40);
      }
      for(int s = 4; s >= 0; s--) {
        float scale = 1.0 - (s * 0.025); // Grow back
        int size = (int)(heartSize * scale);
        display.clearDisplay();
        drawHeart(heartLeftCX, heartCY, size);
        drawHeart(heartRightCX, heartCY, size);
        display.display();
        delay(40);
      }
    }
  }
}

// Smooth transition from message back to eyes
void transitionToEyes() {
  // Fade effect: briefly clear, then show eyes growing in
  display.clearDisplay();
  display.display();
  delay(200);
  
  // Grow eyes from small to normal
  for(int s = 0; s <= 10; s++) {
    float t = (float)s / 10.0;
    int w = (int)(eyeW_neutral * t);
    int h = (int)(eyeH_neutral * t);
    int adjustedX_L = leftX + (eyeW_neutral - w) / 2;
    int adjustedX_R = rightX + (eyeW_neutral - w) / 2;
    int adjustedY = eyeTopY + (eyeH_neutral - h) / 2;
    
    display.clearDisplay();
    if(w > 0 && h > 0) {
      display.fillRoundRect(adjustedX_L, adjustedY, w, h, cornerR, SH110X_WHITE);
      display.fillRoundRect(adjustedX_R, adjustedY, w, h, cornerR, SH110X_WHITE);
    }
    display.display();
    delay(50);
  }
}