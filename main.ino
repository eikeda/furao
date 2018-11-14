#include <SoftwareSerial.h>
#include <Wire.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>

SoftwareSerial sSerial(11, 10);
Sd2Card card;
RTC_DS3231 rtc;
File file;

const int s0 = 7;
const int s1 = 6;
const int enable_1 = 5;
const int enable_2 = 4;

int chipSelect = 10;
bool cardIsUp = false;
bool realTimeClockIsUp = false;
String logFileName = "";
DateTime dateTime;

char sensordata[32];
byte computer_bytes_received = 0;
byte sensor_bytes_received = 0;
int channel;
char *cmd;
int retries;
boolean answerReceived;
byte error;

String stamp_type;
String pt_br_stamp_type;
char stamp_version[4];

char computerdata[20];
int computer_in_byte;
boolean computer_msg_complete = false;

const long validBaudrates[] = { 38400, 19200, 9600, 115200, 57600 };
long channelBaudrate[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void serialPrintDivider()
{ 
    Serial.println(  F("------------------")); 
}

void greetings()
{
    Serial.println( F("    ##################################################"));
    Serial.println( F("    # - Eric Ikeda - FuraoDataLogger - 2018/11/04  - #"));
    Serial.println( F("    # - Tentacle Shield Integration - Version 0.01 - #"));
    Serial.println( F("    ##################################################"));
    delay(5000);
}

void get_sensor_type_in_portuguese()
{
    if (stamp_type.equalsIgnoreCase("EZO pH"))
    {
      pt_br_stamp_type = "percentual hidrogenionico";
    }
    else if (stamp_type.equalsIgnoreCase("EZO ORP"))
    {
      pt_br_stamp_type = "potencial de oxirreducao";
    }
    else if (stamp_type.equalsIgnoreCase("EZO DO"))
    {
      pt_br_stamp_type = "oxigenio dissolvido";
    }
    else if (stamp_type.equalsIgnoreCase("EZO EC"))
    {
      pt_br_stamp_type = "condutividade eletrica";
    }
    else if (stamp_type.equalsIgnoreCase("EZO RTD"))
    {
      pt_br_stamp_type = "temperatura";
    }
    else
    {
      pt_br_stamp_type = "Sensor nao identificado";
    }
}

void addLineToLogFile(String stringLine)
{
  char buffFileName[19];
  logFileName.toCharArray(buffFileName, 19);
  
  dateTime = rtc.now();
  
  file = SD.open(buffFileName, FILE_WRITE); 
  if(file)
  {
    file.print(dateTime.year(), DEC);
    file.print('-');
    file.print(dateTime.month() < 10 ? "0" : "");
    file.print(dateTime.month(), DEC);
    file.print('-');
    file.print(dateTime.day() < 10 ? "0" : "");
    file.print(dateTime.day(), DEC);
    file.print(' ');
    file.print(dateTime.hour() < 10 ? "0" : "");
    file.print(dateTime.hour(), DEC);
    file.print(':');
    file.print(dateTime.minute() < 10 ? "0" : "");
    file.print(dateTime.minute(), DEC);
    file.print(':');
    file.print(dateTime.second() < 10 ? "0" : "");
    file.print(dateTime.second(), DEC);
    file.print('|');
    file.println(stringLine);

    file.close();
    Serial.println("Linha adicionada ao log");
  }
}

void scan(boolean scanserial)
{
    int stamp_amount = 0;

    if (scanserial) 
    {
        for (channel = 0; channel < 4; channel++) 
        {
            if (change_channel()) 
            {
                String readings = "";
                get_sensor_type_in_portuguese();
                stamp_amount++;
                serialPrintDivider();
                Serial.print(    F("-- SERIAL CHANNEL "));
                readings.concat("Serial Channel: ");
                Serial.println(  channel);
                readings.concat(String(channel));
                Serial.print(    F("-- Type: "));
                readings.concat(" - Type: ");
                Serial.print(  stamp_type);
                readings.concat(String(stamp_type));
                Serial.print(    F(", Version: "));
                readings.concat(" - Version: ");
                Serial.println(    stamp_version);
                readings.concat(String(stamp_version));
                Serial.print(    F("-- Obtendo: "));
                readings.concat(" - Reading: ");
                Serial.println(  pt_br_stamp_type);
                readings.concat(String(pt_br_stamp_type));
                Serial.print(F("> "));
                Serial.println("r");
                sSerial.print("r");
                sSerial.print("\r");
                delay(800);

                if (sSerial.available() > 0)
                {
                    sensor_bytes_received = sSerial.readBytesUntil(13, sensordata, 30);
                    sensordata[sensor_bytes_received] = 0;
                    Serial.print(F("< "));
                    Serial.println(sensordata);
                    readings.concat(" - Value: ");
                    readings.concat(String(sensordata));

                    addLineToLogFile(readings);
                    Serial.println(readings);
                }
            }
        }
    }
}

void configSerial()
{
    pinMode(s1, OUTPUT);
    pinMode(s0, OUTPUT);
    pinMode(enable_1, OUTPUT);
    pinMode(enable_2, OUTPUT);

    Serial.begin(9600);
    while (!Serial);
    sSerial.begin(38400);
    stamp_type.reserve(16);
}

bool isSecureDigitalCardUp()
{
  Serial.println("Iniciando Cartao SD...");
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);
  if (!SD.begin(chipSelect))
  {
    Serial.println("Ops...Falha na inicializacao do Cartao SD.");
    return false;
  }
  Serial.println("Cartao SD iniciado!");
  return true;
}

void settingRealTimeClockUp()
{
  if (rtc.lostPower()) 
  {
    //A linha abaixo ajusta o RTC em tempo de compilacao
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    //Ou pode-se utilizar linha abaixo para ajustar manualmente o RTC
    //rtc.adjust(DateTime(2018, 1, 21, 3, 0, 0));
  }
}

bool isRealTimeClockUp()
{
  if (!rtc.begin())
  {
    Serial.println("RTC nao inicializado...");
    return false;
  }
  
  Serial.println("RTC inicializado!");  
  return true;
}

void formatLogFileName()
{
  dateTime = rtc.now();

  logFileName.concat(String(dateTime.year()));
  logFileName.concat(String(dateTime.month()));
  logFileName.concat(String(dateTime.day()));
  logFileName.concat(".csv");
}

void setup() 
{
  configSerial();
  greetings();

  while(isSecureDigitalCardUp() == false)
  {
    cardIsUp = isSecureDigitalCardUp();
    delay(1000);
  }

  realTimeClockIsUp = isRealTimeClockUp();

  if(realTimeClockIsUp)
  {
    settingRealTimeClockUp();
  }
}

bool isLogFileCreated()
{
  char buffFileName[19];
  logFileName.toCharArray(buffFileName, 19);

  if (!SD.exists(buffFileName)) 
  {
    //only open a new file if it doesn't exist
    Serial.print("===>>>Arquivo de log aberto: ");
    Serial.println(logFileName);
    file = SD.open(buffFileName, FILE_WRITE); 
    if(file)
    {
      file.close();
      Serial.println("Arquivo encontrado ou criado.");
    }
  }

  if (!SD.exists(buffFileName)) 
  {
    Serial.println("Nao foi possivel criar o arquivo...");
    return false;
  }
  
  Serial.print("Nome do arquivo de log: ");
  Serial.println(logFileName);
  return true;
}

void loop() 
{
  Serial.println("Looping Principal");
  logFileName = "";
  formatLogFileName();
  isLogFileCreated();

  scan(true);

  delay(60000);
}

boolean change_channel()
{
  stamp_type = "";
  memset(stamp_version, 0, sizeof(stamp_version));
  change_serial_mux_channel();

  if (channel < 8)
  {
    if (!check_serial_connection())
    {
      return false;
    }
  }
  return true;
}

void change_serial_mux_channel()
{
    switch (channel)
    {
        case 0:
            digitalWrite(enable_1, LOW);
            digitalWrite(enable_2, HIGH);
            digitalWrite(s0, LOW);
            digitalWrite(s1, LOW);
        break;

        case 1:
            digitalWrite(enable_1, LOW);
            digitalWrite(enable_2, HIGH);
            digitalWrite(s0, HIGH);
            digitalWrite(s1, LOW);
        break;

        case 2:
            digitalWrite(enable_1, LOW);
            digitalWrite(enable_2, HIGH);
            digitalWrite(s0, LOW);
            digitalWrite(s1, HIGH);
        break;

        case 3:
            digitalWrite(enable_1, LOW);
            digitalWrite(enable_2, HIGH);
            digitalWrite(s0, HIGH);
            digitalWrite(s1, HIGH);
        break;

        case 4:
            digitalWrite(enable_1, HIGH);
            digitalWrite(enable_2, LOW);
            digitalWrite(s0, LOW);
            digitalWrite(s1, LOW);
        break;

        case 5:
            digitalWrite(enable_1, HIGH);
            digitalWrite(enable_2, LOW);
            digitalWrite(s0, HIGH);
            digitalWrite(s1, LOW);
        break;

        case 6:
            digitalWrite(enable_1, HIGH);
            digitalWrite(enable_2, LOW);
            digitalWrite(s0, LOW);
            digitalWrite(s1, HIGH);
        break;

        case 7:
            digitalWrite(enable_1, HIGH);
            digitalWrite(enable_2, LOW);
            digitalWrite(s0, HIGH);
            digitalWrite(s1, HIGH);
        break;

        default:
            digitalWrite(enable_1, HIGH);
            digitalWrite(enable_2, HIGH);
    }
}

boolean check_serial_connection()
{
    answerReceived = true;
    retries = 0;

    if (channelBaudrate[channel] > 0) 
    {
        sSerial.begin(channelBaudrate[channel]);
        while (retries < 3 && answerReceived == true) 
        {
            answerReceived = false;
            if (request_serial_info()) 
            {
                return true;
            }
        }
    }
    answerReceived = true;

    while (retries < 3 && answerReceived == true)
    {
        answerReceived = false;
        if (scan_baudrates()) 
        {
            return true;
        }
        retries++;
    }
    return false;
}

boolean scan_baudrates()
{
    for (int j = 0; j < 5; j++)
    {
        sSerial.begin(validBaudrates[j]);
        sSerial.print(F("\r"));
        sSerial.flush();
        sSerial.print(F("c,0\r"));
        delay(150);
        sSerial.print(F("e\r"));

        delay(150);
        clearIncomingBuffer();

        int r_retries = 0;
        answerReceived = true;
        while (r_retries < 3 && answerReceived == true) 
        {
            answerReceived = false;
            if (request_serial_info())
            {
                channelBaudrate[channel] = validBaudrates[j];
                return true;
            }
            r_retries++;
        }
    }
    return false;
}

void clearIncomingBuffer()
{          
    while (sSerial.available() ) 
    {
        sSerial.read();
    }
}

boolean request_serial_info()
{
    clearIncomingBuffer();
    sSerial.write("i");
    sSerial.write("\r");

    delay(150);

    sensor_bytes_received = sSerial.readBytesUntil(13, sensordata, 9);

    if (sensor_bytes_received > 0)
    {
        answerReceived = true;

        if (parseInfo())
        {
            delay(100);
            clearIncomingBuffer();
            return true;
        }
    }
    return false;
}

boolean parseInfo()
{ 
    if (sensordata[0] == '?' && sensordata[1] == 'I')// seems to be an EZO stamp 
    {
        // PH EZO
        if (sensordata[3] == 'p' && sensordata[4] == 'H') 
        {
            stamp_type = F("EZO pH");
            stamp_version[0] = sensordata[6];
            stamp_version[1] = sensordata[7];
            stamp_version[2] = sensordata[8];

            return true;
            //ORP EZO
        }
        else if (sensordata[3] == 'O' && sensordata[4] == 'R') 
        {
            stamp_type = F("EZO ORP");
            stamp_version[0] = sensordata[6];
            stamp_version[1] = sensordata[7];
            stamp_version[2] = sensordata[8];

            return true;
            //DO EZO
        }
        else if (sensordata[3] == 'D' && sensordata[4] == 'O') 
        {
            stamp_type = F("EZO DO");
            stamp_version[0] = sensordata[6];
            stamp_version[1] = sensordata[7];
            stamp_version[2] = sensordata[8];

            return true;
            //D.O. EZO
        }
        else if (sensordata[3] == 'D' && sensordata[4] == '.' && sensordata[5] == 'O' && sensordata[6] == '.') 
        {
            stamp_type = F("EZO DO");
            stamp_version[0] = sensordata[8];
            stamp_version[1] = sensordata[9];
            stamp_version[2] = sensordata[10];

            return true;
            //EC EZO
        }
        else if (sensordata[3] == 'E' && sensordata[4] == 'C') 
        {
            stamp_type = F("EZO EC");
            stamp_version[0] = sensordata[6];
            stamp_version[1] = sensordata[7];
            stamp_version[2] = sensordata[8];

            return true;
            //RTD EZO
        }
        else if (sensordata[3] == 'R' && sensordata[4] == 'T' && sensordata[5] == 'D') 
        {
            stamp_type = F("EZO RTD");
            stamp_version[0] = sensordata[7];
            stamp_version[1] = sensordata[8];
            stamp_version[2] = sensordata[9];

            return true;
            //unknown EZO stamp
        }
        else 
        {
            stamp_type = F("unknown EZO stamp");
            return true;
        }
    }
  else //it's a legacy stamp (non-EZO)
  {
    // Legacy pH
    if ( sensordata[0] == 'P') 
    {
        stamp_type = F("pH (legacy)");
        stamp_version[0] = sensordata[3];
        stamp_version[1] = sensordata[4];
        stamp_version[2] = sensordata[5];
        stamp_version[3] = 0;
        return true;
        //legacy ORP
    }
    else if ( sensordata[0] == 'O') 
    {
        stamp_type = F("ORP (legacy)");
        stamp_version[0] = sensordata[3];
        stamp_version[1] = sensordata[4];
        stamp_version[2] = sensordata[5];
        stamp_version[3] = 0;
        return true;
        //Legacy D.O.
    }
    else if ( sensordata[0] == 'D') 
    {
        stamp_type = F("D.O. (legacy)");
        stamp_version[0] = sensordata[3];
        stamp_version[1] = sensordata[4];
        stamp_version[2] = sensordata[5];
        stamp_version[3] = 0;
        return true;
        //Lecagy EC
    }
    else if ( sensordata[0] == 'E') 
    {
        stamp_type = F("EC (legacy)");
        stamp_version[0] = sensordata[3];
        stamp_version[1] = sensordata[4];
        stamp_version[2] = sensordata[5];
        stamp_version[3] = 0;
        return true;
    }
    return false;
  }
}