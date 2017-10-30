#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ADS1015.h>
#include <stdio.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
// set up variables using the SD utility library functions:
Sd2Card card;
//SdVolume volume;
//SdFile root;

const int chipSelect = 10;

void setup(void)
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("\nInitializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  
  Serial.println("Getting differential reading from AIN0 (P) and AIN1 (N)");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
  
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
  ads.begin();
}

void loop(void)
{
  int16_t results_x, results_y;
  float lbs_x, lbs_y;
  
  /* Be sure to update this value based on the IC and the gain settings! */
#define  MULTIPLIER 0.1875F /* ADS1115  @ +/- 6.144V gain (16-bit results) */

  results_x    = ads.readADC_Differential_0_1(); 
  results_y    = ads.readADC_Differential_2_3();  

#define MAP(results) \
  (((float)results * MULTIPLIER) / 100.0 * 25.0)

  lbs_x        = MAP(results_x);
  lbs_y        = MAP(results_y);

#define PRINT_DIFFERENTIAL(results) \
  Serial.print("Differential: "); Serial.print(results); Serial.print("("); Serial.print(results * MULTIPLIER); Serial.println("mV)"); 

  PRINT_DIFFERENTIAL(results_x);
  PRINT_DIFFERENTIAL(results_y);

#define PRINT_IBS(lbs) \
  Serial.print("Map: ("); Serial.print(lbs); Serial.print("lbs)\n");

  PRINT_IBS(lbs_x);
  PRINT_IBS(lbs_y);
  
  delay(1000);
}
