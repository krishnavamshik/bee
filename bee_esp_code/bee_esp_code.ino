#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting OLED initialization...");
  Wire.begin(21, 22); // SDA=21, SCL=22
  
  // Try to initialize display
  if(!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SH1106 allocation failed"));
    Serial.println(F("Check wiring:"));
    Serial.println(F("SDA -> GPIO 21"));
    Serial.println(F("SCL -> GPIO 22"));
    Serial.println(F("VCC -> 3.3V"));
    Serial.println(F("GND -> GND"));
    for(;;);
  }
  
  Serial.println("OLED initialized successfully!");
  
  // Clear the buffer completely
  display.clearDisplay();
  display.display();
  delay(100);
  
  // Now draw Hello World
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 25);
  display.println(F("Hello"));
  display.setCursor(10, 45);
  display.println(F("World!"));
  display.display();
  
  delay(2000);
  
  Serial.println("\n=== ESP32 OLED Menu ===");
  Serial.println("1 - Hello World");
  Serial.println("2 - Eye Animation");
  Serial.println("3 - Custom Message");
  Serial.println("Enter your choice:");
}

void loop() {
  if (Serial.available() > 0) {
    char choice = Serial.read();
    
    switch(choice) {
      case '1':
        Serial.println("Displaying: Hello World");
        showHelloWorld();
        break;
        
      case '2':
        Serial.println("Playing: Eye Animation");
        eyeAnimation();
        break;
        
      case '3':
        {
          Serial.println("Enter your message:");
          delay(100);
          while(Serial.available()) Serial.read(); // Clear buffer
          
          while(!Serial.available()) {
            delay(10);
          }
          
          String msg = Serial.readStringUntil('\n');
          msg.trim();
          Serial.print("Displaying: ");
          Serial.println(msg);
          showCustomMessage(msg);
        }
        break;
        
      case '\n':
      case '\r':
        // Ignore newline characters
        break;
        
      default:
        Serial.println("Invalid choice! Enter 1, 2, or 3");
        break;
    }
  }
}

void showHelloWorld() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 25);
  display.println(F("Hello"));
  display.setCursor(10, 45);
  display.println(F("World!"));
  display.display();
}

void drawFilledEyes(int lx, int rx, int y, int w, int h, int r) {
  display.clearDisplay();
  display.fillRoundRect(lx, y, w, h, r, SH110X_WHITE);
  display.fillRoundRect(rx, y, w, h, r, SH110X_WHITE);
  display.display();
}

// Helper: smooth interpolate between two integers
int lerpInt(int a, int b, float t) {
  return a + (int)((b - a) * t + 0.5);
}

// Smooth transition between two eye sizes over `steps` frames
void animateEyeTransition(int lx, int rx, int y, int fromW, int fromH, int toW, int toH, int r, int steps, int stepDelay) {
  for (int s = 1; s <= steps; s++) {
    float t = (float)s / (float)steps;
    int curW = lerpInt(fromW, toW, t);
    int curH = lerpInt(fromH, toH, t);
    // center eyes vertically around the original y
    int curY = y + ( (fromH - curH) / 2 );
    drawFilledEyes(lx, rx, curY, curW, curH, r);
    delay(stepDelay);
  }
  // ensure final exact size
  drawFilledEyes(lx, rx, y + ((fromH - toH)/2), toW, toH, r);
}

// Wink: right eye closes quickly while left stays open
void animateWink(int lx, int rx, int y, int openW, int openH, int closedH, int r) {
  int steps = 6;
  int stepDelay = 30;
  // close right eye
  for (int s = 1; s <= steps; s++) {
    float t = (float)s / steps;
    int curRH = lerpInt(openH, closedH, t);
    int rightY = y + ((openH - curRH) / 2);
    display.clearDisplay();
    // Left eye open
    display.fillRoundRect(lx, y, openW, openH, r, SH110X_WHITE);
    // Right eye partially closed
    display.fillRoundRect(rx, rightY, openW, curRH, r, SH110X_WHITE);
    display.display();
    delay(stepDelay);
  }

  // small hold when winked
  delay(220);

  // open right eye back
  for (int s = 1; s <= steps; s++) {
    float t = (float)s / steps;
    int curRH = lerpInt(closedH, openH, t);
    int rightY = y + ((openH - curRH) / 2);
    display.clearDisplay();
    display.fillRoundRect(lx, y, openW, openH, r, SH110X_WHITE);
    display.fillRoundRect(rx, rightY, openW, curRH, r, SH110X_WHITE);
    display.display();
    delay(stepDelay);
  }
  // ensure open
  drawFilledEyes(lx, rx, y, openW, openH, r);
}

// Draw a solid heart centered at (cx, cy) with approximate `size`
// Uses two filled circles (lobes) and a filled triangle for the lower part.
// size should be ~12-18 for good look on 128x64.
void drawHeart(int cx, int cy, int size) {
  // size -> radius for lobes and triangle height
  int r = size / 3;          // radius for the two top lobes
  int lobeOffset = r;       // horizontal offset of second lobe
  int topY = cy - r / 2;    // vertical position of lobes
  int leftLobeX = cx - r;   // left lobe center
  int rightLobeX = cx + r;  // right lobe center

  // draw two lobes
  display.fillCircle(leftLobeX, topY, r, SH110X_WHITE);
  display.fillCircle(rightLobeX, topY, r, SH110X_WHITE);

  // draw the lower triangle - three points form the V bottom
  int triTopX = cx;
  int triTopY = topY + r;                   // just under the lobes
  int triLeftX = cx - (size / 2);
  int triRightX = cx + (size / 2);
  int triBottomX = cx;
  int triBottomY = topY + size;             // depth of triangle
  display.fillTriangle(triLeftX, triTopY, triRightX, triTopY, triBottomX, triBottomY, SH110X_WHITE);
}

// Draw both hearts as eyes with an optional pulsing twinkle
void drawHeartEyes(int leftCX, int rightCX, int cy, int baseSize, bool twinkle) {
  display.clearDisplay();

  // base hearts
  drawHeart(leftCX, cy, baseSize);
  drawHeart(rightCX, cy, baseSize);
  display.display();

  if (twinkle) {
    // tiny pulse: shrink then grow to create twinkle - quick and subtle
    for (int p = 0; p < 2; p++) {
      delay(120);
      display.clearDisplay();
      drawHeart(leftCX, cy, baseSize - 2);
      drawHeart(rightCX, cy, baseSize - 2);
      display.display();
      delay(100);
      display.clearDisplay();
      drawHeart(leftCX, cy, baseSize);
      drawHeart(rightCX, cy, baseSize);
      display.display();
      delay(80);
    }
  }
}

// Main enhanced animation sequence (replace your previous eyeAnimation)
void eyeAnimation() {
  // positions & sizes tuned for 128x64
  const int leftX = 16;      // left eye top-left X
  const int rightX = 76;     // right eye top-left X
  const int eyeTopY = 18;    // top Y for eyes
  const int eyeW_neutral = 36;
  const int eyeH_neutral = 26;
  const int eyeH_happy = 18;
  const int eyeH_blink = 4;  // closed-thin-bar
  const int cornerR = 6;

  // heart positions (centers)
  const int heartCY = 30;
  const int heartLeftCX = leftX + eyeW_neutral / 2;
  const int heartRightCX = rightX + eyeW_neutral / 2;
  const int heartSize = 16;

  int cycles = 2;

  for (int c = 0; c < cycles; c++) {
    // 1) Smooth to neutral (in case starting from elsewhere)
    animateEyeTransition(leftX, rightX, eyeTopY, eyeW_neutral, eyeH_happy, eyeW_neutral, eyeH_neutral, cornerR, 8, 25);
    delay(700);

    // 2) Blink (quick close & open)
    animateEyeTransition(leftX, rightX, eyeTopY, eyeW_neutral, eyeH_neutral, eyeW_neutral, eyeH_blink, cornerR, 6, 25);
    animateEyeTransition(leftX, rightX, eyeTopY, eyeW_neutral, eyeH_blink, eyeW_neutral, eyeH_neutral, cornerR, 6, 25);
    delay(200);

    // 3) Small breathing idle (subtle expand/shrink)
    for (int b = 0; b < 2; b++) {
      animateEyeTransition(leftX, rightX, eyeTopY, eyeW_neutral, eyeH_neutral, eyeW_neutral, eyeH_neutral - 3, cornerR, 8, 30);
      animateEyeTransition(leftX, rightX, eyeTopY, eyeW_neutral, eyeH_neutral - 3, eyeW_neutral, eyeH_neutral, cornerR, 8, 30);
    }

    // 4) Wink in the middle
    animateWink(leftX, rightX, eyeTopY, eyeW_neutral, eyeH_neutral, eyeH_blink, cornerR);
    delay(400);

    // 5) Happy (slightly shorter, friendly)
    animateEyeTransition(leftX, rightX, eyeTopY, eyeW_neutral, eyeH_neutral, eyeW_neutral, eyeH_happy, cornerR, 10, 28);
    delay(700);

    // 6) Love: hearts twinkle gently before holding
    drawHeartEyes(heartLeftCX, heartRightCX, heartCY, heartSize, true);
    delay(900);

    // 7) Smooth return to neutral
    animateEyeTransition(leftX, rightX, eyeTopY, eyeW_neutral, eyeH_happy, eyeW_neutral, eyeH_neutral, cornerR, 10, 28);
    delay(700);
  }

  // End frame text (optional)
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(28, 28);
  display.println(F("Animation Done"));
  display.display();
  delay(1200);
}

void showCustomMessage(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  
  // Word wrap for long messages
  int lineHeight = 10;
  int currentY = 0;
  int maxWidth = SCREEN_WIDTH;
  
  String word = "";
  int cursorX = 0;
  
  for(unsigned int i = 0; i < message.length(); i++) {
    char c = message.charAt(i);
    
    if(c == ' ' || i == message.length() - 1) {
      if(i == message.length() - 1 && c != ' ') {
        word += c;
      }
      
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(word, 0, 0, &x1, &y1, &w, &h);
      
      if(cursorX + w > maxWidth) {
        currentY += lineHeight;
        cursorX = 0;
      }
      
      if(currentY >= SCREEN_HEIGHT) break;
      
      display.setCursor(cursorX, currentY);
      display.print(word);
      cursorX += w;
      
      if(c == ' ') {
        display.print(" ");
        cursorX += 6;
      }
      
      word = "";
    } else {
      word += c;
    }
  }
  
  display.display();
}