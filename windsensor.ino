#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ADS1015.h>
#include "RTClib.h"


const byte interruptPin = 2;
volatile int counter; //# of pulses
volatile long rpm;    //revs per min
volatile bool rw_flag;
volatile float V;     //Velocity [miles per hour]
volatile int g_cycles = 0; //# of cycles for timer1

RTC_PCF8523      rtc;
Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

File        dataFile;
const char  FILE_PATH[]   = "datalog.txt";
const int chipSelect      = 10;

/*Wind dir variables*/
const int sensorPin = A3;    //input value: wind sensor analog
int sensorValue = 0;  // variable to store the value coming from the sensor

void setup(void)
{
        
    // Open serial communications and wait for port to open:
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.begin(57600);

    if (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (! rtc.initialized()) {
        Serial.println("RTC is NOT running!");
        // following line sets the RTC to the date & time this sketch was compiled
        // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }
      
    
    Serial.print("\nInitializing SD card...");
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    pinMode(10, OUTPUT);
    // we'll use the initialization code from the utility libraries
    // since we're just testing if the card is working!
    if (!SD.begin(chipSelect)) {
        Serial.println("initialization failed. Things to check:");
        Serial.println("* is a card inserted?");
        Serial.println("* is your wiring correct?");
        Serial.println("* did you change the chipSelect pin to match your shield or module?");
       // return;
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
    //ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
    // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
    // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
    ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

    ads.begin();

// initialize timer1 - 16 bit 65536
    noInterrupts();           // disable all interrupts
    TCCR1A  = 0;
    TCCR1B  = 0;
    
    TCNT1   = 49911;            // preload timer 16MHz/1024 => Period=4sec
    TCCR1B |= ((1 << CS12)| (1 << CS10)) ;    // 1024 prescaler 
    TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
    interrupts();             // enable all interrupts

// initialize timer0 - rising edge triggered interrupt - pin 2
    attachInterrupt(digitalPinToInterrupt(interruptPin), pin_irq_handler, RISING );

    // initialize variables for interrupts
    counter = 0;
    rpm     = 0;
    rw_flag = 0;
    V       = 0.0f;

#define WRITE_TO_SDCARD(text)                                               \ 
    if((dataFile = SD.open(FILE_PATH, FILE_WRITE))){                 \
        dataFile.println(text);                                             \
        dataFile.close();                                                   \
    } else {                                                                \
        Serial.println("Failed to open or create " + String(FILE_PATH));    \ 
    }                       

    WRITE_TO_SDCARD("time,\t\t\tx_mV,\ty_mv,\tx_lbs,\ty_lbs,\tdir,\trpm,\tVel");
    Serial.println("time,\t\t\tx_mV,\ty_mv,\tx_lbs,\ty_lbs,\tdir,\trpm,\tVel");
}

ISR(TIMER1_OVF_vect)        
{
#define PERIOD_THRESHOLD 6 //6 seconds
    TCNT1 = 49911;
    g_cycles++;
    
    if(PERIOD_THRESHOLD == g_cycles){
        rpm     = counter * 15L;
        rpm    /= 40L;
        V       = (rpm / 16.767f) + 0.6f;
        rw_flag = 1;
        counter = 0;
        g_cycles = 0;
    }
}

void pin_irq_handler()
{
    ++counter;
}

void loop(void)
{
    int16_t results_x, results_y;
    float lbs_x, lbs_y;
    float x_mV, y_mV;
    String str;


    if(rw_flag){
      
        rw_flag = !rw_flag;
        
        DateTime now = rtc.now();
        results_x    = ads.readADC_Differential_0_1(); 
        results_y    = ads.readADC_Differential_2_3();
    
        //read wind dir analog value
        sensorValue = analogRead(sensorPin);

/* Be sure to update this value based on the IC and the gain settings! */
#define  MULTIPLIER 0.0078125F /* ADS1115  @ +/- 0.256V gain (16-bit results) */

        x_mV   = results_x * MULTIPLIER;
        y_mV   = results_y * MULTIPLIER;
        
        lbs_x        = x_mV / 100.0 * 25.0;
        lbs_y        = y_mV / 100.0 * 25.0;
    
        str = "";
        str += String(now.year(), DEC);
        str += '/';
        str += String(now.month(), DEC);
        str += '/';
        str += String(now.day(), DEC);
        str += " ";
        str += String(now.hour(), DEC);
        str += ':';
        str += String(now.minute(), DEC);
        str += ':';
        str += String(now.second(), DEC);  
        str += ",\t";
        str += String(x_mV);
        str += ",\t";
        str += String(y_mV);
        str += ",\t";
        
        str += String(lbs_x);
        str += ",\t";
        str += String(lbs_y);
        str += ",\t";
        //map dir sensor val from analog 0 to 1013 to 0 to 360 deg
        str += String(((float)sensorValue - 0.0f) / (1013.0f - 0.0f) * (360.0f - 0.0f));
        str += ",\t";
        str += String(rpm);
        str += ",\t";
        str += String(V);
        
        WRITE_TO_SDCARD(str);
        Serial.println(str);
    }
}
