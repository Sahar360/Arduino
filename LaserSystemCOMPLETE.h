#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <SD.h>
#include <SPI.h>

#define I2C_ADDR    0x3F // LCD Adress
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin); // Library components.

unsigned long previousMillis = 0;
const long interval = 300000;
const int ledPin = 9;
const int sensor = 7;
const int buzzer = 8;
boolean sig = 0;
boolean state = 0;
int CS_PIN = 4; // pin for SD logger.
int elec;

File file; // file instance for SD logger.

void setup()
{
  Serial.begin(9600);
  lcd.begin (16,2);
  pinMode(ledPin, OUTPUT);
  pinMode(sensor, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  
  // Switch on the backlight
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home (); // go home
}

void loop()
{
  elec = getMaxValue(); // get electricity value (analog read).
  Serial.println(elec);

  sig = digitalRead(sensor); // read binary sensor.
  digitalWrite(ledPin, sig -1); // set led to the opposite of binary sensor value.
  
  tmElements_t tm; // Set RTC.

    if (RTC.read(tm))  // display RTC on screen
  {
    lcd.setCursor (0,1); // go to start of 1st line.
    lcd.print(tm.Hour);
    lcd.print(':');
    lcd.print(tm.Minute);
  lcd.print("|");
  lcd.print(tm.Day);
  lcd.print('/');
  lcd.print(tm.Month);
  lcd.print('/');
  lcd.print(tmYearToCalendar(tm.Year));
  }
    
  unsigned long currentMillis = millis();
  if (sig == 0 && elec > 1000) // condition 1: laser and blower ON. (On duty).
  {
  previousMillis = currentMillis;
  lcd.setCursor (0,0);
    lcd.print("On duty.         "); 
  noTone(buzzer);
  state = 0;
  }

  else if (sig == 0 && elec < 1000) // condition 2: laser OFF blower ON. (Turn the blower OFF).
  {
  if (currentMillis - previousMillis >= interval)
  {
    lcd.setCursor (0,0);
    lcd.print("Close the blower");
    tone(buzzer, 1000, 800);
    if (RTC.read(tm) && state == 0) 
    {
	while (!Serial) ; // wait for serial
	initializeSD();
    file = SD.open("report.txt", FILE_WRITE);
    file.println("Alarm occured at:");
    file.print(tm.Hour);
    file.print(':');
    file.print(tm.Minute);
    file.print("|");
    file.print(tm.Day);
    file.print('/');
    file.print(tm.Month);
    file.print('/');
    file.print(tmYearToCalendar(tm.Year));
    file.close();
	state = 1;
    }
  }
  }
  else if (sig == 1 && elec > 1000) // condition 3: laser ON, blower OFF. (Turn the blower ON).
  {
  previousMillis = currentMillis;
  lcd.setCursor (0,0);
  lcd.print("Open the blower");
  tone(buzzer, 1000, 80);
  state = 0;
  }
  else // condition 4: Laser OFF, Blower OFF. (Idle).
  {
    previousMillis = currentMillis;
  lcd.setCursor (0,0);
  lcd.print("Idle.           ");
  noTone(buzzer);
  state = 0;
  }
}

int getMaxValue() // fuction by Elecrow for measring AC.
{
  int sensorValue;             //value read from the sensor
  int sensorMax = 0;
  uint32_t start_time = millis();
  while((millis()-start_time) < 1000)//sample for 1000ms
  {
    sensorValue = analogRead(0);
    if (sensorValue > sensorMax) 
    {
      /*record the maximum sensor value*/
      sensorMax = sensorValue;
    }
  }
  return sensorMax;
}

void initializeSD()
{
  pinMode(CS_PIN, OUTPUT);
  SD.begin();
}

int createFile(char filename[]) // make a new file, or open an existing in the SD.
{
  file = SD.open(filename, FILE_WRITE);
}

void closeFile()
{
    file.close();
}