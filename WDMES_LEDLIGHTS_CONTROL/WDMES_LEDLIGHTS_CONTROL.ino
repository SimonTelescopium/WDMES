#pragma region INTRODUCTION
/* 
  WDMES I2C L.E.D. CONTROLLER
  ---------------------------
  AUTHOR:Simon Dawes (with help from Thomas)
  ---------------------------
  This uses a I2C I/O Expansion boards based on the PCF8574 IC
  Default I2C address is 0x20 (all address switches set to 0)
  SCl and SDA need pull up resisters ~4.7kOhm - 10kOhm
  SDA to Pin A4 of Arduino nano
  SCL to Pin A5 of Arduino nano
  the PCF8574 GPIO board needs 5v

  PCF8574 DIP Sw Config
  Pin 1 | Pin 2  | Pin 3 |Address
  off     off      off     0x20
  off     off      on      0x21
  off     on       off     0x22
  off     on       on      0x23

  LED's wired to 5v PCF8574 o/p then to ground

  Controls
  --------
  Controls consist of a 4 way rotary switch connected to a PCF8574 i/o expansion board, communicating over I2C
  - Common is connected to p0 
  - Day selection is connected to p1
  - Evening selection is connected to p2
  - Night selection is connected to p3

  User feedback is via a 20x4 LCD screen communicating over I2C
  In this way the controlls can be seperate from the main lighting circuit
  */
//  FILE:WDMES_LEDLIGHTS_CONTROL.ino

#pragma endregion

#pragma region SETUP_LIBS_GLOBALS
#include <Wire.h>               //I2C lib
#include <PCF8574.h>            //IO extender lib
#include <LiquidCrystal_I2C.h>  //LCD lib
#include <math.h>

//const int PCF8574_address = 0x20;
PCF8574 PCF_AuxiliaryLights(0x20);  // this is the address for the Auxiliary lights (up to 8 lights)
PCF8574 PCF_BuildingLights(0x21);   // this is the address for the building lights (up to 8 lights)
PCF8574 PCF_PlatformLights(0x22);   // this is the address for the platform lights (up to 4 platforms, the rest will be used for special lights)
PCF8574 PCF_Controls(0x23);         // this is the PCF GPIO board in the controll box.

LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address and the number of columns and rows, ths is the control box LCD

int CurrentTime = 0;  //holds the current hour the model is emulating
int CurrentDay = 0;   //holds the current day of the week
String TimeStatus = " Running";
bool Pause = false;
const String DayOfWeek[7] = {
  "Monday         ",
  "Tuesday        ",
  "Wednesday      ",
  "Thursday       ",
  "Friday         ",
  "Saturday       ",
  "Sunday         "
};

// Change values of below for pull up or pull down on the LED's, the WDMES 00 guage model is LOW (0) = LED ON
const int ON = 1;
const int OFF = 0;

// declare global arrays for LED states
int BuildingLEDStatus[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int AuxiliaryLEDStatus[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int PlatformLEDStatus[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int RealTime = false;

const double ReturnScale = 300;  //Change to 3947 (this is equvilent to simulating a '5 minute' cadence in the model at OO gauge 1/76)
double Scale = ReturnScale;

void SpeedUp(int, int = -1);  // Sets default parameters for SpeedUp function
#pragma endregion
//
//    ------------------------------ NO MORE CONFIGURATION BEYOND THIS POINT -------------------------------------
//

void setup() {

  Serial.begin(9600);  // Start Serial Port

  int i = 30;  // 30 second delay due to LCD power up delay
  Serial.print("Please Wait [");
  do {

    //Serial.print(i);
    Serial.print(".");
    delay(1000);
    i--;
  } while (i > 0);
  Serial.println("]");
  Wire.begin();                 // Start i2C
  PCF_AuxiliaryLights.begin();  // Start PCF8574 for AuxiliaryLights board
  PCF_BuildingLights.begin();   // Start PCF8574 for BuildingLights board
  PCF_PlatformLights.begin();   // Start PCF8574 for PlatformLights board
  PCF_Controls.begin();         // Start PCF8574 for Control board


  lcd.begin(20, 4);  // Start LCD
  lcd.backlight();   //turn on back light


  PCFTest();                 ////perform a test on the connected PCF devices (o/p to serial port) test PCF output result on serial port
  PCF_Controls.write(0, 0);  //set pin 0 of control board this is the common for the control dial so we can detect the position the rotary switch is in
  lcdStart();
}

void loop() {

  SendStatus();  //print status of all LED's on serial port
  lcd.setCursor(0, 2);
  lcd.print(DayOfWeek[CurrentDay] + TimeFormat(CurrentTime));
  lcd.setCursor(12, 3);
  lcd.print(TimeStatus);

  switch (CurrentTime) {

    case 0:  //Night   00:00 - 07:00
      lcdStart();
      lcd.setCursor(0, 3);
      lcd.print("Night        ");
      //AuxiliaryLights(ON);
      BuildingLights(OFF);
      PlatformLights(ON);
      break;

    case 84:  //Dawn    07:00-08:00
      lcdStart();
      lcd.setCursor(0, 3);
      lcd.print("Dawn         ");
      //AuxiliaryLightsTwilight(OFF);
      PlatformLightsTwilight(OFF);
      break;

    case 96:  //Day     08:00 - 18:00
      lcdStart();
      lcd.setCursor(0, 3);
      lcd.print("Day          ");
      LightsDay();
      break;

    case 150:  //Clubhouse empties 12:30
      ClubHouseLights(OFF);
      break;

    case 216:  //Dusk    18:00-19:00 (216)
      lcdStart();
      lcd.setCursor(0, 3);
      lcd.print("Dusk         ");
      //AuxiliaryLightsTwilight(ON);
      PlatformLightsTwilight(ON);
      BuildingLightsTwilight(ON);
      break;

    case 228:  //Evening 19:00 - 23:00
      lcdStart();
      lcd.setCursor(0, 3);
      lcd.print("Evening      ");
      LightsEvening();
      break;

    case 276:  //Bedtime 23:00 - 00:00
      lcdStart();
      lcd.setCursor(0, 3);
      lcd.print("Bedtime      ");
      LightsBedtime();
      break;
  }

  if (RealTime == false) {
    delay(Scale);  // 5 minute delay to scale
  } else {         // add more frequent checks in case mode changes, maybe code to increment time by 1 minute.
    delay(Scale);
  };

  // // HACK to flash Platform Lights
  // AuxiliaryLight(1,ON);
  // BuildingLights(ON);
  // delay(1000);
  // PlatformLights(OFF);
  // AuxiliaryLights(OFF);
  //. delay(1000);

  if (Pause == false) {  // if Pause is false then increment time otherwise hour stays the same effectivly pausing time
    CurrentTime++;
    TimeStatus = " Running";
  } else {
    Serial.println("Time Paused");
    TimeStatus = "  Paused";
  }

  CheckControls();  //  check state of controls

  if (CurrentTime == 288) {  //reset Start Time for model midnight
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
void SendStatus() {
  // This function presents the full status of the lights on the serial port
  String Status = DayOfWeek[CurrentDay] + TimeFormat(CurrentTime) + " Building:[";
  for (int i = 0; i < 8; i++) {
    Status = Status + BuildingLEDStatus[i];
  }
  Serial.print(Status + "]");

  Status = " AuxiliaryLights:[";
  for (int i = 0; i < 8; i++) {
    Status = Status + AuxiliaryLEDStatus[i];
  }
  Serial.print(Status + "]");

  Status = " Platform:[";


  for (int i = 0; i < 6; i++) {
    Status = Status + PlatformLEDStatus[i];
  }
  Status = Status + "]";

  Serial.print(Status);
  Status = "";
  Status = Status + " ClubHouse Light [" + PlatformLEDStatus[6];
  Status = Status + "] Welder [" + PlatformLEDStatus[7] + "]";

  Serial.println(Status);
}

void LightsDay() {
  //This is the Daytime Lighting Sequence for all lights
  AuxiliaryLights(OFF);
  BuildingLights(OFF);
  PlatformLights(OFF);
  //
  switch (CurrentDay) {
    case 1:  //Tuesday
      //Serial.println("Clubhouse: On");
      ClubHouseLights(ON);
      break;

    case 3:  //Thursday
      //Serial.println("Clubhouse: On");
      ClubHouseLights(ON);
      break;

    case 6:  //Sunday
      //Serial.println("Clubhouse: On");
      ClubHouseLights(ON);
      break;
  }
}

void BuildingLightsTwilight(int Status) {
  //This is the Daytime Lighting Sequence
  int i = 0;
  int BuildingLightNo;
  int TimeAccumulator;  // holds how long has been spent in this routine since last time update
  int RandomDelay;      //generates the random delay
  do {
    //this loop continues until all building lights are on
 
    BuildingLightNo = random(8);   //choose a building to light
    if (BuildingLEDStatus[BuildingLightNo] != Status) {
      //Light off so turn on
      BuildingLight(BuildingLightNo, Status);
      i++;
      //delay turning on next light by a random amount
      RandomDelay = (random(Scale / 30, Scale / 2));
      TimeAccumulator = TimeAccumulator + RandomDelay;

      delay(RandomDelay);

      if (TimeAccumulator >= Scale) {
        CurrentTime++;  // update time
        lcd.setCursor(0, 2);
        lcd.print(DayOfWeek[CurrentDay] + TimeFormat(CurrentTime));
        TimeAccumulator = 0;
      }
    }

  } while (i < 8);  //exit when all light are on
  AuxiliaryLight(5, Status); // turn on RHS Signal Box Light
}

void PlatformLightsTwilight(int Status) {
  //This is the Daytime Lighting Sequence
  int i = 0;
  do {
    //this loop continues until all platform lights are on
    //choose a building to light
    PlatformLight(i, Status);
    delay(Scale / 4);

    i++;
  } while (i <= 7);  //exit when all light are on
  CurrentTime++;     // update time
  //hack for platform D
  delay(1000);
  AuxiliaryLight(0, Status); // turn on/off Plat.D LHS Light
  delay(500);
  AuxiliaryLight(4, Status); // Turn on/off Plat.D RHS light

  lcd.setCursor(0, 2);
  lcd.print(DayOfWeek[CurrentDay] + TimeFormat(CurrentTime));
}

void LightsEvening() {
  //This is the Evening Lighting Sequence
  BuildingLights(ON);
  switch (CurrentDay) {
    case 3:  //Thursday
      ClubHouseLights(ON);
      break;
  }
}

int CheckControls() {
  // If the PCF Control Board is not connected, the arduino tries to reconnect and time returns to automatic.
  if (!PCF_Controls.isConnected()) {
    AutomaticTime();
    return;
  }
  // this routine reads the controls and returns a new value for CurrentTime
  PCF_Controls.write(0, 0);
  int i = 2;
  RealTime = false;
  do {
    if (PCF_Controls.read(i) == 0) {
      // found switch position
      switch (i) {
        case 2:  //Day - 10am
          Serial.print("Fast Fwd to 'Day'");
          SpeedUp(120);
          return;
        case 3:  //Evening - 10pm
          Serial.print("Fast Fwd to 'Evening'");
          SpeedUp(264);
          return;
        case 4:  // Midnight - 12am
          Serial.print("Fast Fwd to 'Midnight'");
          SpeedUp(288, 0);
          return;
        case 5:  // Real Time
          return;
      }
    }
    i++;
  } while (i < 6);
  AutomaticTime();
  return;
}

void AutomaticTime() {
  Scale = ReturnScale;
  Pause = false;
}

void SpeedUp(int FirstTime, int SecondTime) {
  if (CurrentTime == FirstTime || CurrentTime == SecondTime) {
    Scale = ReturnScale;
    Pause = true;
  } else {
    Scale = 1;
    Pause = false;
  }
}

void LightsBedtime() {
  //This is the nighttime Lighting Sequence
  PlatformLights(ON);
  BuildingLightsTwilight(OFF);
  ClubHouseLights(OFF);
}

void AuxiliaryLights(int Status) {
  //Turns Auxiliary lights on or off
  for (int i = 0; i <= 7; i++) {
    AuxiliaryLight(i, Status);
    AuxiliaryLEDStatus[i] = Status;
  }
}

void BuildingLights(int Status) {
  //Turns building lights on or off
  for (int i = 0; i <= 7; i++) {
    BuildingLight(i, Status);
    BuildingLEDStatus[i] = Status;
  }
}

void PlatformLights(int Status) {
  //Turns building lights on or off
  for (int i = 0; i <= 7; i++) {
    PlatformLight(i, Status);
    PlatformLEDStatus[i] = Status;
    delay(500);
  }
  //add controls for Platform D
  delay(1000);
  AuxiliaryLight(0, Status);
  delay(500);
  AuxiliaryLight(4, Status);
}

void ClubHouseLights(int Status) {
  // clubhouse
  PCF_AuxiliaryLights.write(1, Status);  // Pin 2, (port 1 in code) on the PCF8574 board should be connected to the clubhouse.
  AuxiliaryLEDStatus[1] = Status;
  // welder
  PCF_AuxiliaryLights.write(7, Status);
  AuxiliaryLEDStatus[7] = Status;
}

void AuxiliaryLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either LOW, HIGH, ON or OFF
  PCF_AuxiliaryLights.write(LED, Status);
  AuxiliaryLEDStatus[LED] = Status;
  // Serial.print("Auxiliary Light ");
  // Serial.print(LED);
  // if (Status == ON){
  // Serial.println(" On");
  // } else {
  //   Serial.println(" Off");
  // }
}

void BuildingLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either ON or OFF
  PCF_BuildingLights.write(LED, Status);
  BuildingLEDStatus[LED] = Status;
}

void PlatformLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either ON or OFF
  PCF_PlatformLights.write(LED, Status);
  PlatformLEDStatus[LED] = Status;
}

void AllLEDOn() {
  for (int i = 0; i <= 7; i++) {
    PCF_AuxiliaryLights.write(i, ON);
    AuxiliaryLEDStatus[i] = ON;
    PCF_BuildingLights.write(i, ON);
    BuildingLEDStatus[i] = ON;
    PCF_PlatformLights.write(i, ON);
    PlatformLEDStatus[i] = ON;
  }
  Serial.println("All Lights On");
}

void AllLEDOff() {
  for (int i = 0; i <= 7; i++) {
    PCF_AuxiliaryLights.write(i, OFF);
    AuxiliaryLEDStatus[i] = OFF;
    PCF_BuildingLights.write(i, OFF);
    BuildingLEDStatus[i] = OFF;
    PCF_PlatformLights.write(i, OFF);
    PlatformLEDStatus[i] = OFF;
  }
  Serial.println("All Lights Off");
}

void PCFTest() {
  Serial.println(__FILE__);
  Serial.print("PCF8574_LIB_VERSION:\t");
  Serial.println(PCF8574_LIB_VERSION);
  // check we can see the I2C PCF8574 module return result on serial port
  if (!PCF_AuxiliaryLights.begin()) {
    Serial.println("could not initialize...");
  }

  if (!PCF_AuxiliaryLights.isConnected()) {
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
  int Hour = abs(timeIndex / 12);
  int Minute = (timeIndex - (Hour * 12)) * 5;
  String FormattedTime;
  if (Hour < 10) {
    FormattedTime = "0" + String(Hour);
  } else {
    FormattedTime = String(Hour);
  }
  if (Minute < 5) {
    FormattedTime = FormattedTime + ":00";
  } else if (Minute < 10) {
    FormattedTime = FormattedTime + ":05";
  } else {
    FormattedTime = FormattedTime + ":" + Minute;
  }
  return FormattedTime;
}

void lcdStart() {
  Serial.println("Resetting LCD");
  lcd.init();
  lcd.setCursor(5, 0);  // set the cursor to column 5, line 1
  lcd.print("W.D.M.E.S.");
  lcd.setCursor(2, 1);  // set the cursor to column 2, line 2
  lcd.print("OO Gauge Railway");
}
