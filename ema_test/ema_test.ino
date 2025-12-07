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

// EMA FILTER 
const float EMA_ALPHA = 0.25;
float ema_value = 0;
bool ema_initialized = false;

float emaFilter(float newSample) {
  if (!ema_initialized) {
    ema_value = newSample;
    ema_initialized = true;
    return ema_value;
  }
  
  ema_value = EMA_ALPHA * newSample + (1.0 - EMA_ALPHA) * ema_value;
  return ema_value;
}

// TEST PROTOCOL 
const int TEST_DISTANCES[] = {100, 320, 540, 760, 980, 1200};
const int NUM_DISTANCES = 6;
const int SAMPLES_PER_DISTANCE = 75;
const int COUNTDOWN_SECONDS = 15;

// data storage for stats
float rawData[6][75];
float filteredData[6][75];

int currentDistanceIndex = 0;
int sampleCount = 0;
bool testComplete = false;
unsigned long countdownStart = 0;
bool countdownActive = false;

void calculateAndDisplayStats() {
  Serial.println("\n\n========================================");
  Serial.println("      EMA TEST - FINAL RESULTS");
  Serial.println("  Filter: Exponential MA (alpha=0.25)");
  Serial.println("========================================\n");
  
  Serial.println("=== RAW DATA (Before Filter) ===");
  Serial.println("Target(mm) | Mean(mm) | StdDev(mm) | Min(mm) | Max(mm)");
  Serial.println("-----------|----------|------------|---------|--------");
  
  for (int i = 0; i < NUM_DISTANCES; i++) {
    float sum = 0, minVal = 9999, maxVal = 0;
    for (int j = 0; j < SAMPLES_PER_DISTANCE; j++) {
      float val = rawData[i][j];
      sum += val;
      if (val < minVal) minVal = val;
      if (val > maxVal) maxVal = val;
    }
    float mean = sum / SAMPLES_PER_DISTANCE;
    
    float sqDiffSum = 0;
    for (int j = 0; j < SAMPLES_PER_DISTANCE; j++) {
      float diff = rawData[i][j] - mean;
      sqDiffSum += diff * diff;
    }
    float stdDev = sqrt(sqDiffSum / SAMPLES_PER_DISTANCE);
    
    char buf[100];
    sprintf(buf, "%10d | %8.1f | %10.1f | %7.0f | %7.0f", 
            TEST_DISTANCES[i], mean, stdDev, minVal, maxVal);
    Serial.println(buf);
  }
  
  Serial.println("\n=== FILTERED DATA (After EMA Filter) ===");
  Serial.println("Target(mm) | Mean(mm) | StdDev(mm) | Min(mm) | Max(mm) | Reduction(%)");
  Serial.println("-----------|----------|------------|---------|---------|-------------");
  
  float totalRawStdDev = 0;
  float totalFilteredStdDev = 0;
  
  for (int i = 0; i < NUM_DISTANCES; i++) {
    float sum = 0, minVal = 9999, maxVal = 0;
    for (int j = 0; j < SAMPLES_PER_DISTANCE; j++) {
      float val = filteredData[i][j];
      sum += val;
      if (val < minVal) minVal = val;
      if (val > maxVal) maxVal = val;
    }
    float mean = sum / SAMPLES_PER_DISTANCE;
    
    float sqDiffSum = 0;
    for (int j = 0; j < SAMPLES_PER_DISTANCE; j++) {
      float diff = filteredData[i][j] - mean;
      sqDiffSum += diff * diff;
    }
    float stdDev = sqrt(sqDiffSum / SAMPLES_PER_DISTANCE);
    
    // calc reduction in variance
    float rawStdDev = 0;
    float rawSum = 0;
    for (int j = 0; j < SAMPLES_PER_DISTANCE; j++) {
      rawSum += rawData[i][j];
    }
    float rawMean = rawSum / SAMPLES_PER_DISTANCE;
    float rawSqDiffSum = 0;
    for (int j = 0; j < SAMPLES_PER_DISTANCE; j++) {
      float diff = rawData[i][j] - rawMean;
      rawSqDiffSum += diff * diff;
    }
    rawStdDev = sqrt(rawSqDiffSum / SAMPLES_PER_DISTANCE);
    
    float reduction = ((rawStdDev - stdDev) / rawStdDev) * 100;
    
    totalRawStdDev += rawStdDev;
    totalFilteredStdDev += stdDev;
    
    char buf[120];
    sprintf(buf, "%10d | %8.1f | %10.1f | %7.0f | %7.0f | %11.1f%%", 
            TEST_DISTANCES[i], mean, stdDev, minVal, maxVal, reduction);
    Serial.println(buf);
  }
  
  float avgReduction = ((totalRawStdDev - totalFilteredStdDev) / totalRawStdDev) * 100;
  
  Serial.println("\n========================================");
  Serial.print("AVERAGE VARIANCE REDUCTION: ");
  Serial.print(avgReduction, 1);
  Serial.println("%");
  Serial.println("========================================");
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
  lcd.print("EMA FILTER TEST");
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
  
  Serial.println("\n=== EMA FILTER TEST STARTING ===");
  Serial.println("Filter: alpha=0.25\n");
  
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
  
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);
  
  if (measure.RangeStatus != 4) {
    int dist_raw = measure.RangeMilliMeter;
    float dist_filtered = emaFilter((float)dist_raw);
    
    rawData[currentDistanceIndex][sampleCount] = (float)dist_raw;
    filteredData[currentDistanceIndex][sampleCount] = dist_filtered;
    
    int dist_mm = (int)dist_filtered;
    
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
        ema_initialized = false;
        ema_value = 0;
        
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