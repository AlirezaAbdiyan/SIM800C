#include "Sim800C.h"
#include <SoftwareSerial.h>


Sim800C GSM;

String str;
uint8_t c;

void setup() {
  Serial.begin(9600);
  GSM.begin(9600);
  GSM.callNumber("*555#");
  //GSM.miss_call("09132383246",2);

}

void loop() 
{
  //GSM.miss_call("09138696632",2);
  c=GSM.check_receive_command();
  switch(c)
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
  /*str=GSM.readSms(1);
  if(str!="")
  {
    Serial.println(str);  
    Serial.println(GSM.getNumberSms(1));
    GSM.delAllSms();
  }*/
  //Serial.println(GSM.getCallStatus());
  //delay(1000);
}

