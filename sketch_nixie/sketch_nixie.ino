#define CLK 27
#define DAT 25
#define RST 23

#define STEPS 18

#include <ThreeWire.h>
#include <RtcDS1302.h>

ThreeWire myWire(DAT, CLK, RST);
RtcDS1302<ThreeWire> Rtc(myWire);

int pins[][4] = {{46, 48, 50, 52}, {38, 40, 42, 44}, {30, 32, 34, 36}, {31, 33, 35, 37}, {39, 41, 43, 45}, {47, 49, 51, 53}};
unsigned int state[6] = {0, 0, 0, 0, 0, 0};
int counter = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) {
      pinMode(pins[i][j], OUTPUT);
    }
  }
  Serial.begin(57600);

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
    // Common Causes:
    //  1) first time you ran and the device wasn't running yet
    //  2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  post(state);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(100);
  RtcDateTime now = Rtc.GetDateTime();
  counter++;
  if (counter == 20) {
    counter = 0;
    printDateTime(now);
    Serial.println();

    if (!now.IsValid()) {
      // Common Causes:
      //  1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }
  unsigned int hour = now.Hour();
  unsigned int minute = now.Minute();
  unsigned int second = now.Second();
  if (state[5] == second % 10) return;
  unsigned int newstate[] = {hour / 10, hour % 10, minute / 10, minute % 10, second / 10, second % 10};
  crossFade(state, newstate);
}

void printDateTime(const RtcDateTime& dt) {
  char datestring[20];

  snprintf_P(
    datestring,
    sizeof(datestring) / sizeof(datestring[0]),
    PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
    dt.Month(),
    dt.Day(),
    dt.Year(),
    dt.Hour(),
    dt.Minute(),
    dt.Second()
  );
  Serial.print(datestring);
}

void nixieDisplay(int index, int num) {
  digitalWrite(pins[index][0], num >> 0 & 0b01 ? HIGH : LOW); // A
  digitalWrite(pins[index][1], num >> 1 & 0b01 ? HIGH : LOW); // B
  digitalWrite(pins[index][2], num >> 2 & 0b01 ? HIGH : LOW); // C
  digitalWrite(pins[index][3], num >> 3 & 0b01 ? HIGH : LOW); // D
}

void crossFade(unsigned int* state, unsigned int* newstate) {
  for (int timer = 0; timer <= STEPS; timer++) {
    for (int index = 0; index < 6; index++) {
      nixieDisplay(index, state[index]);
    }
    delay(STEPS - timer);
    for (int index = 0; index < 6; index++) {
      nixieDisplay(index, newstate[index]);
    }
    delay(timer);
  }
  for (int index = 0; index < 6; index++) {
    state[index] = newstate[index];
  }
}

void post(unsigned int* state) {
  for (int num = 1; num < 10; num++) {
    unsigned int newstate[6] = {num, num, num, num, num, num};
    crossFade(state, newstate);
    delay(500);
  }
}
