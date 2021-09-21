#define SDA 27
#define SCL 25

#define STEPS 18

#include <SoftwareWire.h>  // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

SoftwareWire myWire(SDA, SCL);
RtcDS3231<SoftwareWire> Rtc(myWire);

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
  Serial.begin(115200);

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    } else {
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");

      // following line sets the RTC to the date & time this sketch was compiled
      // it will also reset the valid flag internally unless the Rtc device is
      // having an issue

      Rtc.SetDateTime(compiled);
    }
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

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

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
