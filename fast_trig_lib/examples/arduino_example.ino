// arduino_example.ino - FastTrig Library Arduino Example
// This example shows how to use FastTrig on Arduino platforms

// Note: Copy fast_trig.hpp to your Arduino libraries folder
// or include it in the same directory as this sketch

#include "fast_trig.hpp"

using namespace FastTrig;

// Use compact version for Arduino Uno (2KB RAM)
// Use larger version for ESP32, Teensy, etc.
#ifdef ARDUINO_AVR_UNO
  using MyTrig = Trig32;  // Only 96 bytes
#else
  using MyTrig = Trig128; // 384 bytes for better accuracy
#endif

// Robot servo control example
class ServoController {
  private:
    int servoPin;
    uint16_t currentAngle;
    
  public:
    ServoController(int pin) : servoPin(pin), currentAngle(0) {
      pinMode(servoPin, OUTPUT);
    }
    
    void setAngle(uint16_t angle) {
      currentAngle = angle;
      // Convert angle to servo pulse width (1-2ms)
      // 0° = 1ms, 180° = 2ms
      int pulseWidth = map(AngleConvert::to_degrees(angle), 0, 360, 1000, 2000);
      
      // Generate servo pulse (simplified - use Servo library in real code)
      digitalWrite(servoPin, HIGH);
      delayMicroseconds(pulseWidth);
      digitalWrite(servoPin, LOW);
    }
    
    void smoothMove(uint16_t targetAngle, int steps) {
      for (int i = 0; i <= steps; i++) {
        // Smooth interpolation using sine
        uint16_t t = (i * 8192) / steps;  // 0 to π/2
        int16_t factor = MyTrig::sin(t);  // Smooth acceleration curve
        
        uint16_t angle = currentAngle + 
                        ((targetAngle - currentAngle) * factor) / 16384;
        setAngle(angle);
        delay(20);  // 50Hz update rate
      }
      currentAngle = targetAngle;
    }
};

// Joystick input handler
class JoystickReader {
  private:
    int xPin, yPin;
    
  public:
    JoystickReader(int x, int y) : xPin(x), yPin(y) {}
    
    void begin() {
      pinMode(xPin, INPUT);
      pinMode(yPin, INPUT);
    }
    
    uint16_t getAngle() {
      // Read joystick (0-1023)
      int x = analogRead(xPin) - 512;  // Center at 0
      int y = analogRead(yPin) - 512;
      
      // Dead zone
      if (abs(x) < 50 && abs(y) < 50) {
        return 0;
      }
      
      // Calculate angle using atan2
      return MyTrig::atan2(y, x);
    }
    
    int16_t getMagnitude() {
      int x = analogRead(xPin) - 512;
      int y = analogRead(yPin) - 512;
      
      return MyTrig::magnitude(x, y);
    }
};

// Distance sensor with angle
class RadarSensor {
  private:
    int trigPin, echoPin;
    uint16_t servoAngle;
    
  public:
    RadarSensor(int trig, int echo) : trigPin(trig), echoPin(echo) {}
    
    void begin() {
      pinMode(trigPin, OUTPUT);
      pinMode(echoPin, INPUT);
    }
    
    long getDistance() {
      // Send ultrasonic pulse
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      
      // Read echo time
      long duration = pulseIn(echoPin, HIGH);
      return duration * 0.034 / 2;  // Convert to cm
    }
    
    void scanArea(int16_t* map, int mapSize) {
      for (int i = 0; i < mapSize; i++) {
        uint16_t angle = (i * 16384) / mapSize;
        
        // Point sensor using servo
        // ... servo control code ...
        
        long distance = getDistance();
        
        // Convert polar to cartesian for mapping
        int16_t x = (distance * MyTrig::cos(angle)) / 16384;
        int16_t y = (distance * MyTrig::sin(angle)) / 16384;
        
        // Store in map (simplified)
        map[i] = distance;
        
        Serial.print("Angle: ");
        Serial.print(AngleConvert::to_degrees(angle));
        Serial.print("° Distance: ");
        Serial.print(distance);
        Serial.print("cm Position: (");
        Serial.print(x);
        Serial.print(", ");
        Serial.print(y);
        Serial.println(")");
        
        delay(50);
      }
    }
};

// Global objects
ServoController servo(9);
JoystickReader joystick(A0, A1);
RadarSensor radar(7, 8);

void setup() {
  Serial.begin(9600);
  
  Serial.println("FastTrig Arduino Example");
  Serial.print("Table size: ");
  Serial.print(MyTrig::table_size());
  Serial.print(" entries, Memory: ");
  Serial.print(MyTrig::table_memory());
  Serial.println(" bytes");
  
  // Initialize hardware
  joystick.begin();
  radar.begin();
  
  // Test trigonometric functions
  testTrigFunctions();
  
  // Performance test
  performanceTest();
}

void loop() {
  // Read joystick and move servo
  uint16_t angle = joystick.getAngle();
  int16_t magnitude = joystick.getMagnitude();
  
  if (magnitude > 100) {  // Deadzone
    servo.setAngle(angle);
    
    Serial.print("Joystick angle: ");
    Serial.print(AngleConvert::to_degrees(angle));
    Serial.print("° Magnitude: ");
    Serial.println(magnitude);
  }
  
  // Radar scan every 10 seconds
  static unsigned long lastScan = 0;
  if (millis() - lastScan > 10000) {
    Serial.println("Starting radar scan...");
    int16_t radarMap[36];  // 10° resolution
    radar.scanArea(radarMap, 36);
    lastScan = millis();
  }
  
  delay(50);
}

void testTrigFunctions() {
  Serial.println("\nTrig Function Test:");
  Serial.println("Angle  Sin     Cos     Tan");
  
  for (int deg = 0; deg <= 360; deg += 30) {
    uint16_t angle = AngleConvert::from_degrees(deg);
    int16_t s = MyTrig::sin(angle);
    int16_t c = MyTrig::cos(angle);
    
    Serial.print(deg);
    Serial.print("°    ");
    Serial.print(s / 16384.0 * 2.0, 3);
    Serial.print("  ");
    Serial.print(c / 16384.0 * 2.0, 3);
    
    if (deg % 180 != 90) {
      int16_t t = MyTrig::tan(angle);
      Serial.print("  ");
      Serial.print(t / 8192.0, 3);
    } else {
      Serial.print("  ∞");
    }
    Serial.println();
  }
}

void performanceTest() {
  Serial.println("\nPerformance Test:");
  
  // Test sin performance
  unsigned long start = micros();
  volatile int16_t result = 0;
  for (int i = 0; i < 1000; i++) {
    result = MyTrig::sin(i);
  }
  unsigned long sinTime = micros() - start;
  
  // Test atan2 performance
  start = micros();
  for (int i = 0; i < 1000; i++) {
    result = MyTrig::atan2(i, i + 100);
  }
  unsigned long atanTime = micros() - start;
  
  // Test magnitude performance
  start = micros();
  for (int i = 0; i < 1000; i++) {
    result = MyTrig::magnitude(i, i + 100);
  }
  unsigned long magTime = micros() - start;
  
  Serial.print("Sin: ");
  Serial.print(sinTime / 1000.0);
  Serial.println(" μs per call");
  
  Serial.print("Atan2: ");
  Serial.print(atanTime / 1000.0);
  Serial.println(" μs per call");
  
  Serial.print("Magnitude: ");
  Serial.print(magTime / 1000.0);
  Serial.println(" μs per call");
}
