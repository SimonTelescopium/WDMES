/* 
WDMES I2C L.E.D. CONTROLLER
---------------------------
AUTHOR:Simon Dawes
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

#include <Wire.h> //I2C lib
#include <PCF8574.h>  //IO extender lib
#include <LiquidCrystal_I2C.h>  //LCD lib
#include <RTClib.h>  // RTC lib
#include <math.h>

RTC_DS3231 rtc; 

//const int PCF8574_address = 0x20;
PCF8574 PCF_StreetLights(0x20);    // this is the address for the street lights (up to 8 lights)
PCF8574 PCF_BuildingLights(0x21);  // this is the address for the building lights (up to 8 lights)
PCF8574 PCF_PlatformLights(0x22);  // this is the address for the platform lights (up to 4 platforms, the rest will be used for special lights)
PCF8574 PCF_Controls(0x23);        // this is the PCF GPIO board in the controll box.

// set the LCD address and the number of columns and rows, ths is the control box LCD
LiquidCrystal_I2C lcd(0x27,20,4); 

//Real Time Clock Address 0x68
// set below the digital pin to use to pause time
//#define PausePin 6  //D6 Pauses when Pin is high (5v)

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
  "Sunday         " };

// Change values of below for pull up or pull down on the LED's, the WDMES 00 guage model is LOW (0) = LED ON
const int ON = 1;
const int OFF = 0;



// declare global arrays for LED states
int BuildingLEDStatus[8] = {0,0,0,0,0,0,0,0};
int StreetLEDStatus[8] = {0,0,0,0,0,0,0,0};
int PlatformLEDStatus[8] = {0,0,0,0,0,0,0,0};
int RealTime = false;

/*Scale is the amount of real time to pass in ms for 5 minmutes of  model time to pass, 
for an authentic experience scale should be set to 3947 i.e 47s in the model is one hour or 1 day in the model is 18.8 minutes
*/
const double ReturnScale = 300; //Change to 3947 (this is equvilent to simulating a '5 minute' cadence in the model at OO gauge 1/76)
double Scale = ReturnScale;  

void SpeedUp(int, int = -1); // Sets default parameters for SpeedUp function

//
//    ------------------------------ NO MORE CONFIGURATION BEYOND THIS POINT -------------------------------------
//

void setup() {
  // Start i2C
  Wire.begin();
  
  // Start PCF8574
  PCF_StreetLights.begin();

  PCF_BuildingLights.begin();
  PCF_PlatformLights.begin();
  PCF_Controls.begin();

  
  // Start LCD
  //lcd.begin(16, 2);
  lcd.init();
  //lcd.begin(20,4);
  lcd.backlight(); //turn on back light

  // Start Serial Port
  Serial.begin(9600);
  //pinMode(PausePin, INPUT_PULLUP);  // Set digital pin used to - PAUSE MODEL TIME - to input

  //SETUP RTC MODULE
  //if (! rtc.begin()) {
  //  Serial.println("Couldn't find RTC");
  //  Serial.flush();
  // while (1);
  //}
  
  rtc.begin();

  // automatically sets the RTC to the date & time on PC this sketch was compiled
  //
  // --------- UNCOMMENT the next line to program the correct time into the RTC 
  // then comment out and reupload code ----------
  //
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // manually sets the RTC with an explicit date & time, for example to set
  // January 21, 2021 at 3am you would call:
  // rtc.adjust(DateTime(2021, 1, 21, 3, 0, 0));




  //perform a test on the connected PCF devices (o/p to serial port)
  PCFTest();//test PCF output result on serial port
  PCF_Controls.write(0, 0); //set pin 0 of control board this is the common for the control dial so we can detect the position the rotary switch is in 
  lcdTest(); //perform a start-up sequence


  //lightsTest();//perform a simple lights test
  //message();  //flash a test message
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(TimeFormat(CurrentTime));
  // get RTCtime and store in variable

  //LCDPrintRealTime();


  lcd.setCursor(0, 2);
  lcd.print(DayOfWeek[CurrentDay] + TimeFormat(CurrentTime));
  //lcd.setCursor(9, 1);
  lcd.setCursor(12,3);
  lcd.print(TimeStatus);

  //Serial.print("CurrentDay: ");
  //Serial.println(DayOfWeek[CurrentDay]);
  //get the 'models' time
  //if (Pause == false) {
    switch (CurrentTime) {

      case 0:  //Night   00:00 - 07:00

        //lcdPrint("Night    ", 1);
        lcd.setCursor(0,3);
        lcd.print("Night        ");
        StreetLights(ON);
        BuildingLights(OFF);
        PlatformLights(ON);
        break;

      case 84:  //Dawn    07:00-08:00

        //lcdPrint("Dawn     ", 1);
        lcd.setCursor(0,3);
        lcd.print("Dawn         ");
        StreetLightsTwilight(OFF);
        PlatformLightsTwilight(OFF);
        break;

      case 96:  //Day     08:00 - 18:00

        //lcdPrint("Day      ", 1);
        lcd.setCursor(0,3);
        lcd.print("Day          ");
        LightsDay();
        break;

      case 150: //Clubhouse empties 12:30
        
        Serial.println("Clubhouse: Off"); // ClubHouseLights(ON);
        break;

      case 216:  //Dusk    18:00-19:00 (216)

        //lcdPrint("Dusk     ", 1);
        lcd.setCursor(0,3);
        lcd.print("Dusk         ");
        StreetLightsTwilight(ON);
        PlatformLightsTwilight(ON);
        BuildingLightsTwilight(ON);
        break;
      
      case 228:  //Evening 19:00 - 23:00

        //lcdPrint("Evening  ", 1);
        lcd.setCursor(0,3);
        lcd.print("Evening      ");
        LightsEvening();
        break;

      case 276:  //Bedtime 23:00 - 00:00

        //lcdPrint("Bedtime  ", 1);
        lcd.setCursor(0,3);
        lcd.print("Bedtime      ");
        LightsBedtime();
        break;
    }

  if (RealTime == false) {
    delay(Scale); // 5 minute delay to scale
  } else {
    // add more frequent checks in case mode changes, maybe code to increment time by 1 minute.
    delay(Scale);
  };


  //Pause = digitalRead(PausePin);
  if (Pause == false) { //changed from false
    // if Pause is false then increment time otherwise hour stays the same effectivly pausing time
    CurrentTime++;
    TimeStatus = " Running";
  } else {
    Serial.println("Time Paused");
    TimeStatus = "  Paused";
  }

  CheckControls();  //  check state of controls

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
      Serial.println("Clubhouse: On"); // ClubHouseLights(ON);
      break;

    case 3:  //Thursday
      Serial.println("Clubhouse: On"); // ClubHouseLights(ON);
      break;

    case 6:  //Sunday
      Serial.println("Clubhouse: On"); // ClubHouseLights(ON);
      break;
  }
}

void StreetLightsTwilight(int Status) {
  //This cxontrols the turning on and off of streetlights
  int i = 0;
  int StreetLightNo; //the number of the streetlight 
  int TimeAccumulator; // holds how long has been spent in this routine since last time update
  int RandomDelay; //generates the random delay
  do {
    //this loop continues until all building lights are on
    //choose a building to light
    StreetLightNo = random(8);
    //if (PCF_StreetLights.read(StreetLightNo) != Status) {

    if (StreetLEDStatus[StreetLightNo] != Status) {
      //Light off so turn on
      StreetLight(StreetLightNo, Status);
      i++;
      Serial.print("Streetlight ");
      Serial.print(i);
      Serial.println("/8");
      //delay turning on next light by a random amount
      RandomDelay=(random(Scale/30, Scale/2));
      TimeAccumulator = TimeAccumulator + RandomDelay;

      delay(RandomDelay);

      if (TimeAccumulator >= Scale) {
        CurrentTime++; // update time
        lcd.setCursor(0,2);
        lcd.print(DayOfWeek[CurrentDay] + TimeFormat(CurrentTime));
        TimeAccumulator = 0;

      }
    }

  } while (i < 8);  //exit when all light are on
}

void BuildingLightsTwilight(int Status) {
  //This is the Daytime Lighting Sequence
  int i = 0;
  int BuildingLightNo;
  int TimeAccumulator; // holds how long has been spent in this routine since last time update
  int RandomDelay; //generates the random delay
  do {
    //this loop continues until all building lights are on
    //choose a building to light
    BuildingLightNo = random(8);
    //if (PCF_BuildingLights.read(BuildingLightNo) != Status) {
    if (BuildingLEDStatus[BuildingLightNo] != Status) {
      //Light off so turn on
      BuildingLight(BuildingLightNo, Status);
      i++;
      Serial.print("Buildinglight ");
      //Serial.print(Status);
      Serial.print(i);
      Serial.println("/8");
      //delay turning on next light by a random amount
      //delay turning on next light by a random amount
      RandomDelay=(random(Scale/30, Scale/2));
      TimeAccumulator = TimeAccumulator + RandomDelay;

      delay(RandomDelay);

      if (TimeAccumulator >= Scale) {
        CurrentTime++; // update time
        lcd.setCursor(0,2);
        lcd.print(DayOfWeek[CurrentDay] + TimeFormat(CurrentTime));
        TimeAccumulator = 0;
      }
    }

  } while (i < 8);  //exit when all light are on

}

void PlatformLightsTwilight(int Status) {
  //This is the Daytime Lighting Sequence
  int i = 0;
  do {
    //this loop continues until all platform lights are on
    //choose a building to light
    PlatformLight(i, Status);
    delay(Scale/4);

    i++;
  } while (i < 4);  //exit when all light are on
  CurrentTime++; // update time
  lcd.setCursor(0,2);
  lcd.print(DayOfWeek[CurrentDay] + TimeFormat(CurrentTime));
}

void LightsEvening() {
  //This is the Evening Lighting Sequence
  //Street Lights On
  StreetLights(ON);
  //Building Lights On
  BuildingLights(ON);

  switch (CurrentDay) {
    case 3:  //Thursday
      Serial.println("Clubhouse: On");  // ClubHouseLights(ON);
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
    if (PCF_Controls.read(i)==0) {
      // found switch position
      switch (i){
        case 2: //Day - 10am
          Serial.print("Fast Fwd to 'Day'");
          SpeedUp(120);
          return;
        case 3: //Evening - 10pm
          Serial.print("Fast Fwd to 'Evening'");
          SpeedUp(264);
          return;
        case 4: // Midnight - 12am
          Serial.print("Fast Fwd to 'Midnight'");
          SpeedUp(288,0);
          return;
        case 5: // Real Time
         // SwitchToRealTime();
          //Serial.println("Real Time Mode");
          return;
        }
    }
    i++;
  } while (i<6);
  AutomaticTime();
  return;
}

void AutomaticTime() {
  Scale = ReturnScale;
  Pause=false;
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
  //StreetLights ON
  StreetLights(ON);
  PlatformLights(ON);
  BuildingLightsTwilight(OFF);  // un-comment when PCF8574 is added
                                //Building Lights Off
                                //
  Serial.println("Clubhouse: Off");  // ClubHouseLights(OFF);
}

void StreetLights(int Status) {
  //Turns street lights on or off
  for (int i = 0; i <= 7; i++) {
    StreetLight(i, Status);
    StreetLEDStatus[i]=Status;
  }
}

void BuildingLights(int Status) {
  //Turns building lights on or off
  for (int i = 0; i <= 7; i++) {
    BuildingLight(i, Status);
    BuildingLEDStatus[i]=Status;
  }
}
void PlatformLights(int Status) {
  //Turns building lights on or off
  for (int i = 0; i <= 3; i++) {
    PlatformLight(i, Status);
    PlatformLEDStatus[i]=Status;
  }
}

void ClubHouseLights(int Status) {
  PCF_PlatformLights.write(4, Status); // Pins 4 and 5 on the PCF8574 board for the platform lights should be connected to welder and clubhouse.
  PlatformLEDStatus[4]=Status;
  PCF_PlatformLights.write(5, Status);
  PlatformLEDStatus[5]=Status;
}

void StreetLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either LOW, HIGH, ON or OFF
  PCF_StreetLights.write(LED, Status);
  StreetLEDStatus[LED]=Status;
}

void BuildingLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either LOW, HIGH, ON or OFF
  PCF_BuildingLights.write(LED, Status);
  BuildingLEDStatus[LED]=Status;
}

void PlatformLight(int LED, int Status) {
  //set an LED acording to Status
  //LED = 0-7 this is a port on the PCF8574 board
  //Status os either LOW, HIGH, ON or OFF
  PCF_PlatformLights.write(LED, Status);
  PlatformLEDStatus[LED]=Status;
}

void AllLEDOn() {
  for (int i = 0; i <= 7; i++) {
    PCF_StreetLights.write(i, ON);
    StreetLEDStatus[i]=ON;
    PCF_BuildingLights.write(i, ON);
    BuildingLEDStatus[i]=ON;
    PCF_PlatformLights.write(i, ON);
    PlatformLEDStatus[i]=ON;
  }
}

void AllLEDOff() {
  for (int i = 0; i <= 7; i++) {
    PCF_StreetLights.write(i, OFF);
    StreetLEDStatus[i]=OFF;
    PCF_BuildingLights.write(i, OFF);
    BuildingLEDStatus[i]=OFF;
    PCF_PlatformLights.write(i, OFF);
    PlatformLEDStatus[i]=OFF;
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
    FormattedTime = "0" + String(Hour);
  } else {
    FormattedTime = String(Hour);
  }
  if (Minute < 5){
    FormattedTime=FormattedTime+":00"; 
  } else if (Minute < 10){
    FormattedTime=FormattedTime+":05"; 
  } else{
    FormattedTime=FormattedTime+":"+Minute;
  }
  return FormattedTime;
}

void lcdTest() {
  lcd.setCursor(5, 1);  // set the cursor to column 5, line 1
  lcd.print("W.D.M.E.S.");
  lcd.setCursor(2, 2);  // set the cursor to column 2, line 2
  lcd.print("OO Gauge Railway");
  delay(3000);
  lcd.clear();  // Clears the  display
  lcd.setCursor(5, 0);  // set the cursor to column 5, line 0
  lcd.print("W.D.M.E.S.");
}

//  void lcdPrint(String message, int line) {
//    lcd.setCursor(0, line+2);  // set the cursor to column 0, line 0
//    lcd.print(message);
//  }

void LCDPrintRealTime(){
  //load datetime with value from RTC
  DateTime now = rtc.now();
  lcd.setCursor(0,1);
  lcd.print("     RTC  ");
  lcd.setCursor(10, 1);  // set the cursor to column 5, line 1
  int digit = now.hour();
  if (digit <10) {
    lcd.print("0");
    lcd.print(digit);
  }
  else {
    lcd.print(digit);
  }
  digit = now.minute() ;
  lcd.print(":");
   if (digit <10) {
    lcd.print("0");
    lcd.print(digit);
  }
  else {
    lcd.print(digit);
  }
  }

void SwitchToRealTime() {
  Serial.println("REALTIME MODE");
  Pause=false;
  RealTime=true;
  
  DateTime now = rtc.now();

  int ScaledTime = 0;
  ScaledTime = ScaledTime + (now.hour() * 12);
  ScaledTime = ScaledTime + floor(now.minute()/5);

  double ScaledTime1;
  ScaledTime1 = ScaledTime1 + (now.hour() * 12);
  ScaledTime1 = ScaledTime1 + (now.minute()/5);
  ScaledTime1 = ScaledTime1 + (now.second()/300);

  double TimeDifference;
  TimeDifference = 300000 * ((ScaledTime + 1) - ScaledTime1);

  Serial.print("CurrentTime: ");
  Serial.println(ScaledTime);
  Serial.print("Exact CurrentTime: ");
  Serial.println(ScaledTime1);
  Serial.print("Wait until next 5min in ms: ");
  Serial.println(TimeDifference);

  TimeStatus = "Realtime";
  CurrentTime = ScaledTime;
  Scale = TimeDifference;

}
