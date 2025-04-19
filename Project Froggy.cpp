#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <time.h>                 //for Time Reciever.
#include <Adafruit_ADS1015.h>     //for ADS1115
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>          //Encode and Decode JSON
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>



//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <Ticker.h>              //If we use Ticker to send data to the database server
#include <EEPROM.h>              //To save checkpoint for some reason.



Adafruit_ADS1115 ads;            /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;          /* Use thi for the 12-bit version */
Adafruit_BME280 bme;             /* SCL d1 SDA d2*/
#define DayLight D5
#define MoonLight D6
#define Sprinkle D7
#define Cooler D8
#define ResetButton 3 // RX button

boolean flagwifi=false;
const char* modes;
const char* relay1;
const char* relay2;
const char* relay3;
const char* relay4;


float TempRange = 27;  
float HumidRange = 60; //TempSetRange
int Hour1 = 6;
int Hour2 = 18;
int Minute1 = 0; 
int Minute2 = 0; // TimeSetRange
//int count = 0; // for esp reset when wifi are not connected
//boolean ResetBoard = true ;
int SprinkleState = LOW;
char Mode, Relay1 , Relay2 , Relay3 , Relay4;
long OnTime = 2000;           // milliseconds of on-time 10 sec
long OffTime = 500; 
unsigned long previousMillis = 0;

const char *host = "40b.online";

int OnlineStatus; 

Ticker set_http , update_http , retrieving_http;
Ticker pumper;

WiFiClient espClient;
PubSubClient client(espClient);
//----------------------------------- Status of working (On,Off) & Mode select -----------
int eeAddress1 = 0; // ตำแหน่งที่ 00 ของ EEPROM to storage sprinkletik
int state_1 = 0; // 0 for off and 1 for on
int state_2 = 0; // 0 for off and 1 for on
int state_3 = 0; // 0 for off and 1 for on
//char Mode = '0'; // false for auto and true for manual
int sprinkletik ; // for control sprinkle work int time
int sethttp = 0;
int setpump = 0;
int updatehttp = 0;
int selectDB = 1;
int selectDB2 = 1;

void http1_tick(){
  sethttp = 1 ;
}
void http2_tick(){
  updatehttp = 1 ;
}
void http_select(){
  selectDB = 1;
  selectDB2 = 1;
}

float temperature, humidity, pressure, altitude; // for BME280
//----------------------------------- Time Zone ------------------------------------------

int timezone = 7 * 3600;                    //ตั้งค่า TimeZone ตามเวลาประเทศไทย
int dst = 0;                                //กำหนดค่า Date Swing Time 
int Hour , Min ;

//--------------------------------------sprinkletik startup----------------------

void set_sprinkletik() { // to control not to do twice in below time
   if (Hour > 6 && Hour < 12){ 
      sprinkletik = 0; 
   }
   else if (Hour > 11 && Hour < 18){ 
    sprinkletik = 1 ;
   }
   else { 
    sprinkletik = 2 ;
   }
}

//------------------------------Temp&Humid---------------------------
void temp(){
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
 // pressure = bme.readPressure() / 100.0F;

  printf ("\tTempurature From BME 280 : %.2f\n",temperature);
  printf ("\tHumidity From BME 280 : %.2f\n",humidity);
  //printf ("\tPressure From BME 280 : %.2f\n",pressure);
  delay(250);
}

//------------------------------  pump mechanic -----------------------
void pumpersetup(){
    setpump = 1;
}

// UVA and UVB
void UV (){ //Open the Daylight by day time
  if (Hour >= Hour1 && Hour < Hour2){ 
    digitalWrite(DayLight, HIGH); 
    digitalWrite(MoonLight, LOW);
    Serial.println("DayLight in Auto on");
    Serial.println("MoonLight in Auto off");
  //  Serial.println("It's Me Day LIGht");
  }
  else{  
    digitalWrite(MoonLight, HIGH); 
    digitalWrite(DayLight, LOW);
    Serial.println("MoonLight in Auto on");
    Serial.println("DayLight in Auto off");
    //Serial.println("It's Me Moon LIGht");
  }
}
//----------------------------------  AC voltage -----------------------------------
    
    int mVperAmp1 = 185 ; // use 100 for 20A Module and 66 for 30A Module and 185 for 5A Module
    double Voltage1 = 0;
    double VRMS1 ;
    double AmpsRMS1 ;
    float CurrentValue1 ;

    int mVperAmp2 = 185 ; // use 100 for 20A Module and 66 for 30A Module and 185 for 5A Module
    double Voltage2 = 0;
    double VRMS2 ;
    double AmpsRMS2 ;
    float CurrentValue2 ;

    int mVperAmp3 = 185 ; // use 100 for 20A Module and 66 for 30A Module and 185 for 5A Module
    double Voltage3 = 0;
    double VRMS3;
    double AmpsRMS3 ;
    float CurrentValue3 ;

    int mVperAmp4 = 185 ; // use 100 for 20A Module and 66 for 30A Module and 185 for 5A Module
    double Voltage4 = 0;
    double VRMS4 ;
    double AmpsRMS4 ;
    float CurrentValue4 ;

//--------------------------------- ADS Setup --------------------------------------
  int16_t adc0, adc1, adc2, adc3;
//-------------------------------------Sensor1---------------------------------------

void RMS1(){ 
 Voltage1 = getVPP1();
 VRMS1 = (Voltage1/2.0) *0.707;  //root 2 is 0.707
 AmpsRMS1 = ((VRMS1 * 1000)/mVperAmp1)+ 0.33;
 Serial.print(AmpsRMS1);
 Serial.println(" Amps RMS1");

 CurrentValue1 = AmpsRMS1 ;
 //client.publish(RMS1_topic,String(CurrentValue1).c_str(), true );
// Firebase.setFloat("ARMS_Amp", CurrentValue1);
}


//-------------------------------------Sensor2---------------------------------------

void RMS2(){ 
 Voltage2 = getVPP2();
 VRMS2 = (Voltage2/2.0) *0.707;  //root 2 is 0.707
 AmpsRMS2 = ((VRMS2 * 1000)/mVperAmp2) -0.15;
 Serial.print(AmpsRMS2);
 Serial.println(" Amps RMS2");

 CurrentValue2 = AmpsRMS2 ;
 //client.publish(RMS2_topic,String(CurrentValue2).c_str(), true );
// Firebase.setFloat("ARMS_Amp", CurrentValue2);
}

//-------------------------------------Sensor3---------------------------------------

void RMS3(){ 
 Voltage3 = getVPP3();
 VRMS3 = (Voltage3/2.0) *0.707;  //root 2 is 0.707
 AmpsRMS3 = (VRMS3 * 1000)/mVperAmp3;
 Serial.print(AmpsRMS3);
 Serial.println(" Amps RMS3");

 CurrentValue3 = AmpsRMS3 ;
 //client.publish(RMS3_topic,String(CurrentValue3).c_str(), true );
// Firebase.setFloat("ARMS_Amp", CurrentValue2);
}

//-------------------------------------Sensor4---------------------------------------

void RMS4(){ 
 Voltage4 = getVPP4();
 VRMS4 = (Voltage4/2.0) *0.707;  //root 2 is 0.707
 AmpsRMS4 = (VRMS4 * 1000)/mVperAmp4;
 Serial.print(AmpsRMS4);
 Serial.println(" Amps RMS4");

 CurrentValue4 = AmpsRMS4 ;
// client.publish(RMS4_topic,String(CurrentValue4).c_str(), true );
// Firebase.setFloat("ARMS_Amp", CurrentValue2);
}

//---------------------------------------- VPP GET1 ------------------------------------

float getVPP1()
{
  float result1;
  int readValue1;             //value read from the sensor
  int maxValue1 = 0;          // store max value here
  int minValue1 = 1024;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue1 = ads.readADC_SingleEnded(0); // รออ่านจาก ADS 1115 A0
       // see if you have a new maxValue
       if (readValue1 > maxValue1) 
       {
           /*record the maximum sensor value*/
           maxValue1 = readValue1;
       }
       if (readValue1 < minValue1) 
       {
           /*record the minimum sensor value*/
           minValue1 = readValue1;
       }
   }
   
   // Subtract min from max
   result1 = ((maxValue1 - minValue1) * 5.0)/1024.0;
      
   return result1;
 }

//---------------------------------------- VPP GET2 ------------------------------------

float getVPP2()
{
  float result2;
  int readValue2;             //value read from the sensor
  int maxValue2 = 0;          // store max value here
  int minValue2 = 1024;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue2 = ads.readADC_SingleEnded(1); // รออ่านจาก ADS 1115 A1
       // see if you have a new maxValue
       if (readValue2 > maxValue2) 
       {
           /*record the maximum sensor value*/
           maxValue2 = readValue2;
       }
       if (readValue2 < minValue2) 
       {
           /*record the minimum sensor value*/
           minValue2 = readValue2;
       }
   }
   
   // Subtract min from max
   result2 = ((maxValue2 - minValue2) * 5.0)/1024.0;
      
   return result2;
 }

//---------------------------------------- VPP GET3 ------------------------------------

float getVPP3()
{
  float result3;
  int readValue3;             //value read from the sensor
  int maxValue3 = 0;          // store max value here
  int minValue3 = 1024;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue3 = ads.readADC_SingleEnded(2); // รออ่านจาก ADS 1115 A2
       // see if you have a new maxValue
       if (readValue3 > maxValue3) 
       {
           /*record the maximum sensor value*/
           maxValue3 = readValue3;
       }
       if (readValue3 < minValue3) 
       {
           /*record the minimum sensor value*/
           minValue3 = readValue3;
       }
   }
   
   // Subtract min from max
   result3 = ((maxValue3 - minValue3) * 5.0)/1024.0;
      
   return result3;
 }

//---------------------------------------- VPP GET4 ------------------------------------

float getVPP4()
{
  float result4;
  int readValue4;             //value read from the sensor
  int maxValue4 = 0;          // store max value here
  int minValue4 = 1024;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue4 = ads.readADC_SingleEnded(3); // รออ่านจาก ADS 1115 A3
       // see if you have a new maxValue
       if (readValue4 > maxValue4) 
       {
           /*record the maximum sensor value*/
           maxValue4 = readValue4;
       }
       if (readValue4 < minValue4) 
       {
           /*record the minimum sensor value*/
           minValue4 = readValue4;
       }
   }
   
   // Subtract min from max
   result4 = ((maxValue4 - minValue4) * 5.0)/1024.0;
      
   return result4;
 }
 
//------------------------------------Time Recieve-----------------------------------------

void checktime(){ 
  
   time_t now = time(nullptr);
   struct tm* p_tm = localtime(&now);
 //  sectest = p_tm->tm_sec ;
                                  //เผื่อนำค่านี้มาใช้งาน เป็นการนำเอาชั่วโมงจาก RTC มาใช้สั่งการทำงานของ DayLight MoonLight
    Hour = p_tm->tm_hour ; 
    Min = p_tm->tm_min ;
    //if (WiFi.status() != WL_CONNECTED){
    if (flagwifi == false) {
      EEPROM.write(1,Hour);
      EEPROM.write(2,Min);
      EEPROM.commit();
    }
  }

//----------------------------------------------------------------------------------------
void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.flush();
    bme.begin(0x76);
     
    Mode = '0';
    pinMode(DayLight,OUTPUT);
    pinMode(MoonLight,OUTPUT);
    pinMode(Sprinkle,OUTPUT);
    pinMode(Cooler,OUTPUT);
    digitalWrite(Cooler,HIGH);
    
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around  
    WiFiManager wifiManager;
    //wifiManager.autoConnect("AutoConnectAP");
    WiFi.begin();
    WiFi.setAutoReconnect(true);

    //----------------------------------- ADS 1115 -----------------------------------

    Serial.println("Getting single-ended readings from AIN0..3");
    Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
    ads.begin();

    //time
    EEPROM.begin(512);
    //if (WiFi.status() == WL_CONNECTED){
    if (flagwifi == true) {
      configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");     //ดึงเวลาจาก Server
      Serial.println("\nWaiting for time");
      
      while (!time(nullptr)) {
         Serial.print(".");
         delay(1000);
        }   
    }
    else {
      Hour = EEPROM.read(1);
      Min = EEPROM.read(2);
      }
    
    
      
    //checktime();
    set_http.attach(300, http1_tick); // ส่งค่าแบบ Insert ขึ้น DB ทุก 5 นาที
    update_http.attach(3, http2_tick); // ส่งค่าแบบ update ขึ้น DB ทุก 1 นาที
    retrieving_http.attach(4 ,http_select);
    pumper.attach(48, pumpersetup); // ทุก 3 นาที จะเช็คว่า temp มากกว่า 30 ไหม ถ้ามากกว่าให้ setpump = 1 
  //pumper + เผื่อไว้ 18 วิ เพราะ เวลา pump ทำงาน millis กับ ticker จะทำงานพร้อมกันทำให้รอบรอ รอบหน้าไม่ใช่ เลข ที่ใส่ จริงๆ

   
}

void pump2(){
    // check to see if it's time to change the state of the LED
     if (humidity <= HumidRange){
        int checkpump = 1;
       
     if(checkpump == 1){  
               Serial.print("Checkpump in pump2 : ");
        Serial.println(checkpump);
       unsigned long currentMillis = millis();
       Serial.println(currentMillis);   
         if ((SprinkleState == LOW) && (currentMillis - previousMillis >= OffTime))
          {
            SprinkleState = HIGH;  // turn it on
            previousMillis = currentMillis;   // Remember the time
            digitalWrite(Sprinkle, SprinkleState);    // Update the actual LED
          }
           else if((SprinkleState == HIGH) && (currentMillis - previousMillis >= OnTime))
          {
            SprinkleState = LOW;  // Turn it off
            previousMillis = currentMillis;  // Remember the time
            digitalWrite(Sprinkle, SprinkleState);  // Update the actual LED
            Relay3 = '0' ;
            setRelay3HTTP();
            Serial.println("Sprinkle Off");
            setpump = 0 ;
            checkpump = 0;
          }
     } 
    }
}


void pump3(){
    // check to see if it's time to change the state of the LED
     if (humidity <= HumidRange){
        int checkpump = 1;
       
     if(checkpump == 1){  
               Serial.print("Checkpump in pump2 : ");
        Serial.println(checkpump);
       unsigned long currentMillis = millis();
       Serial.println(currentMillis);   
         if ((SprinkleState == LOW) && (currentMillis - previousMillis >= OffTime))
          {
            SprinkleState = HIGH;  // turn it on
            previousMillis = currentMillis;   // Remember the time
            digitalWrite(Sprinkle, SprinkleState);    // Update the actual LED
          }
           else if((SprinkleState == HIGH) && (currentMillis - previousMillis >= OnTime))
          {
            SprinkleState = LOW;  // Turn it off
            previousMillis = currentMillis;  // Remember the time
            digitalWrite(Sprinkle, SprinkleState);  // Update the actual LED
            Relay3 = '0' ;
            //setRelay3HTTP();
            Serial.println("Sprinkle Off");
            setpump = 0 ;
            checkpump = 0;
          }
     } 
    }
}
void pumpManual(){
    // check to see if it's time to change the state of the LED
      unsigned long currentMillis = millis();
      Serial.println(currentMillis);
      Serial.println("Previous");
      Serial.println(previousMillis);
      if(Relay3 == '1'){
         if ((SprinkleState == LOW) && (currentMillis - previousMillis >= OffTime))
          {
            SprinkleState = HIGH;  // turn it on
            previousMillis = currentMillis;   // Remember the time
            digitalWrite(Sprinkle, SprinkleState);    // Update the actual LED
          }
           else if((SprinkleState == HIGH) && (currentMillis - previousMillis >= OnTime))
          {
            Serial.println("Time to close sprinkle");
            SprinkleState = LOW;  // Turn it off
            previousMillis = currentMillis;  // Remember the time
            digitalWrite(Sprinkle, SprinkleState);  // Update the actual LED
            Relay3 = '0' ;
            setRelay3HTTP();
            Serial.println("Sprinkle Off");
            setpump = 0 ;
          }
        
      }

}  
void GetHTTP1(){ //CurrentValue1
  if (flagwifi == true) {
     //GET Data ตั้ง HTTP Get ตามต้องการ
    HTTPClient http;
    String url1 = "?temperature=" + String(temperature)+ "&humidity=" +String(humidity);
    String Link = "http://35.247.158.120/get_db.php" + url1; //old one
    
  
    
    http.begin(Link); //https://circuits4you.com website or IP address of server
    int httpCode = http.GET(); //Send the request
    String payload = http.getString(); //Get the response payload
   // Serial.println(payload); //Print request response payload
    http.end(); //Close connection
    sethttp = 0 ;
  }
}

void GetHTTP2(){
    if (flagwifi == true) {
       //GET Data ตั้ง HTTP Get ตามต้องการ
      HTTPClient http;
      String url2 = "?temperature=" + String(temperature)+ "&humidity=" +String(humidity) + "&vcs1=" + String(CurrentValue1) + "&vcs2=" + String(CurrentValue2) + "&vcs3=" + String(CurrentValue3)  + "&vcs4=" + String(CurrentValue4)  ;
      String Link = "http://35.247.158.120/update_db.php" + url2; //new one 3/1/2020
    
      
      http.begin(Link); //https://circuits4you.com website or IP address of server
      int httpCode = http.GET(); //Send the request
      String payload = http.getString(); //Get the response payload
      //Serial.println(payload); //Print request response payload
      http.end(); //Close connection
      updatehttp = 0 ;
    }
}
void setRelay3HTTP(){
    if (flagwifi == true) {
      HTTPClient http;
      Serial.println("now in SetRelayHTTP");
      Serial.print("Relay3 : ");
      Serial.print(Relay3);
      Serial.println("");
      http.begin("http://35.247.158.120/update_relay3.php?Relay3="+ String(Relay3) );
      int httpCode = http.GET();
      String payload = http.getString(); //Get the response payload
      http.end(); //Close connection
    }
} 
  
void Retrieve_relay(){
    if(selectDB == 1 && flagwifi == true){
      
      HTTPClient http;  //Object of class HTTPClient
      http.begin("http://35.247.158.120/Retrieving_Relay.php");
      int httpCode = http.GET();
      String payload = http.getString(); //Get the response payload
 //     Serial.println(payload); //Print request response payload
      //Check the returning code                                                                  
        // Parsing
      const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;        
      DynamicJsonDocument jsonBuffer(bufferSize);
      auto error = deserializeJson(jsonBuffer,http.getString());
  
     //  Serial.println(jsonBuffer["ModeCheck"].as<char*>()); //ชี้เข้า ModeCheck ที่อัดลง JsonObj ในที่นี้ชี้ 1
        relay1 = jsonBuffer["Relay1"].as<char*>();
        relay2 = jsonBuffer["Relay2"].as<char*>();
        relay3 = jsonBuffer["Relay3"].as<char*>();
        relay4 = jsonBuffer["Relay4"].as<char*>();
        modes = jsonBuffer["ModeCheck"].as<char*>();
        Mode = *modes;
        Relay1 = *relay1;
        Relay2 = *relay2;
        Relay3 = *relay3;
        Relay4 = *relay4;
        
        http.end();   //Close connection
        Serial.print("Mode : ");
        Serial.println(Mode);   
        selectDB = 0;

        
    }
}

void Retrieve_temprange(){
    if(selectDB2 == 1 && flagwifi == true){
      HTTPClient http;  //Object of class HTTPClient
      http.begin("http://35.247.158.120/getRange.php");
      int httpCode = http.GET();
      String payload = http.getString(); //Get the response payload
 //     Serial.println(payload); //Print request response payload
      //Check the returning code                                                                  
        // Parsing
      const size_t bufferSize2 = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;        
      DynamicJsonDocument jsonBuffer2(bufferSize2);
      auto error = deserializeJson(jsonBuffer2,http.getString());
  
     //  Serial.println(jsonBuffer["ModeCheck"].as<char*>()); //ชี้เข้า ModeCheck ที่อัดลง JsonObj ในที่นี้ชี้ 1

        TempRange = jsonBuffer2["tempRange"].as<float>();
        HumidRange = jsonBuffer2["humidRange"].as<float>();
        Hour1 = jsonBuffer2["Hour1"].as<int>();
        Hour2 = jsonBuffer2["Hour2"].as<int>();
        Minute1 = jsonBuffer2["Minute1"].as<int>();
        Minute2 = jsonBuffer2["Minute2"].as<int>();
        Serial.print("HumidRange : ");
        Serial.println(HumidRange);
  Serial.print("TempRange : ");
        Serial.println(TempRange);
        Serial.print("H1Range : ");
        Serial.println(Hour1);
        Serial.print("H2Range : ");
        Serial.println(Hour2);
      /*  Serial.print("Minute1 : ");
        Serial.println(Minute1);
        Serial.print("Minute2 : ");
        Serial.println(Minute2);*/
      selectDB2 = 0;
    }
}

void  coolerSet(){
  if (temperature > TempRange ){  // sample Temprange 29 workat 30 stop at 24
    digitalWrite(Cooler,HIGH); //
  }
  else if (temperature <= TempRange){
    digitalWrite(Cooler,LOW);
  }
  else ;
}
void mode_auto(){
   /* Serial.print("Mode in Auto : ");
    Serial.println(Mode);*/
    if(Mode == '0' && flagwifi == true) {
   // digitalWrite(Cooler,HIGH); // Wait for cooler set
    coolerSet();
    Serial.print("TempRangeOutSide CoolerSet : ");
    Serial.println(TempRange);
    Retrieve_relay();
    Retrieve_temprange(); 
      checktime();
      temp();
      RMS1();
      RMS2();
      RMS3();
      RMS4(); 
      Serial.println("Auto_Mode");
      UV(); //Day & Moon in one func
 //     Serial.println("After UV(); Auto");
     // Serial.println("After UV");
      if(setpump == 1) {
       // Serial.println("In Set Pump");
        pump2();  
        
     }
      if (sethttp == 1){ //เผื่อกำหนดการำทงานของ GetHTTP ผ่าน ticker (ticker ไม่สามารถประมวลผลเองได้)
         // Serial.println("In HTTP1");
          GetHTTP1();
      }
      else if (updatehttp == 1){ //เผื่อกำหนดการำทงานของ GetHTTP ผ่าน ticker (ticker ไม่สามารถประมวลผลเองได้)
       //  Serial.println("In HTTP2");
          GetHTTP2();
      }
 //     Serial.println("Before Retrieve_relay");

//      Serial.println("After Retrieve_relay");   
    }
  
}
void mode_manual(){
/*    Serial.print("Mode in Manual : ");
    Serial.println(Mode);*/
    if (Mode == '1' && flagwifi == true ){
      pumpManual();
      Retrieve_relay();
      Retrieve_temprange();
      temp();
      RMS1();
      RMS2();
      RMS3();
      RMS4(); 
        Serial.print("Relay1 : ");
        Serial.println(Relay1); 
        Serial.print("Relay2 : ");
        Serial.println(Relay2); 
        Serial.print("Relay3 : ");
        Serial.println(Relay3); 
        Serial.print("Relay4: ");
        Serial.println(Relay4); 
      Serial.println("Manual_Mode");
      if(Relay1 == '1'){
        digitalWrite(DayLight,HIGH);
        Serial.println("DayLight ON");
      }
      else {
        digitalWrite(DayLight,LOW);
        Serial.println("DayLight Off");
      }
      if (Relay2 == '1'){
        digitalWrite(MoonLight,HIGH);
        Serial.println("MoonLight ON");
      }
      else {
        digitalWrite(MoonLight,LOW);
        Serial.println("MoonLight Off");
      }
      if (Relay4 == '1'){
        digitalWrite(Cooler,HIGH);
        Serial.println("Cooler ON");
      }
      else {
        digitalWrite(Cooler,LOW);
        Serial.println("Cooler Off");
      }
      
      if (sethttp == 1){ //เพื่อกำหนดการำทงานของ GetHTTP ผ่าน ticker (ticker ไม่สามารถประมวลผลเองได้)
          GetHTTP1();
      }
      else if (updatehttp == 1){ //เพื่อกำหนดการำทงานของ GetHTTP ผ่าน ticker (ticker ไม่สามารถประมวลผลเองได้)
          GetHTTP2();
      }       

    }
}

void loop() {
 // Serial.print("WifiStatus: ");
 // Serial.println(WiFi.status());
  //WiFi.begin();
  if (WiFi.status() != WL_CONNECTED) {
      flagwifi=false;
      Serial.println("The internet is down");
      coolerSet();
      checktime();
      temp();
      UV(); 
      if(setpump == 1) {
          pump3();    
      }
    }
   else {
    flagwifi=true;
    mode_auto();
    mode_manual();
    }
   
    
}