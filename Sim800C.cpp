/*
 *	SOFTWARE SERIAL NOTES:
 *
 *		PINOUT
 *		The library has the following known limitations:
 *		1. If using multiple software serial ports, only one can receive data at a time.
 *		2. Not all pins on the Mega and Mega 2560 support change interrupts, so only the following can be used for RX: 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69).
 *		3. Not all pins on the Leonardo and Micro support change interrupts, so only the following can be used for RX: 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).
 *		4. On Arduino or Genuino 101 the current maximum RX speed is 57600bps
 *		5. On Arduino or Genuino 101 RX doesn't work on Pin 13
 *
 *		BAUD RATE
 *		Supported baud rates are 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 31250, 38400, 57600, and 115200.
*/

#include "Arduino.h"
#include "Sim800C.h"
#include <SoftwareSerial.h>

#ifdef SwSerial
  SoftwareSerial HwSwSerial(DEFAULT_RX_PIN, DEFAULT_TX_PIN); // RX, TX  
#else
  #define HwSwSerial  Serial   
#endif  

Sim800C::Sim800C(void)
{
}

void Sim800C::begin()
{
    pinMode(DEFAULT_POWER_PIN, OUTPUT);

    _baud = DEFAULT_BAUD_RATE;			// Default baud rate 9600
    HwSwSerial.begin(_baud);

    _sleepMode = 0;
    _functionalityMode = 1;

    _buffer.reserve(BUFFER_RESERVE_MEMORY); // Reserve memory to prevent intern fragmention
    Setup();
}

void Sim800C::begin(uint32_t baud)
{

    pinMode(DEFAULT_POWER_PIN, OUTPUT);

    _baud = baud;
    HwSwSerial.begin(_baud);

    _sleepMode = 0;
    _functionalityMode = 1;

    _buffer.reserve(BUFFER_RESERVE_MEMORY); // Reserve memory to prevent intern fragmention
    Setup();
}

uint8_t Sim800C::Setup(void)
{
    uint8_t respons = 0;
    int tryCount=0;
    PowerOn();
    delay(10000);
    send_cmd_wait_reply("AT+IPR="+String(_baud)+"\r\n",RESPON_OK,10000);
    send_cmd_wait_reply(F("ATE0\r\n"),RESPON_OK,10000);
    while ((respons = send_cmd_wait_reply(F("AT\r\n"),RESPON_OK, 10000))!=1)
    {
        delay(1500);
        tryCount++;
        if(tryCount>10) return ERROR;
    }
    send_cmd_wait_reply("AT+IPR="+String(_baud)+"\r\n",RESPON_OK,10000);
    //no cmd echo
    send_cmd_wait_reply(F("ATE0\r\n"), RESPON_OK, 500000);
    //FOR ENABLE TO DISPLAY WHEN RING HOST PHONE => SIM SEND "MO RING" AND WHEN CONNECT SIM SEND "MO CONNECTED"
    send_cmd_wait_reply(F("AT+MORING=1\r\n"),RESPON_OK, 500000);
    //ENABLE CALL
    send_cmd_wait_reply(F("AT+CLIR=0\r\n"),RESPON_OK, 500000);
    //text mode
    send_cmd_wait_reply(F("AT+CMGF=1\r\n"),RESPON_OK, 500000);
    //storage all to Sim card
    send_cmd_wait_reply(F("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n"),RESPON_OK, 500000);
    //clip=1 //for display income call
    send_cmd_wait_reply(F("AT+CLIP=1\r\n"),RESPON_OK, 500000);
    // AT+CNMI=2,1, return SMS as: +CMTI: "SM",i        i=INDEX
    send_cmd_wait_reply(F("AT+CNMI=2,1,0,0,0\r\n"), RESPON_OK, 500000);
    is_network_registered();
}

uint8_t Sim800C::is_network_registered()
{
    unsigned char connCode;
	HwSwSerial.print(F("AT+CREG?\r\n")); //+CREG: 0,1
    _buffer=_readSerial(25000); 
    if ( ((_buffer.indexOf(network_registered1)) != -1) || ((_buffer.indexOf(network_registered2)) != -1))
    {
        //setStatus(READY);
		delay(1000);
		return REG_REGISTERED;
    }	
	return REG_NOT_REGISTERED;
}


/*
 * AT+CSCLK=0	Disable slow clock, module will not enter sleep mode.
 * AT+CSCLK=1	Enable slow clock, it is controlled by DTR. When DTR is high, module can enter sleep mode. When DTR changes to low level, module can quit sleep mode
 */
bool Sim800C::setSleepMode(bool state)
{

    _sleepMode = state;

    if (_sleepMode) HwSwSerial.print(F("AT+CSCLK=1\r\n "));
    else 			HwSwSerial.print(F("AT+CSCLK=0\r\n "));

    if ( (_readSerial().indexOf("ER")) == -1)
    {
        return false;
    }
    else return true;
    // Error found, return 1
    // Error NOT found, return 0
}

bool Sim800C::getSleepMode()
{
    return _sleepMode;
}

/*
 * AT+CFUN=0	Minimum functionality
 * AT+CFUN=1	Full functionality (defualt)
 * AT+CFUN=4	Flight mode (disable RF function)
*/
bool Sim800C::setFunctionalityMode(uint8_t fun)
{

    if (fun==0 || fun==1 || fun==4)
    {

        _functionalityMode = fun;

        switch(_functionalityMode)
        {
        case 0:
            HwSwSerial.print(F("AT+CFUN=0\r\n "));
            break;
        case 1:
            HwSwSerial.print(F("AT+CFUN=1\r\n "));
            break;
        case 4:
            HwSwSerial.print(F("AT+CFUN=4\r\n "));
            break;
        }

        if ( (_readSerial().indexOf("ER")) == -1)
        {
            return false;
        }
        else return true;
        // Error found, return 1
        // Error NOT found, return 0
    }
    return false;
}

uint8_t Sim800C::getFunctionalityMode()
{
    return _functionalityMode;
}


bool Sim800C::setPIN(String pin)
{
    String command;
    command  = "AT+CPIN=";
    command += pin;
    command += "\r";

    // Can take up to 5 seconds

    HwSwSerial.print(command);

    if ( (_readSerial(5000).indexOf("ER")) == -1)
    {
        return false;
    }
    else return true;
    // Error found, return 1
    // Error NOT found, return 0
}


String Sim800C::getProductInfo()
{
    HwSwSerial.print("ATI\r");
    return (_readSerial());
}


String Sim800C::getOperatorsList()
{

    // Can take up to 45 seconds

    HwSwSerial.print("AT+COPS=?\r");

    return _readSerial(45000);

}

String Sim800C::getOperator()
{

    HwSwSerial.print("AT+COPS ?\r");

    return _readSerial();

}

void Sim800C::PowerOn()
{
	digitalWrite(DEFAULT_POWER_PIN,LOW);
	delay(1000);
	digitalWrite(DEFAULT_POWER_PIN,HIGH);
	delay(2200);
}

void Sim800C::PowerOff()
{
	digitalWrite(DEFAULT_POWER_PIN,LOW);
	delay(1000);
	digitalWrite(DEFAULT_POWER_PIN,HIGH);
	delay(1700);
	//Or
	//HwSwSerial.print(F("AT+CPOWD=1",1);
}

void Sim800C::reset()
{
    PowerOff();
	delay(500);
	PowerOn();
    // wait for the module response

    HwSwSerial.print(F("AT\r\n"));
    while (_readSerial().indexOf("OK")==-1 )
    {
        HwSwSerial.print(F("AT\r\n"));
    }

    //wait for sms ready
    while (_readSerial().indexOf("SMS")==-1 );
}

void Sim800C::setPhoneFunctionality()
{
    /*AT+CFUN=<fun>[,<rst>]
    Parameters
    <fun> 0 Minimum functionality
    1 Full functionality (Default)
    4 Disable phone both transmit and receive RF circuits.
    <rst> 1 Reset the MT before setting it to <fun> power level.
    */
    HwSwSerial.print (F("AT+CFUN=1\r\n"));
}


String Sim800C::signalQuality()
{
    /*Response
    +CSQ: <rssi>,<ber>Parameters
    <rssi>
    0 -115 dBm or less
    1 -111 dBm
    2...30 -110... -54 dBm
    31 -52 dBm or greater
    99 not known or not detectable
    <ber> (in percent):
    0...7 As RXQUAL values in the table in GSM 05.08 [20]
    subclause 7.2.4
    99 Not known or not detectable
    */
    HwSwSerial.print (F("AT+CSQ\r\n"));
    return(_readSerial());
}

bool Sim800C::answerCall()
{
    return send_cmd_wait_reply(F("ATA\r\n"),RESPON_OK,10000);
}


bool Sim800C::callNumber(String number)
{
    return send_cmd_wait_reply("ATD"+number+";\r\n",RESPON_OK,30000);
}



uint8_t Sim800C::getCallStatus()
{
    /*
      values of return:

     0 Ready (MT allows commands from TA/TE)
     2 Unknown (MT is not guaranteed to respond to tructions)
     3 Ringing (MT is ready for commands from TA/TE, but the ringer is active)
     4 Call in progress

    */
    HwSwSerial.print (F("AT+CPAS\r\n"));
    _buffer=_readSerial();
    return _buffer.substring(_buffer.indexOf("+CPAS: ")+7,_buffer.indexOf("+CPAS: ")+9).toInt();
}


bool Sim800C::hangoffCall()
{
    return send_cmd_wait_reply(F("ATH\r\n"),RESPON_OK,10000);
}

bool Sim800C::send_cmd_wait_reply(String aCmd,const char*aResponExit,uint32_t aTimeoutMax)
{
    HwSwSerial.print(aCmd);
    _buffer=_readSerial(aTimeoutMax);
    if ( (_buffer.indexOf(aResponExit)) != -1)
    {
        delay(100);
        return OK;
    }
    delay(100);
    return ERROR;    
}

bool Sim800C::send_cmd_wait_reply(const __FlashStringHelper *aCmd,const char*aResponExit,uint32_t aTimeoutMax)
{
    HwSwSerial.print(aCmd);
    _buffer=_readSerial(aTimeoutMax);
    if ( (_buffer.indexOf(aResponExit)) != -1)
    {
        delay(100);
        return OK;
    }
    delay(100);
    return ERROR;
}


bool Sim800C::sendSms(char* number,char* text)
{
    // Can take up to 60 seconds
    HwSwSerial.print (F("AT+CMGS=\""));  	// command to send sms
    HwSwSerial.print (number);
    HwSwSerial.print(F("\"\r\n"));
    _buffer=_readSerial(10000);
    if ( (_buffer.indexOf(">")) != -1)
    {
        HwSwSerial.print(text);
        HwSwSerial.print((char)26);
        _buffer=_readSerial(60000);
        //expect CMGS:xxx   , where xxx is a number,for the sending sms.
        if ( (_buffer.indexOf("ER")) == -1)
        {
            return OK;
        }    
    }
    
    return ERROR;
}


String Sim800C::getNumberSms(uint8_t index)
{
    /* +CMGR: "REC UNREAD","+989132383246", "","14/05/08,10:49:04+14"
			 
        MESSAGE TEXT

        OK
	*/
    _buffer=readSms(index);
    //Serial.println(_buffer.length());
    if (_buffer.length() > 10) //avoid empty sms
    {
        uint8_t _idx1=_buffer.indexOf("+CMGR:");
        _idx1=_buffer.indexOf("\",\"",_idx1+1);
        return _buffer.substring(_idx1+3,_buffer.indexOf("\", \"",_idx1+4));
    }
    else
    {
        return "";
    }
}


String Sim800C::readSms(uint8_t index)
{
    HwSwSerial.print (F("AT+CMGR="));
    HwSwSerial.print (index);
    HwSwSerial.print ("\r\n");
    _buffer=_readSerial();
    if (_buffer.indexOf("CMGR:")!=-1)
    {
        return _buffer;
    }
    else return "";
}

bool Sim800C::deleteSMS(uint8_t position)
{
    HwSwSerial.print(F("AT+CMGD="));
    HwSwSerial.print(position);
    HwSwSerial.print(F("\r\n"));

    _buffer=_readSerial(25000);

	if ( (_buffer.indexOf("ER")) == -1)
    {
        return OK;
    }
    else return ERROR;
}

bool Sim800C::delAllSms()
{
    // Can take up to 25 seconds
    return send_cmd_wait_reply(F("AT+CMGDA=\"DEL ALL\"\r\n"),RESPON_OK,25000);
}

uint8_t Sim800C::check_receive_command(String str_out)
{
    _buffer=_readSerial(10);
    int index1,index2;
    if (_buffer.length()>=6)
    {
        if (_buffer.indexOf("+CMTI:")!=-1)   //+CMTI: "SM",i        i=INDEX
        {
            //Sms received
            if(_buffer.length()>=14)
            {
                index1=_buffer.indexOf(",");
			    index2=_buffer.indexOf(cr);
                if(index1>index2)
					index2=_buffer.indexOf(cr,index2+1);
                sms_index=_buffer.substring(index1+1,index2).toInt();
                if(sms_index>0)
                    return Sms_received;
            }
        }
        else if (_buffer.indexOf("+CLIP:")!=-1)  //+CLIP: "+983152401442",145,"",,"",0
        {
            //Calling
            if(_buffer.length()>=11)
            {
                index1=_buffer.indexOf("\"");
                index2=_buffer.indexOf("\"",index1+1);
                str_out=_buffer.substring(index1+4,index2);
                return Calling_with_number;
            }
        }
        else if (_buffer.indexOf("+CUSD:")!=-1)
        {
            index1=_buffer.indexOf("\"");
            index2=_buffer.lastIndexOf("\"");
            if(index1==index2 && index1!=-1)
                index2=_buffer.length();
            
            if (index1!=-1 && index2!=-1)
            {
                str_out=_buffer.substring(index1,index2);
            }
            return CUSD;
        }
        else if (_buffer.indexOf("NO CARRIER")!=-1)
        {
            return NO_CARRIER;
        }
        else if (_buffer.indexOf("RING")!=-1)
        {
            return RING;
        }
        else if (_buffer.indexOf("NO DIALTONE")!=-1)
        {
            return NO_DIALTONE;
        }
        else if (_buffer.indexOf("BUSY")!=-1)
        {
            return BUSY;
        }
        else if (_buffer.indexOf("NO ANSWER")!=-1)
        {
            return NO_ANSWER;
        }
        else if (_buffer.indexOf("MO RING")!=-1)
        {
            return MO_RING;
        }
        else if (_buffer.indexOf("MO CONNECTED")!=-1)
        {
            return MO_CONNECTED;
        }
        else
        {
            return NOT_Recog_Data;
        }
    }
    return No_data;
}

bool Sim800C::miss_call(String aSenderNumber,uint8_t NumOfTry) //NumOfTry 1-255
{
	String str;
    uint8_t st;
    uint32_t del;
	int i;
    bool _break=false;
	for (i = 0; i < NumOfTry; i++)
	{
		if(callNumber(aSenderNumber)==OK)
		{
            del=millis();
			while(millis()-del<10000 && !_break)
			{
				st=check_receive_command(str);
				switch (st)
				{
					case NO_ANSWER:hangoffCall(); i=256;_break=true;
					break;
					case NO_DIALTONE: hangoffCall();_break=true;
					break;
					case BUSY: hangoffCall();_break=true;
					break;
					case MO_RING: hangoffCall(); i=256;_break=true;
					break;
					case MO_CONNECTED: hangoffCall(); i=256;_break=true;
					break;
					case NO_CARRIER: hangoffCall();_break=true;
					break;
				}
			}
			delay(300);
			hangoffCall();
			delay(1000);
		}
		delay(100);
	}
	if(i>=256) return OK;
	return ERROR;
}

void Sim800C::RTCtime(int *day,int *month, int *year,int *hour,int *minute, int *second)
{
    HwSwSerial.print(F("at+cclk?\r\n"));
    // if respond with ERROR try one more time.
    _buffer=_readSerial();
    if ((_buffer.indexOf("ERR"))!=-1)
    {
        delay(50);
        HwSwSerial.print(F("at+cclk?\r\n"));
    }
    if ((_buffer.indexOf("ERR"))==-1)
    {
        _buffer=_buffer.substring(_buffer.indexOf("\"")+1,_buffer.lastIndexOf("\"")-1);
        *year=_buffer.substring(0,2).toInt();
        *month= _buffer.substring(3,5).toInt();
        *day=_buffer.substring(6,8).toInt();
        *hour=_buffer.substring(9,11).toInt();
        *minute=_buffer.substring(12,14).toInt();
        *second=_buffer.substring(15,17).toInt();
    }
}

//Get the time  of the base of GSM
String Sim800C::dateNet()
{
    HwSwSerial.print(F("AT+CIPGSMLOC=2,1\r\n "));
    _buffer=_readSerial();

    if (_buffer.indexOf("OK")!=-1 )
    {
        return _buffer.substring(_buffer.indexOf(":")+2,(_buffer.indexOf("OK")-4));
    }
    else
        return "0";
}

String Sim800C::_readSerial()
{

    uint64_t timeOld = millis();
    int len,i;

    while (!HwSwSerial.available() && !(millis() > timeOld + TIME_OUT_READ_SERIAL))
    {
        delay(13);
    }

    String str;

    while(HwSwSerial.available())
    {
        len=HwSwSerial.available();
        for(i=0;i<len;i++)
        {
            str += (char) HwSwSerial.read();
        }
        delay(15);
    }

    return str;

}

String Sim800C::_readSerial(uint32_t timeout)
{

    uint64_t timeOld = millis();
    int len,i;

    while (!HwSwSerial.available() && !(millis() > timeOld + timeout))
    {
        delay(13);
    }

    String str;

    while(HwSwSerial.available())
    {
        len=HwSwSerial.available();
        for(i=0;i<len;i++)
        {
            str += (char) HwSwSerial.read();
        }
        delay(15);
    }

    return str;

}

