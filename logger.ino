#include <Wire.h>
#include <SPI.h>
#include "logger.h"

const int  cs = 48; //chip select
const unsigned int reads_per_sec = 8927;

// Configurations
const int nbr_sensors = 2;  // Must be the length of the pins array
const unsigned int seconds_per_logentry = 5;
const int pins[nbr_sensors] = {0, 1};  // Analog pins

//=====================================
void setup()
{
  Serial.begin(9600);

  // Writer Initialization
  Wire.begin();  // Join i2c bus (address optional for master)
  pinMode(13, OUTPUT);
  RTC_init();

  Serial.println("Logger started");
}

//=====================================
void log_pin_stat(int pin, Stat stat)
{
  String msg;
  String time = ReadTimeDate();

  msg = time + "    ";
  msg += "Pin (" + String(pin) + ") --";
  msg += "  min: " + String(stat.min);
  msg += "  max: " + String(stat.max);
  msg += "  avg: " + String(stat.avg);

  Serial.println(msg);
  writeString(4, msg);
}

//=====================================
Stat stat_pin(const int pin)
{
  Stat stat;
  int value, o;
  unsigned long sum, avg_sum = 0;

  for (int i = 0; i < seconds_per_logentry; i++) {
    /*
     * Sums up reads_per_sec reads and then calculates the avg to avoid
     * expensive divition.
     */
    sum = 0;
    for (o = 0; o < reads_per_sec; o++) {
      value = analogRead(pin);
      if (value < stat.min) {
        stat.min = value;
      } else if (value > stat.max) {
        stat.max = value;
      }
      sum += value;
    }
    avg_sum += int(sum / reads_per_sec);
  }
  stat.avg = int(avg_sum / seconds_per_logentry);
  return stat;
}

//=====================================
void loop()
{
  keepAlive();

  // Loop over all analog pins
  for (int p = 0; p < nbr_sensors; p++) {
    log_pin_stat(pins[p], stat_pin(pins[p]));
  }
}

//=====================================
void keepAlive()
{
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
}

//=====================================
void writeLineEnding(int address)
{
  Wire.beginTransmission(address);
  Wire.write(';');
  Wire.endTransmission();
}

//=====================================
void writeString(int address, String message)
{
  char msg[message.length() + 1];
  message.toCharArray(msg, message.length() + 1);

  int ix = 0;
  int maxIx = strlen(msg) / sizeof(char);

  while(ix < maxIx) {

    Wire.beginTransmission(address);
    for (int i = 0; i < 32 && ix < maxIx; i++) {
        Wire.write(msg[ix]);
        ix++;
    }
    Serial.println("1");
    Wire.endTransmission();
    Serial.println("2");
  }
  writeLineEnding(address);
}

//=====================================
int RTC_init()
{
    pinMode(cs,OUTPUT); // chip select
    // start the SPI library:
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE1); // both mode 1 & 3 should work
    //set control register
    digitalWrite(cs, LOW);
    SPI.transfer(0x8E);
    SPI.transfer(0x60); //60= disable Osciallator and Battery SQ wave @1hz, temp compensation, Alarms disabled
    digitalWrite(cs, HIGH);
    delay(10);
}

//=====================================
int SetTimeDate(int d, int mo, int y, int h, int mi, int s)
{
  int a,b;  // TODO: rename those to better names
  int TimeDate[7] = {s, mi, h, 0, d, mo, y};
  for (int i = 0; i <= 6; i++) {
    if (i == 3) {
      i++;
    }
    b = TimeDate[i] / 10;
    a = TimeDate[i] - (b * 10);
    if (i == 2) {
      if (b == 2) {
        b = B00000010;
      } else if (b == 1) {
        b = B00000001;
      }
    }
    TimeDate[i] = a + (b << 4);

    digitalWrite(cs, LOW);
    SPI.transfer(i + 0x80);
    SPI.transfer(TimeDate[i]);
    digitalWrite(cs, HIGH);
  }
}

//=====================================
String AddLeadingZeroes(int time)
{
  char buf[3];
  sprintf(buf, "%02d", time);
  return String(buf);
}

//=====================================
String ReadTimeDate()
{
  String temp;
  int a, b;  // TODO: rename those to better names
  int TimeDate[7];  // Second, minute, hour, null, day, month, year

  for (int i = 0; i <= 6; i++) {
    if (i == 3) {
      i++;
    }
    digitalWrite(cs, LOW);
    SPI.transfer(i + 0x00);
    unsigned int n = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    a = n & B00001111;
    if (i == 2) {
      b = (n & B00110000) >> 4;  // 24 hour mode
      if (b == B00000010) {
        b = 20;
      } else if (b == B00000001) {
        b = 10;
      }
      TimeDate[i] = a + b;
    } else if (i == 4) {
      b = (n & B00110000) >> 4;
      TimeDate[i] = a + b * 10;
    } else if (i == 5) {
      b = (n & B00010000) >> 4;
      TimeDate[i] = a + b * 10;
    } else if (i == 6) {
      b = (n & B11110000) >> 4;
      TimeDate[i] = a + b * 10;
    } else {
      b = (n & B01110000) >> 4;
      TimeDate[i] = a + b * 10;
    }
  }

  String str;
  str = AddLeadingZeroes(TimeDate[6]);
  temp.concat(str);
  temp.concat("/");
  str = AddLeadingZeroes(TimeDate[5]);
  temp.concat(str);
  temp.concat("/");
  str = AddLeadingZeroes(TimeDate[4]);
  temp.concat(str);
  temp.concat("    ");
  str = AddLeadingZeroes(TimeDate[2]);
  temp.concat(str);
  temp.concat(":");
  str = AddLeadingZeroes(TimeDate[1]);
  temp.concat(str);
  temp.concat(":");
  str = AddLeadingZeroes(TimeDate[0]);
  temp.concat(str);

  return(temp);
}
