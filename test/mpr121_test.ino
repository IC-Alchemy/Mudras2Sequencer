#include <Wire.h>
#include "Adafruit_MPR121.h"

// You can have up to 4 on one I2C bus but one is enough for testing!
Adafruit_MPR121 touchSensor = Adafruit_MPR121();

void setup() {
  Serial.begin(115200);
  while (!Serial) { // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!touchSensor.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");
}

void loop() {
  // Get the currently touched pads
  uint16_t touched = touchSensor.touched();

  Serial.print("Touched: 0x"); 
  Serial.println(touched, HEX);

  delay(100);
}