/**
* Контроллер управления вытяжным вентилятором. Версия 2.0 WiFi
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>


#include <Wire.h>

#include <TM1637.h>
#include <DHT.h> 


//#include <Time.h>
#include <EEPROM.h>
//#include <arduino.h>

#include "WC_EEPROM.h"
#include "WC_HTTP.h"
#include "WC_NTP.h"

#define TM1637_CLK  5 
#define TM1637_DIO  4
#define PIN_COOLER  13
#define PIN_BUTTON  12
#define PIN_DHT     14
#define ERROR_VALUE 2147483647

// Состояния FSM
enum TMode
  {
  tmWait,       //Режим ожидания
  tmNeedPower,  //Требуется включение вентилятора
  tmAutoPower,  //Вентилятор работает в автоматическом режиме
  tmManualPower //Вентилятор работает в ручном режиме
};


TM1637 tm1637(TM1637_CLK,TM1637_DIO);
DHT dht(PIN_DHT, DHT11, 15); 

uint32_t ms0        = 0;
uint32_t ms1        = 0;
uint32_t ms2        = 0;
uint32_t ms3        = 0;
uint32_t ms4        = 0;
uint32_t ms5        = 0;
bool dp             = false;
int br              = 16;
int Hum             = 0;
int HumPrev         = 0;
int Temp            = 0;
int Avalue          = 0;
int Count1          = 0;
int Count2          = 0;
uint32_t first_tm   = 0;
uint32_t tm         = 0;
uint32_t t_cur      = 0;    
long  t_correct     = 0;
uint16_t err_count  = 0;
bool points         = true;
uint8_t disp_mode   = 0;

bool press_button1  = false;
uint32_t ms_button1 = 0;

// Флаг включения света
bool flag_light = false;
// Флаг нажатия кнопки
bool flag_btn   = false;
// Флаг срабатывания по влажности
bool flag_hum   = false;
// Флаг работы вентилятора
bool flag_cool  = false;
// Считчик таймера работы вентилятора
uint16_t timer   = 0;
TMode mode         = tmWait;



void setup() {
// Последовательный порт для отладки
   Serial.begin(115200);
   Serial.printf("\n\nFree memory %d\n",ESP.getFreeHeap());


  
// Инициализация EEPROM
   EC_begin();  
   EC_read();
// Инициализация дисплея
   Serial.printf("Display TM1637 init\n");
   tm1637.init();
// Установка яркости дисплея  
   Serial.printf("Init TM1637 display\n");
   tm1637.set(7);
   tm1637.display(0,8);
   tm1637.display(1,2);
   tm1637.display(2,6);
   tm1637.display(3,6);
// Инициализация датчика DGT11   
   Serial.printf("Init DHT11on %d pin ...\n",PIN_DHT);
   dht.begin(); 
// Инициируем кнопку
   Serial.printf("Init button %d pin ...\n",PIN_BUTTON);
   pinMode(PIN_BUTTON, INPUT);     
// Инициируем выход управления 
   Serial.printf("Init relay cooler %d pin ...\n",PIN_COOLER);
   pinMode(PIN_COOLER, OUTPUT); 
   digitalWrite(PIN_COOLER, LOW); 
// Подключаемся к WiFi  
  WiFi_begin();
  delay(2000);
// Старт внутреннего WEB-сервера  
  HTTP_begin(); 
  char str[20];  
  
// Инициализация UDP клиента для NTP
  NTP_begin();  

     
}



void loop() {
   uint32_t ms = millis();
   t_cur       = ms/1000;
   uint16_t m  = 0;
   uint16_t h  = 0;
// Фиксируем нажатие кнопки 
   if( digitalRead(PIN_BUTTON) == LOW && press_button1 == false ){
       Serial.println("Press button");
       press_button1 = true;
       ms_button1 = ms;
   }
// Фиксируем отпускание кнопки 
   if( digitalRead(PIN_BUTTON) == HIGH && press_button1 == true && (ms-ms_button1)>200 ){
       press_button1 = false;
       uint16_t dt = ms-ms_button1;
       Serial.printf("Fixed press button %d ms",dt);
// Кортокое нажатие кнопки    
       flag_btn = true;  
 //      if( dt < timePress ){
   }

   if(ms < ms1 || (ms - ms1) > 500 ){
      ms1 = ms;
      tm    = t_cur + t_correct;
      switch( disp_mode ){
        case 0: 
// Отображение времени        
            if( timer == 0 ){
               m = ( tm/60 )%60;
               h = ( tm/3600 )%24;
               tm1637.clearDisplay();
               tm1637.point(points);
               tm1637.display(0,h/10);
               tm1637.display(1,h%10);
               tm1637.display(2,m/10);
               tm1637.display(3,m%10);
            }
// Отображение времени таймера
            else {            
               m = (timer/2)%60;
               h = timer/120;
               tm1637.clearDisplay();
               tm1637.point(true);
               tm1637.display(0,h/10);
               tm1637.display(1,h%10);
               tm1637.display(2,m/10);
               tm1637.display(3,m%10);              
            }
            break;
// Отображение влажности
        case 1: 
              tm1637.clearDisplay();
              tm1637.point(false);
              tm1637.display(0,Hum/10);
              tm1637.display(1,Hum%10);
              DisplaySpecialChar(3,0x24);
            break;
        default:
// Отображение температуры
            if( timer == 0 ){
               tm1637.clearDisplay();
               tm1637.point(false);
               tm1637.display(0,Temp/10);
               tm1637.display(1,Temp%10);
               DisplaySpecialChar(2,0x63);
               tm1637.display(3,0x0C);
            }
// Отображение времени таймера
            else {            
               m = (timer/2)%60;
               h = timer/120;
               tm1637.clearDisplay();
               tm1637.point(true);
               tm1637.display(0,h/10);
               tm1637.display(1,h%10);
               tm1637.display(2,m/10);
               tm1637.display(3,m%10);
              
            }
            break;
      }
      points = !points;

      
      Avalue = analogRead(A0);  
      if( Avalue > EC_Config.LIGHT_LIMIT )flag_light = false;
      else flag_light = true;
      int h    = (int)dht.readHumidity();
      int t   = (int)dht.readTemperature();
      if( h != ERROR_VALUE ){
         Hum  = h;
         Temp = t;
      }
      SetStatusFSM();
   }
   

// Каждые 600 секунд считываем время в интернете 
   if( !isAP && (ms < ms2 || (ms - ms2) > EC_Config.TIMEOUT_NTP || ms2 == 0 ) ){
       err_count++;
// Делаем три  попытки синхронизации с интернетом

       uint32_t t = GetNTP();
       if( t!=0 ){
          ms2       = ms;
          if( first_tm == 0 )first_tm = t;
          err_count = 0;
          t_correct = t - t_cur;
       }
       Serial.printf("Get NTP time. Error = %d\n",err_count);
   }
    
// Переключение режимов отображения
   if(ms < ms3 || (ms - ms3) > EC_Config.TIMEOUT_DISPLAY  ){
      ms3 = ms;
     
      switch( disp_mode ){
         case 0 : disp_mode = 1; break;
         case 1 : disp_mode = 2; break;
         default: disp_mode = 0;
      }
      
      Serial.printf("Analog = %d temp=%d hum=%d\n",Avalue,Temp,Hum);
      m = ( tm/60 )%60;
      h = ( tm/3600 )%24;
      Serial.printf("Time: %02d:%02d\n",h,m);   
   }

// Проверка датчика влажности   
   if(ms < ms4 || (ms - ms4) > EC_Config.TIMEOUT_CHANGE_HUM ){
      ms4 = ms;
      if( Hum!=0 && HumPrev!=0 && 
         ( Hum - HumPrev > EC_Config.HUM_DELTA  || 
           Hum > EC_Config.HUM_MAXIMUM ) )flag_hum = true;
      HumPrev = Hum;
   }

// Отправка на сервер
   if(ms < ms5 || (ms - ms5) > EC_Config.TIMEOUT_SEND1 || 
      (ms - ms5) > EC_Config.TIMEOUT_SEND2 && (flag_light || flag_cool) ){
      Serial.printf("%ld %ld %ld %ld %d %d\n",ms,ms5,EC_Config.TIMEOUT_SEND1,EC_Config.TIMEOUT_SEND2,
         (int)flag_light,(int)flag_cool);  
      ms5 = ms;
      if( SetParamHTTP() ){
         Count1 = 0;
         Count2 = 0;
      }

   }
   HTTP_loop();
   delay(100);

}


/**
 * Вывести спец-символ на дисплей
 */
void DisplaySpecialChar(uint8_t BitAddr,int8_t SpecChar)
{
  tm1637.start();          //start signal sent to TM1637 from MCU
  tm1637.writeByte(ADDR_FIXED);//

  tm1637.stop();           //
  tm1637.start();          //
  tm1637.writeByte(BitAddr|0xc0);//
  
  tm1637.writeByte(SpecChar);//
  tm1637.stop();            //
  tm1637.start();          //
  
  tm1637.writeByte(tm1637.Cmd_DispCtrl);//
  tm1637.stop();           //
}

/**
* Функция перехода автомата состояний
*/
void SetStatusFSM(){
   if( flag_light )Count2++;
//   Serial.printf("Mode=%d, Timer=%d, Light=%d, Button=%d, HumFlag=%d\n",
//       mode, timer, (int)flag_light,(int)flag_btn,(int)flag_hum);
   switch(mode){
// Режим ожидания     
      case tmWait :
         digitalWrite(PIN_COOLER, LOW);
         flag_cool = false;
// Нажата кнопка      
         if( flag_btn ){
            timer = EC_Config.TIMER_PERIOD*2;
            mode  = tmManualPower;
         }
// Если сработал датчик влажности
         else if( flag_hum ){
            timer = EC_Config.TIMER_PERIOD*2;
            mode  = tmNeedPower;
         }   
         break;
// Состояние ожилания работы вентилятора
      case tmNeedPower:
         digitalWrite(PIN_COOLER, LOW);
         flag_cool = false;
// Нажата кнопка      
         if( flag_btn ){
            mode  = tmManualPower;
         }
// Свет выключен
         else if( !flag_light ){
            mode  = tmAutoPower;
         }
         break;
// Состояние "Вентилятор работает в автомате"
      case tmAutoPower:
// Включить вентилятор
         digitalWrite(PIN_COOLER, HIGH);
         Count1++;
         flag_cool = true;
// Таймер считает
         if( timer > 0 )timer--;
// Нажата кнопка      
         if( flag_btn ){
            mode  = tmWait;
            timer = 0;
         }
// Включился чвет      
         else if( flag_light ){
            mode  = tmNeedPower;
         }
// Таймер сработал
         else if( timer <= 0 ){
            timer = 0;
            mode  = tmWait;
         }
         break;
// Состояние "Вентилятор работает в ручном режиме"
      case tmManualPower:
// Включить вентилятор
         digitalWrite(PIN_COOLER, HIGH);
         Count1++;
         flag_cool = true;
// Таймер считает
         if( timer > 0 )timer--;
// Нажата кнопка      
         if( flag_btn ){
            mode  = tmWait;
            timer = 0;
         }
// Таймер сработал
         else if( timer <= 0 ){
            timer = 0;
            mode  = tmWait;
         }
         break;
   } 
// Сбросить флани кнопка и влажность   
   flag_btn = false;
   flag_hum = false;

}



