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
  for(int i = 1; i <= ZONES; i++) {        // For every zone
    // digitalWrite(zonePins[i - 1], LOW); // Set zone LOW
    // zoneState[i - 1] = false;           // Update the zone status
    offZone(i - 1);
  }
  if(operationMode == 2) operationMode = 1;
}

void nowater() {

  /* Function called from an interrupt
   * If during the watering of a zone the water tank runs
   * low on water we stop watering
   */
  for(int i = 1; i <= ZONES; i++) {        // For every zone
    // digitalWrite(zonePins[i - 1], LOW); // Set zone LOW
    // zoneState[i - 1] = false;           // Update the zone status
    offZone(i - 1);
  }
  if(operationMode == 2) operationMode = 1;
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

  tmElements_t tm;
  unsigned long currentMillis = millis();
  static unsigned long timer = 0;
  static bool stateBlinker = false;
  static int oldOperationMode = -1;
  static bool oldZoneState[ZONES];
  char fecha[17];

  if(oldOperationMode == -1) {
    for(int i = 1; i <= ZONES; i++) {
      oldZoneState[i - 1] = false;
      lcd.setCursor(0, 1);
      lcd.write(1);
      oldOperationMode = 0;
    }
  }
  if (RTC.read(tm)) {
    if (stateBacklight && currentMillis - timerBacklight >= BACKLIGHT) {
      stateBacklight = !stateBacklight;
      lcd.noBacklight();
    }
    if (currentMillis - timer >= 1000) { // Every one second
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
      timer = currentMillis;
    }
    if(oldOperationMode != operationMode) {
      lcd.setCursor(0, 1);
      if (operationMode == 0) {
        // lcd.setCursor(0, 1);
        lcd.write(1);
        lcd.print("  ");
      } else if (operationMode == 1) {
        // lcd.setCursor(1, 1);
        lcd.print(' ');
        lcd.write(1);
        lcd.print(' ');
      } else if (operationMode == 2) {
        // lcd.setCursor(2, 1);
        lcd.print("  ");
        lcd.write(1);
      }
      oldOperationMode = operationMode;
    }
    for(int i = 1; i <= ZONES; i++) {
      // if(oldZoneState[i - 1] != zoneState[i - 1]) {
      // }
      lcd.setCursor(i + 2, 1);
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
  int keyPressed = 0;
  static int oldKeyPressed = 0;
  unsigned long currentMillis = millis();
  static unsigned long timer = 0;
  //char texto[255];

  keyPressed = map(analogRead(KEYPIN), 0, 1024, 0, 10);
  // sprintf(texto, "%lu %lu %lu %d %d", currentMillis, timer, currentMillis - timer, keyPressed, oldKeyPressed);
  // Serial.println(texto);
  if(keyPressed != 0) {
    timerBacklight = currentMillis;
    if(!(stateBacklight)) {
      lcd.backlight();
      stateBacklight = true;
    }
  }
  if(keyPressed != 0 && oldKeyPressed == 0) {
    oldKeyPressed = keyPressed;
    timer = currentMillis;
  }
  if(keyPressed == 0 && oldKeyPressed == 2 && currentMillis - timer >= DEBOUNCE) {
    oldKeyPressed = 0;
    return btnLeft;
  } else if(keyPressed == 0 && oldKeyPressed == 5 && currentMillis - timer >= 4000) {
    oldKeyPressed = 0;
    return btnProg;
  } else if(keyPressed == 0 && oldKeyPressed == 5 && currentMillis - timer >= DEBOUNCE) {
    oldKeyPressed = 0;
    return btnSet;
  } else if(keyPressed == 0 && oldKeyPressed == 6 && currentMillis - timer >= DEBOUNCE) {
    oldKeyPressed = 0;
    return btnDown;
  } else if(keyPressed == 0 && oldKeyPressed == 8 && currentMillis - timer >= DEBOUNCE) {
    oldKeyPressed = 0;
    return btnRight;
  } else if(keyPressed == 0 && oldKeyPressed == 9 && currentMillis - timer >= DEBOUNCE) {
    oldKeyPressed = 0;
    return btnUp;
  } else if(keyPressed == 0 && oldKeyPressed != 0 && currentMillis - timer < DEBOUNCE) {
    oldKeyPressed = 0;
    return btnNone;
  } else {
    return btnNone;
  }
  return btnNone;
}

void manualOperation() {

  int runZone = 1;
  bool zoneRun = true;
  unsigned long currentMillis = millis();
  unsigned long timerManual = currentMillis;
  //char texto[255];
  
  while(operationMode == 2) {
    currentMillis = millis();
    //sprintf(texto, "%d %lu %lu %lu", runZone, currentMillis, timerManual, currentMillis - timerManual);
    //Serial.println(texto);
    displayStatus();
    if(keyPress() == btnSet) {
      if(zoneState[runZone]) {
        offZone(ZONEC);
        offZone(runZone);
      }
      timerManual = currentMillis;
      runZone++;
      zoneRun = true;
      if(runZone == ZONES) {
        for(int i = 1; i <= ZONES; i++) {
          offZone(i - 1);
        }
        operationMode++;
        if(operationMode == 3) operationMode = 0;
      }
    }
    if(!(zoneState[runZone]) && currentMillis - timerManual >= 5000) {
      //Serial.println("Encendiendo");
      //delay(1000);
      onZone(runZone);
      onZone(ZONEC);
      zoneRun = false;
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  setSyncProvider(RTC.get); // Set the RTC as source of time for alarms
  pinMode(RAINS, INPUT); // Configure the rain sensor
  pinMode(TANKS, INPUT_PULLUP); // Configure the water tank sensor
  for(int i = 1; i <= ZONES; i++) {
    pinMode(zonePins[i - 1], OUTPUT); // Configure the zones
    digitalWrite(zonePins[i - 1], HIGH);
    zoneState[i - 1] = false;
  }
  pinMode(LED_BUILTIN, OUTPUT); // Configure the builtin led
  attachInterrupt(digitalPinToInterrupt(RAINS), raining, RISING); // Create an interrupt for the rain sensor
  attachInterrupt(digitalPinToInterrupt(TANKS), nowater, RISING); // Create an interrupt for the water tank sensor
  lcd.init(); // Initialize the LCD screen
  lcd.backlight(); // Turn on the LCD backlight
  lcd.createChar(0, arrowUp);
  lcd.createChar(1, arrowDown);
  alarmas(); // Read the alarms
}

void loop() {
  // put your main code here, to run repeatedly:
  int keyPressed;
  
  displayStatus();
  keyPressed = keyPress();
  if(keyPressed == btnSet) {
    operationMode++;
    if(operationMode == 3) operationMode = 0;
  }
  if(operationMode == 2) manualOperation();
  Alarm.delay(0);
}
