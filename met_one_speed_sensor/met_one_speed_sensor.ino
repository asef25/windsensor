const byte interruptPin = 3;
volatile int counter;
volatile int rev;

void irq_handler(){
    ++counter;
    if(counter == 40){
        ++rev;
        counter = 0;
    }
}

void setup() {
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.begin(57600);
    pinMode(interruptPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(interruptPin), irq_handler, RISING );
    counter = rev = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
    Serial.println(rev);
}


