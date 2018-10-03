// USAGE:
//---------------------------------------------------------------------------------------------
// - Set host serial terminal to 9600 baud
// - To open a serial channel (numbered 0 - 7), send the number of the channel
// - To issue a command, enter it directly to the console.
//---------------------------------------------------------------------------------------------
#include <SoftwareSerial.h>     //Include the software serial library  
SoftwareSerial sSerial(11, 10); // RX, TX  - Name the software serial library sSerial (this cannot be omitted)
                                // assigned to pins 10 and 11 for maximum compatibility

const int s0 = 7;               //Arduino pin 7 to control pin S0
const int s1 = 6;               //Arduino pin 6 to control pin S1
const int enable_1 = 5;         //Arduino pin 5 to control pin E on shield 1
const int enable_2 = 4;         //Arduino pin 4 to control pin E on shield 2

char sensordata[32];              //A 30 byte character array to hold incoming data from the sensors
byte computer_bytes_received = 0; //We need to know how many characters bytes have been received
byte sensor_bytes_received = 0;   //We need to know how many characters bytes have been received
int channel;                      //INT pointer for channel switching - 0-7 serial, 8-127 I2C addresses
char *cmd;                        //Char pointer used in string parsing
int retries;                      // com-check functions store number of retries here
boolean answerReceived;           // com-functions store here if a connection-attempt was successful
byte error;                       // error-byte to store result of Wire.transmissionEnd()

String stamp_type;                // hold the name / type of the stamp
char stamp_version[4];            // hold the version of the stamp

char computerdata[20];            //we make a 20 byte character array to hold incoming data from a pc/mac/other.
int computer_in_byte;
boolean computer_msg_complete = false;

const long validBaudrates[] = { 38400, 19200, 9600, 115200, 57600 };//list of baudrates to try when connecting to a stamp (they're ordered by probability to speed up things a bit)
long channelBaudrate[] = { 0, 0, 0, 0, 0, 0, 0, 0 };//store for the determined baudrates for every stamp

void serialPrintDivider() 
{
  Serial.println(  F("------------------"));
}

void intro() //print intro
{
    serialPrintDivider();
    Serial.println( F("Whitebox Labs -- Tentacle Shield - Stamp Setup"));
    Serial.println( F("For info type 'help'"));
    Serial.println( F("To read current config from attached stamps type 'scan'"));
    Serial.println( F(" (e.g. if you've changed baudrates)"));
    Serial.println( F("TYPE CHANNEL NUMBER (Serial: 0-7):"));
}

void help() //print help dialogue
{
    serialPrintDivider();
    Serial.println( F("To open a serial channel (numbered 0 - 7), send the number of the channel"));
    Serial.println( F("To issue a command, enter it directly to the console."));
    Serial.println( F("To update information about connected stamps, type 'scan'."));
    Serial.println( F(" -> This might take a while, it will scan all ports (serial)"));
    Serial.println( F(" -> and determine correct baudrates."));
    Serial.println( F("To set baudrate manually, type one of 38400,19200,9600,115200,57600"));
    Serial.println( F("=========="));
}

void scan(boolean scanserial) // Scan for all devices. Set scanserial to false to scan I2C only (much faster!) 
{
    if (scanserial) 
    {
        Serial.println(F("Starting scan... this might take up to a minute."));
    } 

    int stamp_amount = 0;

    if (scanserial) 
    {
        for (channel = 0; channel < 8; channel++) 
        {
            if (change_channel()) 
            {
                stamp_amount++;

                serialPrintDivider();
                Serial.print(    F("-- SERIAL CHANNEL "));
                Serial.println(  channel);
                Serial.println(  F("--"));
                Serial.print(    F("-- Type: "));
                Serial.println(  stamp_type);
                Serial.print(    F("-- Baudrate: "));
                Serial.println(  channelBaudrate[channel]);
            }
        }
    }
}

void setup() 
{
    pinMode(s1, OUTPUT);       //Set the digital pin as output.
    pinMode(s0, OUTPUT);       //Set the digital pin as output.
    pinMode(enable_1, OUTPUT); //Set the digital pin as output.
    pinMode(enable_2, OUTPUT); //Set the digital pin as output.

    Serial.begin(9600);     //Set the hardware serial port to 9600
    while (!Serial) ;       //Leonardo-type arduinos need this to be able to write to the serial port in setup()
    sSerial.begin(38400);   //Set the soft serial port to 38400
    stamp_type.reserve(16); //reserve string buffer to save some SRAM

    intro(); //display startup message
}

void loop() 
{
    while (Serial.available() > 0) // While there is serial data from the computer
    {
        computer_in_byte = Serial.read(); // read a byte

        if (computer_in_byte == '\n' || computer_in_byte == '\r') // if a newline character arrives, we assume a complete command has been received
        {
            computerdata[computer_bytes_received] = 0;
            computer_msg_complete = true;
            computer_bytes_received = 0;
        } 
        else //command not complete yet, so just ad the byte to data array
        {
            computerdata[computer_bytes_received] = computer_in_byte;
            computer_bytes_received++;
        }
    }

    if (computer_msg_complete) //if there is a complete command from the computer
    {
        cmd = computerdata;  //Set cmd with incoming serial data
        if (String(cmd) == F("help")) //if help entered...
        {
            help(); //call help dialogue
            computer_msg_complete = false;  //Reset the var computer_bytes_received to equal 0
            return;
        }
        else if (String(cmd) == F("scan")) //if scan requested
        {
            scan(true);
            computer_msg_complete = false;  //Reset the var computer_bytes_received to equal 0
            return;
        }
        else //it's not a known command, so probaby it's a channel number
        {
            //TODO: without loop?
            for (int x = 0; x <= 8; x++) //loop through input searching for a channel change request (integer between 1 and 127)
            {    
                if (String(cmd) == String(x)) 
                {
                    Serial.print(F("changing channel to "));
                    Serial.println( cmd);
                    channel = atoi(cmd); //set channel variable to number 0-127
                    if (change_channel()) //set MUX switches or I2C address
                    {
                        Serial.println(  F("-------------------------------------"));
                        Serial.print(    F("ACTIVE channel : "));
                        Serial.println(  channel );
                        Serial.print(    F("Type: "));
                        Serial.print(    stamp_type);
                        Serial.print(    F(", Version: "));
                        Serial.print(    stamp_version);
                        Serial.print(    F(", COM: "));

                        Serial.print(  F("UART ("));
                        Serial.print(  String(channelBaudrate[channel]));
                        Serial.println(F(" baud)"));
                    }
                    else 
                    {
                        Serial.println(F("CHANNEL NOT AVAILABLE! Empty slot? Different COM-mode?"));
                        Serial.println(F("Try 'scan' or set baudrate manually (see 'help')."));
                    }
                    computer_msg_complete = false; //Reset the var computer_bytes_received to equal 0
                    return;
                }
            }

            for ( int x = 0; x < 5; x++) // check if a baudrate was entered manually
            {
                if (String(cmd) == String(validBaudrates[x])) 
                {
                    Serial.print(F("Setting baudrate manually to "));
                    Serial.println(validBaudrates[x]);
                    channelBaudrate[channel] = validBaudrates[x];
                    String(channel).toCharArray(cmd, 4); //do a "fake" command for the next loop-run, as if the channel number was entered again (will refresh info with the manually set baudrate)
                    return; //do not set computer_bytes_received to 0, so the channel-command gets handled right away
                }
            }

            if (String(cmd).startsWith(F("serial,"))) 
            {
                Serial.println(F("! when switching from i2c to serial or vice-versa,"));
                Serial.println(F("! don't forget to switch the hardware jumpers accordingly."));
            }
        }

        Serial.print(F("> "));// echo to the serial console
        Serial.println(cmd);
        sSerial.print(cmd);   //Send the command from the computer to the Atlas Scientific device using the softserial port
        sSerial.print("\r");  //After we send the command we send a carriage return <CR>

        computer_msg_complete = false;          //Reset the var computer_bytes_received to equal 0
    }

    if (sSerial.available() > 0) //If data has been transmitted from an Atlas Scientific device
    {
        sensor_bytes_received = sSerial.readBytesUntil(13, sensordata, 30); //we read the data sent from the Atlas Scientific device until we see a <CR>. We also count how many character have been received
        sensordata[sensor_bytes_received] = 0; //we add a 0 to the spot in the array just after the last character we received. This will stop us from transmitting incorrect data that may have been left in the buffer
        Serial.print(F("< "));
        Serial.println(sensordata); //let’s transmit the data received from the Atlas Scientific device to the serial monitor
    }
}

boolean change_channel() //function controls which UART/I2C port is opened. returns true if channel could be changed.
{
  stamp_type = "";
  memset(stamp_version, 0, sizeof(stamp_version)); // clear stamp info
  change_serial_mux_channel();                     // configure serial muxer(s) (or disable if we're in I2C mode)

  if (channel < 8) //serial?
  {
    if (!check_serial_connection()) //determine and set the correct baudrate for this stamp
    {
      return false;
    }
  }
  return true;
}

void change_serial_mux_channel() //configures the serial muxers depending on channel.
{
    switch (channel) //Looking to see what channel to open
    {
        case 0:                                  //If channel==0 then we open channel 0
            digitalWrite(enable_1, LOW);           //Setting enable_1 to low activates primary channels: 0,1,2,3
            digitalWrite(enable_2, HIGH);          //Setting enable_2 to high deactivates secondary channels: 4,5,6,7
            digitalWrite(s0, LOW);                 //S0 and S1 control what channel opens
            digitalWrite(s1, LOW);                 //S0 and S1 control what channel opens
        break;                                 //Exit switch case

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
            digitalWrite(enable_1, HIGH);   //disable soft serial
            digitalWrite(enable_2, HIGH);   //disable soft serial
    }
}

boolean check_serial_connection() // check the selected serial port. find and set baudrate, request info from the stamp
{                                 // will return true if there is a stamp on this serial channel, false otherwise
    answerReceived = true; //will hold if we received any answer. also true, if no "correct" baudrate has been found, but still something answered.
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
    answerReceived = true; //will hold if we received any answer. also true, if no "correct" baudrate has been found, but still something answered.

    while (retries < 3 && answerReceived == true) //we don't seem to know the correct baudrate yet. try it 3 times (in case it doesn't work right away)
    {
        answerReceived = false; // we'll toggle this to know if we received an answer, even if no baudrate matched. probably a com-error, so we'll just retry.
        if (scan_baudrates()) 
        {
            return true;
        }
        retries++;
    }
    return false;  //no stamp was found at this channel
}

boolean scan_baudrates() //scans baudrates to auto-detect the right one for this uart channel. if one is found, it is saved globally in channelBaudrate[]
{
    for (int j = 0; j < 5; j++) // TODO: make this work for legacy stuff and EZO in uart / continuous mode
    {
        sSerial.begin(validBaudrates[j]); //open soft-serial port with a baudrate
        sSerial.print(F("\r"));
        sSerial.flush();                  //buffers are full of junk, clean up
        sSerial.print(F("c,0\r"));        //switch off continuous mode for new ezo-style stamps
        delay(150);
        //clearIncomingBuffer();          //buffers are full of junk, clean up
        sSerial.print(F("e\r"));   //switch off continous mode for legacy stamps

        delay(150);             //give the stamp some time to burp an answer
        clearIncomingBuffer();

        int r_retries = 0;
        answerReceived = true;
        while (r_retries < 3 && answerReceived == true) 
        {
            answerReceived = false;
            if (request_serial_info()) //check baudrate for correctness by parsing the answer to "i"-command
            {
                channelBaudrate[channel] = validBaudrates[j];  //we found the correct baudrate!
                return true;
            }
            r_retries++;
        }
    }
    return false; //we could not determine a correct baudrate
}

void clearIncomingBuffer() // "clears" the incoming soft-serial buffer
{          
    while (sSerial.available() ) 
    {
        //Serial.print((char)sSerial.read());
        sSerial.read();
    }
}

boolean request_serial_info() // helper to request info from a uart stamp and parse the answer into the global stamp_ variables
{
    clearIncomingBuffer();
    sSerial.write("i");   // send "i" which returns info on all versions of the stamps
    sSerial.write("\r");

    delay(150);           //give it some time to send an answer

    sensor_bytes_received = sSerial.readBytesUntil(13, sensordata, 9);  //we read the data sent from the Atlas Scientific device until we see a <CR>. We also count how many character have been received

    if (sensor_bytes_received > 0) //there's an answer
    {
        answerReceived = true;     //so we can globally know if there was an answer on this channel

        if ( parseInfo() ) // try to parse the answer string
        {
            delay(100);
            clearIncomingBuffer(); // some stamps burp more info (*OK or something). we're not interested yet.
            return true;
        }
    }
    return false; // it was not possible to get info from the stamp
}

boolean parseInfo() // parses the answer to a "i" command. returns true if answer was parseable, false if not.
{ 
    // example:
    // PH EZO  -> '?I,pH,1.1'
    // ORP EZO -> '?I,OR,1.0'   (-> wrong in documentation 'OR' instead of 'ORP')
    // DO EZO  -> '?I,D.O.,1.0' || '?I,DO,1.7' (-> exists in D.O. and DO form)
    // EC EZO  -> '?I,EC,1.0 '
    // TEMP EZO-> '?I,RTD,1.2'


    // Legazy PH  -> 'P,V5.0,5/13'
    // Legazy ORP -> 'O,V4.4,2/13'
    // Legazy DO  -> 'D,V5.0,1/13'
    // Legazy EC  -> 'E,V3.1,5/13'

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
    return false;        // can not parse this info-string
  }
}
