/* 
WDMES I2C L.E.D. CONTROLLER
---------------------------
By Simon Dawes
---------------------------
This uses an I2C I/O Expansion board based on the PCF8574 IC
Default I2C address is 0x20 (all address switches set to 0)
SCl and SDA need pull up resisters ~4.7kOhm - 10kOhm
SDA to Pin A4 of Arduino nano
SCL to Pin A5 of Arduino nano
the PCF8574 GPIO board needs 5v

PCF8574 Sw Config
Pin 1 | Pin 2  | Pin 3 |Address
off     off      off     0x20
off     off      on      0x21

Currently supports 1 I2C Board - it will support more in the future
timing is scaled to OO guage ~1/76.1 (only roughly as Ive rounded quite a bit!)
LED's wired to 5v PCF8574 switches then to ground
*/
//  FILE:WDMES_LEDLIGHTS_CONTROL.ino

#include <Wire.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>

//const int PCF8574_address = 0x20;
PCF8574 PCF_StreetLights(0x20);    // this is the address for the street lights (up to 8 lights)
PCF8574 PCF_BuildingLights(0x21);  // this is the address for the building lights (up to 8 lights)
PCF8574 PCF_PlatformLights(0x22);  //this is the address for the platform lights (up to 4 platforms, the rest will be used for special lights)

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address and the number of columns and rows

// set below the digital pin to use to pause time
#define PausePin 6  //D6 Pauses when Pin is high (5v)

int CurrentTime = 0;  //holds the current hour the model is emulating
int CurrentDay = 0;   //holds the current day of the week
String TimeStatus = "Running";
const String DayOfWeek[7] = { "Monday     ", "Tuesday    ", "Wednesday  ", "Thursday   ", "Friday     ", "Saturday   ", "Sunday     " };
bool Pause = false;
// Change values of below for pull up or pull down on the LED's, the WDMES 00 guage model is LOW (0) = LED ON
const int ON = 0;
const int OFF = 1;

/*Scale is the amount of real time to pass in ms for 1 hour of  model time to pass, 
for an authentic experience scale should be set to 47000
i.e 47s in the model is one hour or 1 day in the model is 18.8 minutes
*/
const double Scale = 200;  //Change to 3917 (this is equvilent to simulating a '5 minute' cadence in the model. (was a 1hr cadence) 47000 to scale with OO Gauge

//
// NO MORE CONFIGURATION BEYOND THIS POINT
//

void setup() {
  // Start i2C
  Wire.begin();

  // Start PCF8574
  PCF_StreetLights.begin();
  PCF_BuildingLights.begin();
  PCF_PlatformLights.begin();

  // Start LCD
  lcd.begin(16, 2);
  lcd.backlight();
  // Start Serial Port
  Serial.begin(9600);
  pinMode(PausePin, INPUT_PULLUP);  // Set digital pin used for - PAUSE TIMW - to input

  //perform a test on the connected PCF devices (o/p to serial port)
  PCFTest();

  //perform a start-up sequence
  
  lcdTest();
  //LightsTest();
  //message();
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(TimeFormat(CurrentTime));
  lcdPrint(DayOfWeek[CurrentDay] + TimeFormat(CurrentTime), 0);
  lcd.setCursor(9, 1);
  lcd.print(TimeStatus);

  //Serial.print("CurrentDay: ");
  //Serial.println(DayOfWeek[CurrentDay]);
  //get the 'models' time
  if (Pause == false) {
    switch (CurrentTime) {

      case 0:  //Night   00:00 - 07:00
        lcdPrint("Night    ", 1);
        StreetLights(ON);
        BuildingLights(OFF);
        PlatformLights(ON);

        break;

      case 84:  //Dawn    07:00-08:00

        lcdPrint("Dawn     ", 1);
        StreetLightsTwilight(OFF);
        PlatformLightsTwilight(OFF);

        break;

      case 96:  //Day     08:00 - 18:00
        lcdPrint("Day      ", 1);
        LightsDay();
        break;

      case 216:  //Dusk    18:00-19:00

        lcdPrint("Dusk     ", 1);
        StreetLightsTwilight(ON);
        PlatformLightsTwilight(ON);
        BuildingLightsTwilight(ON);

        break;

      case 228:  //Evening 19:00 - 23:00
        lcdPrint("Evening  ", 1);
        LightsEvening();
        break;

      case 276:  //Bedtime 23:00 - 00:00
        lcdPrint("Bedtime  ", 1);
        LightsBedtime();
        break;
    }
  }
  delay(Scale);

  Pause = digitalRead(PausePin);
  if (Pause == false) {
    // if Pause is false then increment time otherwise hour stays the same effectivly pausing time
    CurrentTime++;
    TimeStatus = "Running";
  } else {
    Serial.println("Time Paused");
    TimeStatus = " Paused";
  }
  if (CurrentTime == 288) {
    //reset Start Time for model midnight
    CurrentTime = 0;
    CurrentDay++;
    if (CurrentDay == 7) {
      CurrentDay = 0;
    }
    Serial.print("Day ");
    Serial.println(DayOfWeek[CurrentDay]);
    Serial.println("Midnight");
  }
}

//----------------------------------------------------------------------------------------------

void LightsDay() {
  //This is the Daytime Lighting Sequence for all lights
  StreetLights(OFF);
  BuildingLights(OFF);
  PlatformLights(OFF);
  //
  switch (CurrentDay) {
    case 1:  //Tuesday
      Serial.println("Welder On");
      break;

    case 3:  //Thursday
      Serial.println("Welder On");
      break;

    case 6:  //Sunday
      Serial.println("Welder On");
      break;
  }
}

void StreetLightsTwilight(int Status) {
  //This is the Daytime Lighting Sequence
  int i = 0;
  int StreetLightNo;
  do {
    //this loop continues until all building lights are on
    //choose a building to light
    StreetLightNo = random(8);
    if (PCF_StreetLights.read(StreetLightNo) != Status) {
      //Light off so turn on
      StreetLight(StreetLightNo, Status);
      i++;
      Serial.print("Streetlight ");
      Serial.print(i);
      Serial.println("/8");
      //delay turning on next light by a random amount
      delay(random(100, 2000));
    }

  } while (i < 8);  //exit when all light are on
}

void BuildingLightsTwilight(int Status) {
  //This is the Daytime Lighting Sequence
  int i = 0;
  int BuildingLightNo;
  do {
    //this loop continues until all building lights are on
    //choose a building to light
    BuildingLightNo = random(8);
    if (PCF_BuildingLights.read(BuildingLightNo) != Status) {
      //Light off so turn on
      BuildingLight(BuildingLightNo, Status);
      i++;
      Serial.print("Buildinglight ");
      //Serial.print(Status);
      Serial.print(i);
      Serial.println("/8");
      //delay turning on next light by a random amount
      delay(random(100, 2000));
    }

  } while (i < 8);  //exit when all light are on

  //
  Serial.println("Twilight: Turn Welder Off");
}

void PlatformLightsTwilight(int Status) {
  //This is the Daytime Lighting Sequence
  int i = 0;
  do {
    //this loop continues until all platform lights are on
    //choose a building to light
    PlatformLight(i, Status);
    delay(1000);
    i++;
  } while (i < 4);  //exit when all light are on
}

void LightsEvening() {
  //This is the Evening Lighting Sequence
  //Street Lights On
  StreetLights(ON);
  //Building Lights On
  BuildingLights(ON);

  switch (CurrentDay) {

    case 3:  //Thursday
      Serial.println("Welder On");
      Serial.println("Club house light on");
      break;
  }
}
void LightsBedtime() {
  //This is the nighttime Lighting Sequence
  //StreetLights ON
  StreetLights(ON);
  PlatformLights(ON);
  BuildingLightsTwilight(OFF);  // un-comment when PCF8574 is added
                                //Building Lights Off
                                //
  Serial.println("Welder Off");
}

void StreetLights(int Status) {
  //Turns street lights on or off
  for (int i = 0; i <= 7; i++) {
    StreetLight(i, Status);
  }
}

void BuildingLights(int Status) {
  //Turns building lights on or off
  for (int i = 0; i <= 7; i++) {
    BuildingLight(i, Status);
  }
}
void PlatformLights(int Status) {
  //Turns building lights on or off
  for (int i = 0; i <= 4; i++) {
    PlatformLight(i, Status);
  }
}

void StreetLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either LOW, HIGH, ON or OFF
  PCF_StreetLights.write(LED, Status);
}

void BuildingLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either LOW, HIGH, ON or OFF
  PCF_BuildingLights.write(LED, Status);
}

void PlatformLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either LOW, HIGH, ON or OFF
  PCF_PlatformLights.write(LED, Status);
}

void AllLEDOn() {
  for (int i = 0; i <= 7; i++) {
    PCF_StreetLights.write(i, ON);
    PCF_BuildingLights.write(i, ON);
    PCF_PlatformLights.write(i, ON);
  }
}

void AllLEDOff() {
  for (int i = 0; i <= 7; i++) {
    PCF_StreetLights.write(i, OFF);
    PCF_BuildingLights.write(i, OFF);
    PCF_PlatformLights.write(i, OFF);
  }
}
void PCFTest() {
  Serial.println(__FILE__);
  Serial.print("PCF8574_LIB_VERSION:\t");
  Serial.println(PCF8574_LIB_VERSION);
  // check we can see the I2C PCF8574 module return result on serial port
  if (!PCF_StreetLights.begin()) {
    Serial.println("could not initialize...");
  }

  if (!PCF_StreetLights.isConnected()) {
    Serial.println("=> not connected");
  } else {
    Serial.println("=> connected!!");
  }
}

void LightsTest() {
  Serial.println("Lights Test Starts");
  for (int i = 0; i <= 2; i++) {
    AllLEDOn();
    delay(1000);
    AllLEDOff();
    delay(1000);
  }
  for (int i = 0; i <= 4; i++) {
    AllLEDOn();
    delay(500);
    AllLEDOff();
    delay(500);
  }
  for (int i = 0; i <= 9; i++) {
    AllLEDOn();
    delay(200);
    AllLEDOff();
    delay(200);
  }
  for (int i = 0; i <= 9; i++) {
    AllLEDOn();
    delay(50);
    AllLEDOff();
    delay(50);
  }
  Serial.println("Lights Test Ends");
}

void message() {
  //Test LED Message in More Code :-)
  Serial.println("Message Starts");
  int myValues[] = {
    1, -1, 3, -1, 3, -1, 3, -1, 3, -1, 3, -1, 1, -1, 1, -1, 1, -1, 1, -3,
    1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -3,
    3, -1, 3, -1, 1, -1, 1, -1, 3, -1, 1, -1, 1, -1, 3, -1, 3, -1, 3, -1, 1, -1, 1, -3,
    1, -1, 3, -1, 1, -1, 1, -1, 3, -1, 1, -1, 1, -1, 1, -1, 3, -1, 1, -1, 1, -1, 1, -1, 3, -1, 3, -1, 1, -1, 3, -1, 3, -1, 1, -1, 3, -1, 3, -1
  };
  //Serial.print("Size of array ");
  //Serial.print(sizeof(myValues));
  // this for loop works correctly with an array of any type or size
  for (byte i = 0; i < (sizeof(myValues) / sizeof(myValues[0])); i++) {
    // do something with myValues[i]
    if (myValues[i] > 0) {
      AllLEDOn();
    } else {
      AllLEDOff();
    }
    delay(abs(myValues[i]) * 100);
  }
  Serial.println("Message ends");
}

String TimeFormat(int timeIndex) {
  int Hour = abs(timeIndex/12);
  int Minute =(timeIndex-(Hour*12))*5;
  String FormattedTime;
  if (Hour < 10) {
    FormattedTime = "0" + String(Hour) + ":00";
  } else {
    FormattedTime = String(Hour) + ":00";
  }
  if (Minute <10){
    FormattedTime=FormattedTime+":05"; 
  } else{
    FormattedTime=FormattedTime+":"+Minute;
  }
  return FormattedTime;
}

void lcdTest() {
  lcd.setCursor(0, 0);  // set the cursor to column 0, line 0
  lcd.print("W.D.M.E.S.");
  lcd.setCursor(0, 1);  // set the cursor to column 0, line 1
  lcd.print("OO Gauge Railway");
  //delay(2000);
  lcd.clear();  // Clears the  display
}
void lcdPrint(String message, int line) {
  lcd.setCursor(0, line);  // set the cursor to column 0, line 0
  lcd.print(message);
}