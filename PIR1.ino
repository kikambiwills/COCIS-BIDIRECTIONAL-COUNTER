#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 
SoftwareSerial gsm(10, 11); // Rx, Tx

byte pret0, pret1;
byte inPir = 12, outPir=17, led0=8, led1=10;
byte triggered0=0, triggered1=0;
int numin = 0, numout = 0;
byte inlog[4], outlog[4];
byte indx = 0, outdx = 0, istate = 0;
byte cy0 = 0, cy1 = 0;
byte enc0 = 0, enc1 = 0;
byte starttimer0 = 0, starttimer1 = 0;
int lasttime = 0, timec = 0, maxlimit = 100;
int lasttimex = 0, timex = 0;
int lasttimeu = 0, timeu = 0;

void setup()
 { // put your setup code here, to run once:
   pinMode(inPir, INPUT);
   pinMode(led0, OUTPUT);pinMode(9, OUTPUT);
   pinMode(led1, OUTPUT);pinMode(11, OUTPUT);
   pinMode(outPir, INPUT);
   digitalWrite(9, 0); //led0
   digitalWrite(16, 0); // led1
   gsm.begin(9600);
   Serial.begin(9600);
   lcd.begin(16, 2);
   lcd.backlight();
   lcd.print("Please Wait...");
   lcd.setCursor(0, 1);
   lcd.print("Initializing...");
   gsm.println("AT");
   delay(1000);
   gsm.println("AT+CMGF=1");
   delay(1000);
   gsm.println("AT+CNMI=2,2,0,0,0");
   delay(1000);
   data_init();
   internet_init();
   lcd.clear();
 }

void loop()
{ // put your main code here, to run repeatedly:
   pret0 = digitalRead(inPir);
   pret1 = digitalRead(outPir);
   lcd.clear(); lcd.setCursor(0, 0);
   lcd.print("IN        OUT");
   lcd.setCursor(0, 1); lcd.print(numin);
   lcd.setCursor(11, 1); lcd.print(numout);
   if (pret0 == 1){
      triggered0 = 1; starttimer0 = 1;
      digitalWrite(8, 1);
   }else{
      digitalWrite(8, 0);
      triggered0 = 0;
   }
   
   if (pret1 == 1){
      triggered1 = 1; starttimer1 = 1;
      digitalWrite(15, 1);
   }else{
      digitalWrite(15, 0);
      triggered1 = 0;
   }

   if (starttimer0 == 1){
      boolean tick = getTimer0(25);
      if (tick){
        Serial.println("TICKING DOWN0---------");
        triggered0 = 0; starttimer0 = 0;
      }
   }
   if (starttimer1 == 1){
      boolean tick = getTimer1(25);
      if (tick){
        Serial.println("TICKING DOWN1---------");
        triggered1 = 0; starttimer1 = 0;
      }
   }
  
   if (triggered0 == 1 && triggered1 == 0){
      istate = 1;
   }else if (triggered0 == 1 && triggered1 == 1){
      istate = 2;
   }else if (triggered0 == 0 && triggered1 == 1){
      istate = 3;
   }else if (triggered0 == 0 && triggered1 == 0){
      istate = 4;
   }else
      istate = 0;
   
   Serial.print("ISTATE: "); Serial.println(istate);
   delay(100);
   
   if (istate == 1){
      inlog[0] = 1; cy0 = 0;
      if (outlog[1] == 2 && outlog[2] != 1){
        cy1++; outlog[2] = 1;
      }else if (outlog[1] != 2){
        cy1 = 0;
        for (int i = 0; i < 4; i++) outlog[i] = 0; // reset
      }
   }else if (istate == 2){
      if (inlog[0] == 1 && inlog[1] != 2){
        cy0++; inlog[1] = 2;
      }else if (inlog[0] != 1){
        cy0 = 0;
        for (int i = 0; i < 4; i++) inlog[i] = 0;
      }
      if (outlog[0] == 3 && (outlog[1] != 2)){
        cy1++; outlog[1] = 2;
      }else if (outlog[0] != 3){
        cy1 = 0;
        for (int i = 0; i < 4; i++) outlog[i] = 0;
      }
   }else if (istate == 3){
      outlog[0] = 3; cy1 = 0;
      if (inlog[1] == 2 && inlog[2] != 3){
        cy0++; inlog[2] = 3;
      }else if (inlog[1] != 2){
        cy0 = 0;
        for (int i = 0; i < 4; i++) inlog[i] = 0;
      }
   }

   if (cy0 >= 2){
      enc0 = 1;
      cy0 = 0;
      for (int i = 0; i < 4; i++) inlog[i] = 0;
   }
   if (cy1 >= 2){
      enc1 = 1;
      cy1 = 0; 
      for (int i = 0; i < 4; i++) outlog[i] = 0;
   }
   
   if (enc0 == 1){
      enc0 = 0; numin++;cy1 = 0;cy0 = 0;
      for (int i = 0; i < 4; i++) inlog[i] = 0;
      for (int i = 0; i < 4; i++) outlog[i] = 0;
   }
   if (enc1 == 1){
      enc1 = 0; numout++;cy0 = 0;cy1 = 0;
      for (int i = 0; i < 4; i++) outlog[i] = 0;
      for (int i = 0; i < 4; i++) inlog[i] = 0;
   }
   // upload section
   boolean upld = uploadTimer(25);
   if (upld){
      Serial.println("UPLOAD IN PROGRESS");
      upload(); 
   }
}

boolean getTimer0(int lmt){
   timec += 1;
   int elapse = timec - lasttime;
   if (timec >= maxlimit) 
      timec = 0;
   if (elapse < 0)
      elapse = (maxlimit + 1 + timec) - lasttime;
   if (elapse >= lmt){
      lasttime = timec;
      return true;
   }else
      return false;
}

boolean getTimer1(int lmt){ // timer for sensor1
   timex += 1;
   int elapse = timex - lasttimex;
   if (timex >= maxlimit) 
      timex = 0;
   if (elapse < 0)
      elapse = (maxlimit + 1 + timex) - lasttimex;
   if (elapse >= lmt){
      lasttimex = timex;
      return true;
   }else
      return false;
}

boolean uploadTimer(int lmt){ // timer for sensor1
   timeu += 1;
   int elapse = timeu - lasttimeu;
   if (timeu >= maxlimit) 
      timeu = 0;
   if (elapse < 0)
      elapse = (maxlimit + 1 + timeu) - lasttimeu;
   if (elapse >= lmt){
      lasttimeu = timeu;
      return true;
   }else
      return false;
}

void data_init()
{
  Serial.println("Please wait.....");
  gsm.println("AT");
  delay(1000); delay(1000);
  gsm.println("AT+CPIN?");
  delay(1000); delay(1000);
  gsm.print("AT+SAPBR=3,1");
  gsm.write(',');
  gsm.write('"');
  gsm.print("contype");
  gsm.write('"');
  gsm.write(',');
  gsm.write('"');
  gsm.print("GPRS");
  gsm.write('"');
  gsm.write(0x0d);
  gsm.write(0x0a);
  delay(1000); ;
  gsm.print("AT+SAPBR=3,1");
  gsm.write(',');
  gsm.write('"');
  gsm.print("APN");
  gsm.write('"');
  gsm.write(',');
  gsm.write('"');
  gsm.print("internet"); //APN
  gsm.write('"');
  gsm.write(0x0d);
  gsm.write(0x0a);
  delay(1000);
  gsm.print("AT+SAPBR=3,1");gsm.write(',');gsm.write('"');
  gsm.print("USER");gsm.write('"');gsm.write(',');
  gsm.write('"');gsm.print("  ");gsm.write('"');
  gsm.write(0x0d);
  gsm.write(0x0a);
  delay(1000);
  gsm.print("AT+SAPBR=3,1");
  gsm.write(','); gsm.write('"');gsm.print("PWD");
  gsm.write('"');gsm.write(','); gsm.write('"');
  gsm.print("  ");
  gsm.write('"');gsm.write(0x0d);gsm.write(0x0a); //newlinecarriage
  delay(2000);
  gsm.print("AT+SAPBR=1,1");
  gsm.write(0x0d);gsm.write(0x0a);
  delay(3000);
}

void internet_init()
{
  Serial.println("Please wait.....");
  delay(1000);
  gsm.println("AT+HTTPINIT");
  delay(1000); delay(1000);
  gsm.print("AT+HTTPPARA=");
  gsm.print('"');
  gsm.print("CID");
  gsm.print('"');
  gsm.print(',');
  gsm.println('1');
  delay(1000);
}
void upload()
{
  gsm.print("AT+HTTPPARA=");
  gsm.print('"');
  gsm.print("URL");
  gsm.print('"');
  gsm.print(',');
  gsm.print('"');
  gsm.print("http:");
  gsm.print('/');gsm.print('/');
  gsm.print("api.thingspeak.com/update?api_key=OUMZ7PZ8D1GAGBPG&field1="); 
  gsm.print(numin);
  gsm.print("&field2=");gsm.print(numout);
  gsm.write(0x0d);
  gsm.write(0x0a);
  delay(1000);
  gsm.println("AT+HTTPACTION=0");
  delay(1000);
}
