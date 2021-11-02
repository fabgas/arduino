/*
  Blink with Delay

*/

// constants won't change. Used here to set a pin number:
const int ledPin =  LED_BUILTIN;// the number of the LED pin

// Variables will change:
int ledState = LOW;             // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

const long tempsAllumage = 6000; // temps d'allumage de la led. (2s)
const long tempsEteinte = 60000-tempsAllumage;           // interval at which to blink (milliseconds)
void setup() {
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
}

void loop() {
   digitalWrite(ledPin, HIGH);
   delay(tempsAllumage); // 
   digitalWrite(ledPin, LOW);
   delay(tempsEteinte); // 
}
