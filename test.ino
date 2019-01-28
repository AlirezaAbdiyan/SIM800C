#include "Sim800C.h"
#include <SoftwareSerial.h>


Sim800C GSM;

//String str,;
char phone_number[15],str[400];

void setup() {
  Serial.begin(9600);
  GSM.begin(9600);
  //GSM.delAllSms();
  //GSM.callNumber("*140#");
  //GSM.miss_call("09132383246",2);
  //GSM.AddToWhiteList(Enable_call_and_SMS,4,"9131234568");
  
  Serial.println(GSM.whiteListStatus(str));
  Serial.println(str);

  //GSM.sendSms("09132383246","Test send sms");
}

void loop() 
{
  //GSM.miss_call("09138696632",2);
  switch(GSM.check_receive_command())
  {
    case Sms_received:
      Serial.print("Sms received, sms index is: ");
      Serial.println(GSM.sms_index);
    break; 

    case Calling_with_number:
      Serial.print("Call from: ");
      Serial.println(GSM.SimBuffer);
    break; 

    case CUSD:
      Serial.print("USSD Message: ");
      Serial.println(GSM.SimBuffer);
    break; 

    case NO_CARRIER:
      Serial.println("NO CARRIER");
    break; 

    case NO_DIALTONE:
      Serial.println("NO DIALTONE");
    break; 

    case BUSY:
      Serial.println("BUSY");
    break; 

    case NO_ANSWER:
      Serial.println("NO ANSWER");
    break; 

    case MO_RING:
      Serial.println("MO RING");
    break; 

    case MO_CONNECTED:
      Serial.println("MO CONNECTED");
    break; 

    case NOT_Recog_Data:
      Serial.println("NOT Recog Data");
    break; 
  };

  if(GSM.sms_index!=NoSMS)
  {
    if(GSM.readSms(GSM.sms_index,phone_number,str)==GETSMS_UNREAD_SMS)  
    if(str!="")
    {
      Serial.println(str);  
      Serial.println(phone_number);
      //GSM.delAllSms();
    }
  }
}

