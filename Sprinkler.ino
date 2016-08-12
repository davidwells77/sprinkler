#include <Wire.h>
#include <Time.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <LiquidCrystal_I2C.h>
#include <TimeAlarms.h>

#define DEBOUNCE 50
#define BACKLIGHT 30000 // For how long the backlight is going to remain on
#define RAINS 2         // Pin hooked up to the rain sensor
#define TANKS 3         // Pin hooked up to the water tank sensor
#define ZONES 4         // Number of zones to water
#define ZONEC 0         // Which of the zones is a common zone (water pump, solenoid transformer)
#define KEYPIN A3       // Pin hooked up to the keypad

const int btnNone =  0;
const int btnSet =   1;
const int btnLeft =  2;
const int btnRight = 3;
const int btnUp =    4;
const int btnDown =  5;
const int btnProg =  6;

volatile int zonePins[ZONES] = {7, 8, 9, 10}; // Definition of the pins hooked up to each of the zones
volatile bool zoneState[ZONES]; // State of the zones
unsigned long timerBacklight = 0;
bool stateBacklight = true;
int operationMode = 0; // Mode of operation, 0 = Auto, 1 = Off, 2 = Manual
int nbrEvents = 0; // Number of programed events
uint8_t arrowUp[8] =   {0x4, 0xe, 0x1f, 0x0, 0x0, 0x0, 0x0};
uint8_t arrowDown[8] = {0x0, 0x0, 0x0, 0x0, 0x1f, 0xe, 0x4};
AlarmId eventId[dtNBR_ALARMS]; // ID's of the programed events

LiquidCrystal_I2C lcd(0x3f, 16, 2); // Initialization of the LCD Screen

bool rain() {

  // Test the rain sensor and return True if raining
  if (digitalRead(RAINS) == HIGH) {
    return false;
  } else {
    return true;
  }
}

bool tank() {

  // Test the water tank sensor and return true if there is no water
  if (digitalRead(TANKS) == LOW) {
    return false;
  } else {
    return true;
  }
}

void onZone(int zone) {

  // Turn on a zone and update the status of the zone
  if (!(rain()) && !(tank())) {
    digitalWrite(zonePins[zone], LOW);
    zoneState[zone] = true;
  }
}

void offZone(int zone) {

  // Turn off a zone and update the status of the zone
  if (zoneState[zone] = true) {
    digitalWrite(zonePins[zone], HIGH);
    zoneState[zone] = false;
  }
}

void raining() {

  /* Function called from an interrupt
   * If during the watering of a zone it starts raining
   * we stop watering
   */
  bool flag = false;
  unsigned long currentMillis = millis();
  static unsigned long timer = 0;
  
  for(int i = 1; i <= ZONES; i++) {
    if(zoneState[i - 1]) flag = true;
  }
  if(flag && currentMillis - timer >= DEBOUNCE) {
    for(int i = 1; i <= ZONES; i++) {        // For every zone
      // digitalWrite(zonePins[i - 1], LOW); // Set zone LOW
      // zoneState[i - 1] = false;           // Update the zone status
      offZone(i - 1);
    }
    if(operationMode == 2) operationMode = 1;
    timer = currentMillis;
  }
}

void nowater() {

  /* Function called from an interrupt
   * If during the watering of a zone the water tank runs
   * low on water we stop watering
   */
  bool flag = false;
  unsigned long currentMillis = millis();
  static unsigned long timer = 0;

  for(int i = 1; i <= ZONES; i++) {
    if(zoneState[i - 1]) flag = true;
  }
  if(flag && currentMillis - timer >= DEBOUNCE) {
    for(int i = 1; i <= ZONES; i++) {        // For every zone
      // digitalWrite(zonePins[i - 1], LOW); // Set zone LOW
      // zoneState[i - 1] = false;           // Update the zone status
      offZone(i - 1);
    }
    if(operationMode == 2) operationMode = 1;
    timer = currentMillis;
  }
}

void onZone1() {

  /* Turn on Zone 1 and the common zone if it is not raining
   * and there is water in the water tank and the operation mode
   * is Auto
   */
  if (operationMode == 0) {
    onZone(1);
    onZone(ZONEC);
  }
}

void onZone2() {

  /* Turn on Zone 2 and the common zone if it is not raining
   * and there is water in the water tank and the operation mode
   * is Auto
   */
  if (operationMode == 0) {
    onZone(2);
    onZone(ZONEC);
  }
}

void onZone3() {

  /* Turn on Zone 3 and the common zone if it is not raining
   * and there is water in the water tank and the operation mode
   * is Auto
   */
  if (operationMode == 0) {
    onZone(3);
    onZone(ZONEC);
  }
}

void offZone1() {

  // Turn off zone 1 and the common zone
  offZone(ZONEC);
  offZone(1);
}

void offZone2() {

  // Turn off zone 2 and the common zone
  offZone(ZONEC);
  offZone(2);
}

void offZone3() {

  // Turn off zone 3 and the common zone
  offZone(ZONEC);
  offZone(3);
}

void alarmas() {

  /* Program the alarms, record the alarm ids and the amount
   *  and the number of alarms
   * In the future these will be read from nvram
   */
  int i = 0;
  eventId[i++] = Alarm.alarmRepeat(dowMonday, 1, 0, 0, onZone1);
  eventId[i++] = Alarm.alarmRepeat(dowMonday, 1, 2, 0, offZone1);
  eventId[i++] = Alarm.alarmRepeat(dowMonday, 3, 0, 0, onZone2);
  eventId[i++] = Alarm.alarmRepeat(dowMonday, 3, 2, 0, offZone2);
  eventId[i++] = Alarm.alarmRepeat(dowThursday, 1, 0, 0, onZone1);
  eventId[i++] = Alarm.alarmRepeat(dowThursday, 1, 2, 0, offZone1);
  eventId[i++] = Alarm.alarmRepeat(dowThursday, 3, 0, 0, onZone2);
  eventId[i++] = Alarm.alarmRepeat(dowThursday, 3, 2, 0, offZone2);
  nbrEvents = i;
}

void displayStatus() {

  tmElements_t tm; // time variable
  unsigned long currentMillis = millis(); // Current millis() value
  static unsigned long timer = 0; // Blinker timer
  static bool stateBlinker = false; // Blinker state
  static int oldOperationMode = -1; // Old operation mode
  static bool oldZoneState[ZONES]; // Old zone state
  char fecha[17]; // String to display the formated date

  if(oldOperationMode == -1) {
    // if it's the first time we update the screen
    for(int i = 1; i <= ZONES; i++) {
      // for every zone
      oldZoneState[i - 1] = false; // Set the status to off
      lcd.setCursor(0, 1); // Automatic operation indicator
      lcd.write(1); // Draw a down pointing arrow
      oldOperationMode = 0; // Update the old operation mode
    }
  }
  if (RTC.read(tm)) {
    // Read the time from the rtc
    if (stateBacklight && currentMillis - timerBacklight >= BACKLIGHT) {
      // If the backlight is on and its time to turn it off
      lcd.noBacklight(); // Turn backlight off
      stateBacklight = !stateBacklight; // flip the backlight state
    }
    if (currentMillis - timer >= 1000) {
      // Every one second
      stateBlinker = !stateBlinker; // Alternate the blinker status
      if (stateBlinker == true) {
        sprintf(fecha, "%04u/%02u/%02u %02u:%02u", tmYearToCalendar(tm.Year), tm.Month, tm.Day, tm.Hour, tm.Minute); // Blinking colon on
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        sprintf(fecha, "%04u/%02u/%02u %02u %02u", tmYearToCalendar(tm.Year), tm.Month, tm.Day, tm.Hour, tm.Minute); // Blinking colon off
        digitalWrite(LED_BUILTIN, LOW);
      }
      lcd.setCursor(0, 0);
      lcd.print(fecha); // Print the date and time
      timer = currentMillis; // Restart the blinker timer
    }
    if(oldOperationMode != operationMode) {
      // if the operation mode changed update the operation indicator
      lcd.setCursor(0, 1);
      if (operationMode == 0) {
        lcd.write(1);
        lcd.print("  ");
      } else if (operationMode == 1) {
        lcd.print(' ');
        lcd.write(1);
        lcd.print(' ');
      } else if (operationMode == 2) {
        lcd.print("  ");
        lcd.write(1);
      }
      oldOperationMode = operationMode;
    }
    for(int i = 1; i <= ZONES; i++) {
      // for every zone
      // if(oldZoneState[i - 1] != zoneState[i - 1]) {
      // only if the zone state changed
      // }
      lcd.setCursor(i + 2, 1); // locate the cursor
      if (zoneState[i - 1]) {
        lcd.write(1);
      } else {
        lcd.print(' ');
      }
    }
    lcd.setCursor(ZONES + 3, 1);
    if (rain()) {
      lcd.write(1);
    } else {
      lcd.print(' ');
    }
    lcd.setCursor(ZONES + 4, 1);
    if (tank()) {
      lcd.write(1);
    } else {
      lcd.print(' ');
    }
  }
}

int keyPress() {
  /* Function to read keys from the keypad
   *  0: None
   *  2: Left
   *  5: Enter
   *  6: Down
   *  8: Right
   *  9: Up
   */
  int keyPressed = 0; // Value read from the keypad
  static int oldKeyPressed = 0; // Old value read from the keypad
  unsigned long currentMillis = millis(); // current millis value
  static unsigned long timer = 0; // how long has the button been pressed
  //char texto[255];

  keyPressed = map(analogRead(KEYPIN), 0, 1024, 0, 10); // Read the analog value from the keypad pin
  // sprintf(texto, "%lu %lu %lu %d %d", currentMillis, timer, currentMillis - timer, keyPressed, oldKeyPressed);
  // Serial.println(texto);
  if(keyPressed != 0) {
    // A key is pressed
    timerBacklight = currentMillis; // restart the backlight off timer
    if(!(stateBacklight)) {
      // if the backlight is off
      lcd.backlight(); // turn the backlight on
      stateBacklight = true; // save the backlight state
    }
  }
  if(keyPressed != 0 && oldKeyPressed == 0) {
    // if there is a key pressed
    oldKeyPressed = keyPressed; // Save the key pressed
    timer = currentMillis; // Start the debounce timer
  }
  if(keyPressed == 0 && oldKeyPressed == 2 && currentMillis - timer >= DEBOUNCE) {
    // if the left button was pressed
    oldKeyPressed = 0;
    return btnLeft;
  } else if(keyPressed == 0 && oldKeyPressed == 5 && currentMillis - timer >= 4000) {
    // if the set button was pressed for 4 seconds or more
    oldKeyPressed = 0;
    return btnProg;
  } else if(keyPressed == 0 && oldKeyPressed == 5 && currentMillis - timer >= DEBOUNCE) {
    // if the set button was pressed
    oldKeyPressed = 0;
    return btnSet;
  } else if(keyPressed == 0 && oldKeyPressed == 6 && currentMillis - timer >= DEBOUNCE) {
    // if the down button was pressed
    oldKeyPressed = 0;
    return btnDown;
  } else if(keyPressed == 0 && oldKeyPressed == 8 && currentMillis - timer >= DEBOUNCE) {
    // if the right button was pressed
    oldKeyPressed = 0;
    return btnRight;
  } else if(keyPressed == 0 && oldKeyPressed == 9 && currentMillis - timer >= DEBOUNCE) {
    // if the up button was pressed
    oldKeyPressed = 0;
    return btnUp;
  } else if(keyPressed == 0 && oldKeyPressed != 0 && currentMillis - timer < DEBOUNCE) {
    // if the button was a bounce
    oldKeyPressed = 0;
    return btnNone;
  } else {
    return btnNone; // while there is no key change return none
  }
  return btnNone; // nothing was pressed
}

void manualOperation() {

  int runZone = 1; // Zone to run
  bool zoneRun = true; // Zone must be run
  unsigned long currentMillis; // Current millis() value
  unsigned long timerManual = currentMillis; // Last time the zone ran millis()
  bool flag = false; // If there is a zone running then true
  //char texto[255];
  
  while(operationMode == 2) {
    currentMillis = millis(); // Current millis() value
    //sprintf(texto, "%d %lu %lu %lu", runZone, currentMillis, timerManual, currentMillis - timerManual);
    //Serial.println(texto);
    displayStatus(); // Update the screen display
    for(int i = 1; i <= ZONES; i++) {
      // For all zones
      if(zoneState[i - 1]) flag = true; // If there is a zone on true
      if(flag) break; // If there is a zone on break
    }
    if(flag && (rain() || tank())) {
      // If there is a zone on and it starts raining or the water runs out
      for(int i = 1; i <= ZONES; i++) {
        // For every zone
        offZone(i - 1); // Turn off
      }
      operationMode = 1; // If it was running on manual and we ran out of water or started raining set operation mode to off
      return;
    }
    if(keyPress() == btnSet) {
      // Read a key press
      if(zoneState[runZone]) { // if set is pressed and there is a zone running turn everything of
        offZone(ZONEC); // Turn common zone off
        offZone(runZone); // Turn running zone off
      }
      timerManual = currentMillis; // Start counting millis since the keypress of set
      runZone++; // Change zone to run
      zoneRun = true; // Zone must be run
      if(runZone == ZONES) {
        // If there are no more zones turn everything of and change operation mode
        for(int i = 1; i <= ZONES; i++) {
          // for all zones
          offZone(i - 1); // turn zone off
        }
        operationMode++; // Change operation mode
        if(operationMode == 3) operationMode = 0; // cycle through operation modes
      }
    }
    if(!(zoneState[runZone]) && currentMillis - timerManual >= 5000) {
      // a zone must be run and 5 seconds have passed since the keypress
      onZone(runZone); // turn the zone on
      onZone(ZONEC); // turn the common zone on
      zoneRun = false; // zone must not be run again
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  delay(1000); // Wait for board to fully boot up before we start
  Serial.begin(9600); // Set Serial port parameters
  setSyncProvider(RTC.get); // Set the RTC as source of time for alarms
  pinMode(RAINS, INPUT); // Configure the rain sensor
  pinMode(TANKS, INPUT_PULLUP); // Configure the water tank sensor
  for(int i = 1; i <= ZONES; i++) {
    // for all zones
    digitalWrite(zonePins[i - 1], HIGH); // Set the pin high before configuring it so the relay doesn't start at startup
    pinMode(zonePins[i - 1], OUTPUT); // Configure the zones
    zoneState[i - 1] = false; // Record the zone state
  }
  pinMode(LED_BUILTIN, OUTPUT); // Configure the builtin led
  /* I would love to use interrupts to shutdown zones if raining or out of water but for some reason
   *  the interrupts get triggered when a zone starts
   */
  //attachInterrupt(digitalPinToInterrupt(RAINS), raining, RISING); // Create an interrupt for the rain sensor
  //attachInterrupt(digitalPinToInterrupt(TANKS), nowater, RISING); // Create an interrupt for the water tank sensor
  lcd.init(); // Initialize the LCD screen
  lcd.backlight(); // Turn on the LCD backlight
  lcd.createChar(0, arrowUp); // Create an up pointing arrow in memory slot 0
  lcd.createChar(1, arrowDown); // Create a down pointing arrow in memory slot 1
  alarmas(); // Read the alarms
}

void loop() {
  // put your main code here, to run repeatedly:
  int keyPressed; // Key press code
  bool flag = false; // If any zone is running the flag will be true
    
  displayStatus(); // Update the screen display
  for(int i = 1; i <= ZONES; i++) {
    // for all zones
    if(zoneState[i - 1]) flag = true; // if a zone is on then flag is true
    if(flag) break; // If there is a zone on break
  }
  if(flag && (rain() || tank())) {
    // if there is a zone on and it starts raining or there is no mo water in the water tank
    for(int i = 1; i <= ZONES; i++) {
      // for all zones
      offZone(i - 1); // turn the zone off
    }
    return;
  }
  keyPressed = keyPress(); // Read a keypress
  if(keyPressed == btnSet) {
    // If the set button was pressed
    operationMode++; // Change operation mode
    if(operationMode == 3) operationMode = 0;
  }
  if(operationMode == 2) manualOperation(); // If Operation mode is manual go to manualOperation
  Alarm.delay(0); // Call the alarms
}
