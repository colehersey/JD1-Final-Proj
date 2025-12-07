#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_VL53L0X.h>

// LCD SETUP
LiquidCrystal lcd(14, 27, 26, 25, 33, 32);

const int I2C_SDA = 21;
const int I2C_SCL = 22;

Adafruit_VL53L0X lox;

//LED SETUP
const int LED_PIN = 5;
const int RANGE_MIN = 100;
const int RANGE_MAX = 1200;

//TEST PROTOCOL
const int TEST_DISTANCES[] = {100, 320, 540, 760, 980, 1200};
const int NUM_DISTANCES = 6;
const int SAMPLES_PER_DISTANCE = 75;
const int COUNTDOWN_SECONDS = 15;

// create the storage for the stats
float distanceData[6][75];  // [distance_index][sample]

int currentDistanceIndex = 0;
int sampleCount = 0;
bool testComplete = false;
unsigned long countdownStart = 0;
bool countdownActive = false;

void calculateAndDisplayStats() {
  Serial.println("\n\n========================================");
  Serial.println("   RAW DATA TEST - FINAL RESULTS");
  Serial.println("   Filter: NONE (Raw sensor data)");
  Serial.println("========================================\n");
  
  Serial.println("Target(mm) | Mean(mm) | StdDev(mm) | Min(mm) | Max(mm) | Range(mm)");
  Serial.println("-----------|----------|------------|---------|---------|----------");
  
  for (int i = 0; i < NUM_DISTANCES; i++) {
    float sum = 0;
    float minVal = 9999;
    float maxVal = 0;
    
    // calc mean and max
    for (int j = 0; j < SAMPLES_PER_DISTANCE; j++) {
      float val = distanceData[i][j];
      sum += val;
      if (val < minVal) minVal = val;
      if (val > maxVal) maxVal = val;
    }
    float mean = sum / SAMPLES_PER_DISTANCE;
    
    // calc the standard deviation
    float sqDiffSum = 0;
    for (int j = 0; j < SAMPLES_PER_DISTANCE; j++) {
      float diff = distanceData[i][j] - mean;
      sqDiffSum += diff * diff;
    }
    float stdDev = sqrt(sqDiffSum / SAMPLES_PER_DISTANCE);
    float range = maxVal - minVal;
    
    // format the rows nicely for screenshots
    char buf[100];
    sprintf(buf, "%10d | %8.1f | %10.1f | %7.0f | %7.0f | %9.0f", 
            TEST_DISTANCES[i], mean, stdDev, minVal, maxVal, range);
    Serial.println(buf);
  }
  
  Serial.println("\n========================================");
  Serial.println("SCREENSHOT THIS TABLE FOR YOUR ANALYSIS");
  Serial.println("========================================\n");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RAW DATA TEST");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  if (!lox.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor ERROR");
    Serial.println("ERROR: VL53L0X not detected");
    while (true) delay(200);
  }
  
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);
  
  delay(2000);
  
  Serial.println("\n=== RAW DATA TEST STARTING ===");
  Serial.println("Collecting data at 6 distances...\n");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Position target");
  lcd.setCursor(0, 1);
  lcd.print("at 100mm");
  
  countdownStart = millis();
  countdownActive = true;
}

void loop() {
  if (testComplete) {
    calculateAndDisplayStats();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TEST COMPLETE!");
    lcd.setCursor(0, 1);
    lcd.print("Check Serial");
    
    while(true) delay(1000);
  }
  
  // countdown phase
  if (countdownActive) {
    unsigned long elapsed = (millis() - countdownStart) / 1000;
    int remaining = COUNTDOWN_SECONDS - elapsed;
    
    if (remaining > 0) {
      lcd.setCursor(0, 0);
      lcd.print("Get ready: ");
      lcd.print(remaining);
      lcd.print("s  ");
      delay(1000);
      return;
    } else {
      countdownActive = false;
      sampleCount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Collecting...");
      Serial.print("Distance ");
      Serial.print(TEST_DISTANCES[currentDistanceIndex]);
      Serial.println("mm - Collecting samples...");
    }
  }
  
  // data collection
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);
  
  if (measure.RangeStatus != 4) {
    int dist_mm = measure.RangeMilliMeter;
    
    distanceData[currentDistanceIndex][sampleCount] = (float)dist_mm;
    
    // calc the led for gamma correction
    int led_pwm = 0;
    if (dist_mm >= RANGE_MIN && dist_mm <= RANGE_MAX) {
      int linear_brightness = map(dist_mm, RANGE_MIN, RANGE_MAX, 255, 0);
      float normalized = linear_brightness / 255.0;
      float gamma_corrected = pow(normalized, 2.2);
      led_pwm = (int)(gamma_corrected * 255);
      led_pwm = constrain(led_pwm, 0, 255);
      analogWrite(LED_PIN, led_pwm);
    } else {
      analogWrite(LED_PIN, 0);
    }
    
    // reupdate the LCD
    lcd.setCursor(0, 1);
    lcd.print("Sample ");
    lcd.print(sampleCount + 1);
    lcd.print("/");
    lcd.print(SAMPLES_PER_DISTANCE);
    lcd.print("  ");
    
    sampleCount++;
    
    if (sampleCount >= SAMPLES_PER_DISTANCE) {
      Serial.print("  Completed ");
      Serial.print(SAMPLES_PER_DISTANCE);
      Serial.println(" samples\n");
      
      currentDistanceIndex++;
      
      if (currentDistanceIndex >= NUM_DISTANCES) {
        testComplete = true;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Move to ");
        lcd.print(TEST_DISTANCES[currentDistanceIndex]);
        lcd.print("mm");
        
        countdownStart = millis();
        countdownActive = true;
      }
    }
  }
  
  delay(100);
}