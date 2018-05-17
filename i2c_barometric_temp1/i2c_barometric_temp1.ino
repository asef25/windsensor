// Adafruit MPL115A2 interface, without using provided library
// Working as of 5/11/18 CDW
// should work either with included i2cwrite() function or by subbing in Wire.write() for all instances



#include <Wire.h>

    #define MPL115A2_ADDRESS                       (0x60)
// Define registers for MPL115A2
    #define MPL115A2_REGISTER_PRESSURE_MSB         (0x00)
    #define MPL115A2_REGISTER_PRESSURE_LSB         (0x01)
    #define MPL115A2_REGISTER_TEMP_MSB             (0x02)
    #define MPL115A2_REGISTER_TEMP_LSB             (0x03)
    #define MPL115A2_REGISTER_A0_COEFF_MSB         (0x04)
    #define MPL115A2_REGISTER_A0_COEFF_LSB         (0x05)
    #define MPL115A2_REGISTER_B1_COEFF_MSB         (0x06)
    #define MPL115A2_REGISTER_B1_COEFF_LSB         (0x07)
    #define MPL115A2_REGISTER_B2_COEFF_MSB         (0x08)
    #define MPL115A2_REGISTER_B2_COEFF_LSB         (0x09)
    #define MPL115A2_REGISTER_C12_COEFF_MSB        (0x0A)
    #define MPL115A2_REGISTER_C12_COEFF_LSB        (0x0B)
    #define MPL115A2_REGISTER_STARTCONVERSION      (0x12)

  
  // declare variables for device constants
  float _mpl115a2_a0;
  float _mpl115a2_b1;
  float _mpl115a2_b2;
  float _mpl115a2_c12;


  uint16_t   raw_pressure, raw_temp; //variables to record pressure and temp to
  float     pressureComp, pBaro, tempC; // pressure compensation variable, calculated pressure and temp



void setup() {
  Wire.begin();                // join i2c bus (address optional for master)
  Serial.begin(9600);          // start serial communication at 9600bps
  Serial.println("initializing");

  //initialize sensor
  int16_t a0coeff;
  int16_t b1coeff;
  int16_t b2coeff;
  int16_t c12coeff;

  Wire.beginTransmission(MPL115A2_ADDRESS);
  i2cwrite((uint8_t)MPL115A2_REGISTER_A0_COEFF_MSB);
  Wire.endTransmission();

 Serial.println("requesting coefficients");
  Wire.requestFrom(MPL115A2_ADDRESS, 8);
  a0coeff = (( (uint16_t) Wire.read() << 8) | Wire.read());
  b1coeff = (( (uint16_t) Wire.read() << 8) | Wire.read());
  b2coeff = (( (uint16_t) Wire.read() << 8) | Wire.read());
  c12coeff = (( (uint16_t) (Wire.read() << 8) | Wire.read())) >> 2;

  /*  
  Serial.print("A0 = "); Serial.println(a0coeff, HEX);
  Serial.print("B1 = "); Serial.println(b1coeff, HEX);
  Serial.print("B2 = "); Serial.println(b2coeff, HEX);
  Serial.print("C12 = "); Serial.println(c12coeff, HEX);
  */

 //record device constants
  _mpl115a2_a0 = (float)a0coeff / 8;
  _mpl115a2_b1 = (float)b1coeff / 8192;
  _mpl115a2_b2 = (float)b2coeff / 16384;
  _mpl115a2_c12 = (float)c12coeff;
  _mpl115a2_c12 /= 4194304.0;

   Serial.println("done initializing");

}

int reading = 0;

void loop() {
  // step 1: instruct sensor to read echoes
   Serial.println("starting loop");
  Wire.beginTransmission(MPL115A2_ADDRESS);
  i2cwrite((uint8_t)MPL115A2_REGISTER_STARTCONVERSION);
  i2cwrite((uint8_t)0x00);
  Wire.endTransmission();

  // Wait a bit for the conversion to complete (3ms max)
  delay(5);

  Wire.beginTransmission(MPL115A2_ADDRESS);
  i2cwrite((uint8_t)MPL115A2_REGISTER_PRESSURE_MSB);  // Register
  Wire.endTransmission();
   Serial.println("1");

  Wire.requestFrom(MPL115A2_ADDRESS, 4);
  raw_pressure = (( (uint16_t) Wire.read() << 8) | Wire.read()) >> 6;
  raw_temp = (( (uint16_t) Wire.read() << 8) | Wire.read()) >> 6;

 Serial.println("calculating");
  // See datasheet p.6 for evaluation sequence
  pressureComp = _mpl115a2_a0 + (_mpl115a2_b1 + _mpl115a2_c12 * raw_temp ) * raw_pressure + _mpl115a2_b2 * raw_temp;

  // Return pressure and temperature as floating point values
  pBaro = ((65.0F / 1023.0F) * pressureComp) + 50.0F;        // kPa
  tempC = ((float) raw_temp - 498.0F) / -5.35F +25.0F;           // C
  
  Serial.print("Pressure (kPa): "); Serial.print(pBaro, 4); Serial.print(" kPa  ");
  Serial.print("Temp (*C): "); Serial.println(tempC, 1);




//  i2cwrite(byte(0x00));      // sets register pointer to the command register (0x00)
//  i2cwrite(byte(0x50));      // command sensor to measure in "inches" (0x50)
//  // use 0x51 for centimeters
//  // use 0x52 for ping microseconds
//  Wire.endTransmission();      // stop transmitting
//
//  // step 2: wait for readings to happen
//  delay(70);                   // datasheet suggests at least 65 milliseconds
//
//  // step 3: instruct sensor to return a particular echo reading
//  Wire.beginTransmission(112); // transmit to device #112
//  i2cwrite(byte(0x02));      // sets register pointer to echo #1 register (0x02)
//  Wire.endTransmission();      // stop transmitting
//
//  // step 4: request reading from sensor
//  Wire.requestFrom(112, 2);    // request 2 bytes from slave device #112
//
//  // step 5: receive reading from sensor
//  if (2 <= Wire.available()) { // if two bytes were received
//    reading = Wire.read();  // receive high byte (overwrites previous reading)
//    reading = reading << 8;    // shift high byte to be high 8 bits
//    reading |= Wire.read(); // receive low byte as lower 8 bits
//    Serial.println(reading);   // print the reading
//  }

  delay(1000);                  // 
}


/*

// The following code changes the address of a Devantech Ultrasonic Range Finder (SRF10 or SRF08)
// usage: changeAddress(0x70, 0xE6);

void changeAddress(byte oldAddress, byte newAddress)
{
  Wire.beginTransmission(oldAddress);
  Wire.write(byte(0x00));
  Wire.write(byte(0xA0));
  Wire.endTransmission();

  Wire.beginTransmission(oldAddress);
  Wire.write(byte(0x00));
  Wire.write(byte(0xAA));
  Wire.endTransmission();

  Wire.beginTransmission(oldAddress);
  Wire.write(byte(0x00));
  Wire.write(byte(0xA5));
  Wire.endTransmission();

  Wire.beginTransmission(oldAddress);
  Wire.write(byte(0x00));
  Wire.write(newAddress);
  Wire.endTransmission();
}

*/

static void i2cwrite(uint8_t x) {
  #if ARDUINO >= 100
  Wire.write((uint8_t)x);
  #else
  Wire.send(x);
  #endif
}
