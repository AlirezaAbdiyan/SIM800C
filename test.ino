#include "Sim800C.h"
#include <SoftwareSerial.h>


Sim800C GSM;

String str;

void setup() {
  Serial.begin(9600);
  GSM.begin(9600);
  GSM.callNumber("09132383246");

}

void loop() 
{
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

