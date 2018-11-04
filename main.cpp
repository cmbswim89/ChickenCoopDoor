/**
 * @file       main.cpp
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Mar 2015
 * @brief
 */
void StopExtend();
void StartExtend();
void StopRetract();
void StartRetract();
void PrintToLcdScreen();
void SaveOpenCloseTime();
void ReadOpenCloseTime();

//#define BLYNK_DEBUG
#define BLYNK_PRINT stdout
#ifdef RASPBERRY
  #include <BlynkApiWiringPi.h>
#else
  #include <BlynkApiLinux.h>
#endif
#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>
#include <time.h>
#include <string>
#include <iostream>
#include <fstream>

#define relay1Pin 2
#define relay2Pin 3
#define sensor1 21
#define sensor2 18
#define LcdScreenPin 5

bool MorningOpenFlag = false;
bool NightCloseFlag = false;
long OpenTime;
long CloseTime;

static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);

bool IsMorningOpenTime(time_t localTime);
bool IsNightCloseTime(time_t localTime);

static const char *auth, *serv;
static uint16_t port;

#include <BlynkWidgets.h>

WidgetLCD lcd(V5);
WidgetLED led1(V3);
WidgetLED led2(V4);

time_t stopwatchRetract;
time_t stopwatchExtend;

typedef struct Time
{
    int Hour;
    int Minute;
}Time_T;

Time_T openTime_Time;
Time_T closeTime_Time;

void setup()
{
	pinMode(relay1Pin,OUTPUT);
	pinMode(relay2Pin,OUTPUT);
	pinMode(sensor1,INPUT);
	pinMode(sensor2,INPUT);
	ReadOpenCloseTime();
    Blynk.begin(auth, serv, port);
}

void loop()
{
	time_t theTime = time(NULL);	
	
	if(IsMorningOpenTime(theTime) && !MorningOpenFlag){
		StartRetract();
		stopwatchRetract = time(NULL);
		MorningOpenFlag = true;
		NightCloseFlag = false;
	}
	
	if(IsNightCloseTime(theTime) && !NightCloseFlag)
	{
		stopwatchExtend = time(NULL);
		StartExtend();
		NightCloseFlag = true;
		MorningOpenFlag = false;
	}

	int sensor1Value = digitalRead(sensor1);
	int sensor2Value = digitalRead(sensor2);
	
	if(sensor1Value== HIGH && digitalRead(relay1Pin) == HIGH){
		StopRetract();
	}
	if(sensor2Value == HIGH && digitalRead(relay2Pin) == HIGH){
		StopExtend();
	}
	
	if(digitalRead(relay1Pin) == HIGH)
	{
		clock_t end = time(NULL);
		int elapsed = int(end - stopwatchRetract);
		printf("Retract Elapsed: %d\n",elapsed);
		if(int(elapsed) > 45){
			StopExtend();
			StopRetract();
		}
	}
	
	if(digitalRead(relay2Pin) == HIGH)
	{
		clock_t end = time(NULL);
		int elapsed = int(end - stopwatchExtend);
		printf("Extend Elapsed: %d\n",elapsed);
		if(int(elapsed) > 45){
			StopRetract();
			StopExtend();
		}
	}
	
	if(millis() % 1000 == 0)
		PrintToLcdScreen();
		
	int CloseHour = CloseTime / 3600;
	int CloseMinute = (CloseTime % 3600) / 60;
	
	int OpenHour = OpenTime / 3600;
	int OpenMinute = (OpenTime % 3600) / 60;

	printf("%i:%i - %i:%i\n",openTime_Time.Hour,openTime_Time.Minute,closeTime_Time.Hour,closeTime_Time.Minute);
	
    Blynk.run();		
}


int main(int argc, char* argv[])
{
    parse_options(argc, argv, auth, serv, port);

    setup();
    while(true) {
        loop();
    }

    return 0;
}

BLYNK_WRITE(V1)
{
	//RETRACT
    if(param.asInt())
		StartRetract();
	stopwatchRetract = time(NULL);
	lcd.clear();
	lcd.print(2,0,"Manual OPEN");
}

BLYNK_WRITE(V0)
{
	//EXTEND
    if(param.asInt())
		StartExtend();
	stopwatchExtend = time(NULL);
	lcd.clear();
	lcd.print(2,0,"Manual Close");
}

BLYNK_WRITE(V2) //Reset Button
{    
	StopRetract();
	StopExtend();
}

BLYNK_WRITE(V6) //Open Time
{
	OpenTime = param[0].asLong();
	openTime_Time.Hour = OpenTime / 3600;
	openTime_Time.Minute = (OpenTime % 3600) / 60;
	SaveOpenCloseTime();
	ReadOpenCloseTime();
}

BLYNK_WRITE(V7) //Close Time
{    
	CloseTime = param[0].asLong();
	closeTime_Time.Hour = CloseTime / 3600;
	closeTime_Time.Minute = (CloseTime % 3600) / 60;
	SaveOpenCloseTime();
	ReadOpenCloseTime();
}

void StartRetract()
{
	led2.on();
	digitalWrite(relay1Pin,HIGH);
}

void StopRetract()
{
	led2.off();
	digitalWrite(relay1Pin,LOW);
	Blynk.notify("Chicken coop Closed!");
}


void StartExtend()
{
	led1.on();
	digitalWrite(relay2Pin,HIGH);
}

void StopExtend()
{
	led1.off();
	digitalWrite(relay2Pin,LOW);
	Blynk.notify("Chicken coop Opened!");
}

bool IsMorningOpenTime(time_t localTime)
{
	struct tm *aTime = localtime(&localTime);
	int hour=aTime->tm_hour;
	int min=aTime->tm_min;

	int OpenHour = OpenTime / 3600;
	int OpenMinute = (OpenTime % 3600) / 60;
	
	if(hour == openTime_Time.Hour && min == openTime_Time.Minute)
		return true;
	else
		return false;
}

bool IsNightCloseTime(time_t localTime)
{
	struct tm *aTime = localtime(&localTime);
	int hour=aTime->tm_hour;
	int min=aTime->tm_min;

	if(hour == closeTime_Time.Hour && min == closeTime_Time.Minute)
		return true;
	else
		return false;
}


void PrintToLcdScreen()
{
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	int hour=aTime->tm_hour;
	
	if(MorningOpenFlag){
		lcd.clear();
		lcd.print(6,0,"OPEN");
	}
	else if(NightCloseFlag)
	{
		lcd.clear();
		lcd.print(5,0,"CLOSED");
	}
	lcd.clear();
	printf("%i:%i - %i:%i",openTime_Time.Hour,openTime_Time.Minute,closeTime_Time.Hour,closeTime_Time.Minute);
	string str = to_string(openTime_Time.Hour) + ":" + to_string(openTime_Time.Minute) + " - " + to_string(closeTime_Time.Hour) + ":" + to_string(closeTime_Time.Minute);
	char t[1024];
	strcpy(t,str.c_str());
	lcd.print(3,1,t);
		
}

void SaveOpenCloseTime()
{
	ofstream output_file("OpenTime.data", ios::binary);
    output_file.write((char*)&openTime_Time, sizeof(openTime_Time));
    ofstream output_file2("CloseTime.data", ios::binary);
    output_file2.write((char*)&closeTime_Time, sizeof(closeTime_Time));
    output_file.close();
}
void ReadOpenCloseTime()
{
	ifstream input_file("OpenTime.data", ios::binary);
    Time_T masterOpenTime[3];
    input_file.read((char*)&openTime_Time, sizeof(openTime_Time));
    
    ifstream input_file2("CloseTime.data", ios::binary);
    Time_T masterCloseTime[3];
    input_file2.read((char*)&closeTime_Time, sizeof(closeTime_Time));
    
}
