#define def_SERVER
#define def_SERVO
#define def_SD
#define def_RTC
#define DEBUG_LINE Serial.print("Line : ");Serial.println(__LINE__);
#ifdef def_SERVER
#include <Ethernet.h>
#include <SPI.h>
#endif
#ifdef def_SERVO
#include<Servo.h>
#endif
#ifdef def_SD
#include <SD.h>
#endif
#ifdef def_RTC
#include <Wire.h>  //For RTC
#include "RTClib.h" //For RTC
#endif

//사용할 I/O 설정
const int SERVO    = 9;
const int CS_PIN      = 4;
const int RTC_GND_PIN = A2;
const int RTC_POW_PIN = A3;
const int RTC_SDA = A4;
const int RTC_SCL = A5;
//SERVO 객체
#ifdef def_SERVO
Servo myServo;
#endif
//RTC 객체
#ifdef def_RTC
RTC_DS3231 RTC;
#endif
//서버 객체
#ifdef def_SERVER
EthernetServer server = EthernetServer(80); //port 80
#endif

boolean receiving = false;
boolean InitSD = false;
boolean bFeeding = false;
boolean InitRTC = false;
//자동 급식 시간 저장을 위한 선언
#ifdef def_RTC
const int MaxFeedingCount = 5;
DateTime AutoFeed[MaxFeedingCount];
#endif

void setup()
{
  //Serial
  Serial.begin(9600);
  Serial.println(F("Serial Init"));
#ifdef def_RTC
  RTC_INIT(); //RTC INIT
#endif
#ifdef def_SERVO
  SERVO_INIT();
#endif
#ifdef def_SERVER
  SERVER_INIT();
#endif

#ifdef def_SD
  SD_INIT(); // SD INIT
#endif
  LoadAutoFeedTime();
  WriteAutoFeedTime();
}
//서버 초기화
#ifdef def_SERVER
void SERVER_INIT()
{
  //Ethernet
  Serial.println(F("SERVER Init"));
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x4A, 0xE1 };
  IPAddress ip(192, 168, 1, 100);
  //Connect with DHCP
  Ethernet.begin(mac, ip);
  Serial.println(F("Ethernet begin"));

  //Start the server
  server.begin();
  Serial.println(F("Server begin"));
  Serial.print(F("Server Started.\nLocal IP: "));
  Serial.println(Ethernet.localIP());
}
#endif
//서보 초기화 및 기준 위치로 이동
#ifdef def_SERVO
void SERVO_INIT()
{
  //Servo
  Serial.println(F("Servo Init"));
  myServo.attach(SERVO);
  myServo.write(0);
  Serial.println(F("Servo Begin"));
}
#endif
//RTC 초기화
#ifdef def_RTC
void RTC_INIT()
{
  //RTC
  Serial.println(F("RTC INIT"));
  pinMode(RTC_POW_PIN, OUTPUT);
  pinMode(RTC_GND_PIN, OUTPUT);
  digitalWrite(RTC_POW_PIN, HIGH);
  digitalWrite(RTC_GND_PIN, LOW);
  if (RTC.begin())
  {
    Serial.println(F("RTC Begin"));
  }
  else
  {
    Serial.println(F("CAN NOT FIND RTC !!!"));
  }
  RTC.adjust(DateTime(__DATE__, __TIME__));
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println(F("RTC Adjust"));
  InitRTC = true;
  Serial.print(F("RTC Ready :"));
  Serial.println((NowDateTime()));
}
#endif
//SD 카드 초기화
#ifdef def_SD
void SD_INIT()
{
  //SD
  Serial.println(F("SD INIT"));
  pinMode(CS_PIN,   OUTPUT);
  if (!SD.begin(CS_PIN))
  {
    Serial.println(F("Card Failure"));
    return;
  }
  InitSD = true;
  Serial.println(F("Card Ready"));
}
#endif

void loop()
{
  CheckAutoFeed(); //자동급식시간이 되었는지 확인
  Feeding(); // 실제 동작 함수
  //웹서버를 위한 소스
#ifdef def_SERVER
  EthernetClient client = server.available();

  if (client)
  {
    //An HTTP request ends with a blank line
    boolean currentLineIsBlank = true;
    boolean sentHeader = false;

    while (client.connected())
    {


      if (client.available())
      {
        char c = client.read(); //Read from the incoming buffer

        if (receiving && c == ' ') receiving = false; //done receiving
        if (c == '?') receiving = true; //found arguments

        //This looks at the GET requests
        if (receiving)
        {

          //An LED command is specified with an F
          if (c == 'F')
          {

            Serial.println(F("Receive FEEDING FROM WEB"));
            bFeeding = true;
            break;
          }
          //Add similarly formatted else if statements here
          //TO CONTROL ADDITIONAL THINGS
        }

        //Print out the response header and the HTML page
        if (!sentHeader)
        {
          //Send a standard HTTP response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html\n"));

          //Feed toggle button
          client.println(F("<form action='' method='get'>"));
          client.println(F("<input type='hidden' name='F' value='7' />"));
          client.println(F("<input type='submit' value='FEED' />"));
          client.println(F("</form>"));

          //Add additional forms forms for controlling more things here.

          sentHeader = true;
        }

        if (c == '\n' && currentLineIsBlank) break;

        if (c == '\n')
        {
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          currentLineIsBlank = false;
        }
      }
    }
    delay(5); //Give the web browser time to receive the data

    client.stop(); //Close the connection:
  }

#endif
}
//실제 급식시 서보 이동 관련 소스
void Feeding()
{
#ifdef def_SERVO
  if (bFeeding)
  {
    Serial.println(F("Feeding"));
    bFeeding = false;
    int pos;
    for (pos = 0; pos <= 180; pos++)
    {
      myServo.write(pos);
      delay(15);
    }
    for (pos = 180; pos >= 0; pos--)
    {
      myServo.write(pos);
      delay(15);
    }
//delay(500);
//  myServo.write(180);
//  delay(500);
//  myServo.write(0);
  }
#endif
}
// #ifdef def_RTC
// void SetTime(int iIndex, DateTime _time)
// {
  // if (iIndex > MaxFeedingCount && iIndex < 1)
  // {
    // return;
  // }
  // AutoFeed[iIndex - 1] =  _time;
// }
// #endif
//자동 급식 시간 체크 함수
void CheckAutoFeed()
{
#ifdef def_RTC
  int i = 0;
  int CheckHour, CheckMin;
  int NowHour, NowMin, NowSec;
  DateTime _now = RTC.now();
  NowHour = _now.hour();
  NowMin = _now.minute();
  NowSec = _now.second();
//    Serial.print(NowHour);
//    Serial.print(F(":"));
//    Serial.print(NowMin);
//    Serial.print(F(":"));
//    Serial.println(NowSec);
  for (i = 0; i < MaxFeedingCount; i++)
  {
    CheckHour = AutoFeed[i].hour();
    CheckMin = AutoFeed[i].minute();
    if (NowHour == CheckHour && NowMin == CheckMin && NowSec == 0 )
    {
      bFeeding = true;
      Serial.println(F("Now FEEDING"));
    }
  }
#endif
}
//SD 카드에서 자동 급식 시간 읽어오는 함수
void LoadAutoFeedTime()
{
#ifdef def_SD
  if (InitSD)
    return;

  //Read the configuration information (Time.txt)
  File commandFile = SD.open("Time.txt");
  if (commandFile)
  {
    Serial.println(F("Reading Command File"));
    int i = 0;
	// 실제 데이터는 4:10 -> 4.10  으로 저장
    while (commandFile.available())
    {
      if (i >= MaxFeedingCount)  break;
      float value = commandFile.parseFloat();
      int _hour = value;
      int _minute = (value - _hour) * 100;
      AutoFeed[i] = DateTime(0, 0, 0, _hour, _minute);
      i++;
    }

    commandFile.close();
  }
  else
  {
    Serial.println(F("Could not read command file."));
    return;
  }
//#else
//  DateTime _now = RTC.now();
//  AutoFeed[0] = _now + TimeSpan(0, 0, 1, 0);
//  AutoFeed[1] = _now + TimeSpan(0, 0, 2, 0);
//  AutoFeed[2] = _now + TimeSpan(0, 0, 3, 0);
//  AutoFeed[3] = _now + TimeSpan(0, 0, 4, 0);
//  AutoFeed[4] = _now + TimeSpan(0, 0, 0, 0);
#endif
}
//SD 카드에 자동급식 시간 저장하는 함수
void WriteAutoFeedTime()
{
#ifdef def_SD
  if (InitSD)
    return;

  //write the configuration information (Time.txt)
  if (SD.exists("Time.txt")) {
    SD.remove("Time.txt");  //File Overwrite
  }
  File commandFile = SD.open("Time.txt", FILE_WRITE);
  // 실제 데이터는 4:10 -> 4.10  으로 저장
  if (commandFile)
  {
    Serial.println(F("Write Command File"));
    int i = 0;
    for (i = 0; i < MaxFeedingCount; i++)
    {
      float value = AutoFeed[i].hour() + (AutoFeed[i].minute() / 100.0);
      commandFile.println(value);
    }
    commandFile.close();
  }
  else
  {
    Serial.println(F("Could not read command file."));
    return;
  }

#endif
}
//현재 시간을 문자열로 출력하는 함수
String NowDateTime()
{

  String year, month, day, hour, minute, second, time, date;
#ifdef def_RTC
  DateTime datetime = RTC.now();
  year  = String(datetime.year(),  DEC);
  month = String(datetime.month(), DEC);
  day  = String(datetime.day(),  DEC);
  hour  = String(datetime.hour(),  DEC);
  minute = String(datetime.minute(), DEC);
  second = String(datetime.second(), DEC);
#endif
  //Concatenate the strings into date and time
  date = year + "/" + month + "/" + day + " " + hour + ":" + minute + ":" + second;
  return date;
}

