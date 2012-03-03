/*
  SD card datalogger

 This example shows how to log data from three analog sensors
 to an SD card using the SD library.

 The circuit:
 * analog sensors on analog ins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 */
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "logger.h"

const int  cs=48; //chip select 

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.

// Configurations
const int chipSelect = 8;
const int nbr_sensors = 1;
const unsigned int pin_reads_per_stat = 255;
const unsigned int stats_per_logentry = 100000;
const int pins[nbr_sensors] = {0};  // Analog pins
unsigned int buffer[pin_reads_per_stat];  // Buffer for pin reads

//=====================================
void setup()
{
  Serial.begin(9600);
  /*
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");*/
  
  // Writer Initialization
  Wire.begin(); // join i2c bus (address optional for master)
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
void loop()
{
  keepAlive();

  // Loop over all analog pins
  for (int p = 0; p <= nbr_sensors; p++) {
    int pin = pins[p];

    // Compute min, max and avg for the collected data
    Stat final;
    unsigned long sum = 0;
    long i;
    for (i = 0; i < stats_per_logentry; i++) {
      int stat = analogRead(pin);
      if (stat < final.min) {
        final.min = stat;
      }
      if (stat > final.max) {
        final.max = stat;
      }
      if (i%1000 == 0) {
        Serial.println(i);
      }
      sum += stat;
    }
    Serial.println("i: " + String(i));
    final.avg = sum/stats_per_logentry;
    
    // Let's log this
    log_pin_stat(pin, final);
  }
}

  /*
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(filename, FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening file");
  }*/

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
  //Wire.write('\n');
  Wire.write(';');
  Wire.endTransmission();
}

//=====================================
void writeString(int address, String message)
{
  char msg[message.length()+1];
  message.toCharArray(msg, message.length()+1);
 
  int ix = 0;
  int maxIx = strlen(msg)/sizeof(char);
  
  while(ix < maxIx) {
 
    Wire.beginTransmission(address);
    for (int i = 0; i < 32 && ix < maxIx; i++) {
        Wire.write(msg[ix]);
        ix++;
    }
    Wire.endTransmission();
  }
  writeLineEnding(address);
}

//=====================================
int RTC_init(){ 
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
int SetTimeDate(int d, int mo, int y, int h, int mi, int s){ 
  int TimeDate [7]={s,mi,h,0,d,mo,y};
  for(int i=0; i<=6;i++){
    if(i==3)
      i++;
    int b= TimeDate[i]/10;
    int a= TimeDate[i]-b*10;
    if(i==2){
      if (b==2)
        b=B00000010;
      else if (b==1)
        b=B00000001;
    } 
    TimeDate[i]= a+(b<<4);
      
    digitalWrite(cs, LOW);
    SPI.transfer(i+0x80); 
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
String ReadTimeDate(){
  String temp;
  int TimeDate [7]; //second,minute,hour,null,day,month,year    
  for(int i=0; i<=6;i++){
    if(i==3)
      i++;
    digitalWrite(cs, LOW);
    SPI.transfer(i+0x00); 
    unsigned int n = SPI.transfer(0x00);        
    digitalWrite(cs, HIGH);
    int a=n & B00001111;    
    if(i==2){ 
      int b=(n & B00110000)>>4; //24 hour mode
      if(b==B00000010)
        b=20;        
      else if(b==B00000001)
        b=10;
      TimeDate[i]=a+b;
    }
    else if(i==4){
      int b=(n & B00110000)>>4;
      TimeDate[i]=a+b*10;
    }
    else if(i==5){
      int b=(n & B00010000)>>4;
      TimeDate[i]=a+b*10;
    }
    else if(i==6){
      int b=(n & B11110000)>>4;
      TimeDate[i]=a+b*10;
    }
    else{ 
      int b=(n & B01110000)>>4;
      TimeDate[i]=a+b*10; 
      }
  }

  String str;
  str = AddLeadingZeroes(TimeDate[6]);
  temp.concat(str);
  temp.concat("/") ;
  str = AddLeadingZeroes(TimeDate[5]);
  temp.concat(str);
  temp.concat("/") ;
  str = AddLeadingZeroes(TimeDate[4]);
  temp.concat(str);
  temp.concat("    ") ;
  str = AddLeadingZeroes(TimeDate[2]);
  temp.concat(str);
  temp.concat(":") ;
  str = AddLeadingZeroes(TimeDate[1]);
  temp.concat(str);
  temp.concat(":") ;
  str = AddLeadingZeroes(TimeDate[0]);
  temp.concat(str);

  return(temp);
}