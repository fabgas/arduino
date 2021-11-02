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

const long tempsAllumage = 20000; // temps d'allumage de la led. (2s)
const long tempsEteinte = 600000-tempsAllumage;           // interval at which to blink (milliseconds)
 long interval = 6000;
void setup() {
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
}

void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
      interval = tempsAllumage;
    } else {
      ledState = LOW;
      interval = tempsEteinte;
    }
   
    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
}

