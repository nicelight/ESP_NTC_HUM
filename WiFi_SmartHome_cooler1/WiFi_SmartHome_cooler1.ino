/*
  Контроллер включения выключения света
  16 января 2018 года
  Работает:
  1. Поднимается точка доступа для прописания настроек домашней wifi сети
  1.1 возможность указать статический ip адрес в wifi сети
  1.2 автоподключение к указанной wifi сети
  2. Поднимается веб сервер для мониторинга и настройки параметров
  2.1 Настройка максимально необходимой влажности
  2.2 Настройка, когда включать свет и сколько часов светить
  2.3 монитор инг текущей температуры, влажности, состояния света
  3. Синхронизация внутренних часов с серверами реального времени по NTP  ( необходимо  wifi соединение с интернетом)

  Допилить:

  преобразование освещенности в люксопопугаи

*/
#define REL_ON    0 // тип реакции реле на упр. сигнал
#define REL_OFF   1
//#define PIN_BUTTON  4
#define PIN_LIGHT   12 // включение света по таймеру
#define PIN_HUMER   13 // включение увлажнителя
#define PIN_DHT     14 // датчик влажности и температуры
// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define ERROR_VALUE 2147483647
int deltaTemper = 0; // на сколько градусов врет датчик DHT ( если на 1 грабус больше реальной t* показывает, то указываем 1. Если меньше то -1 )

#define DEBUG  // закомментировать для увеличения производительности esp

#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Wire.h>
#include <DHT.h>
#include <EEPROM.h>

#include "WC_EEPROM.h"
#include "WC_HTTP.h"
#include "WC_NTP.h"



DHT dht(PIN_DHT, DHTTYPE);


// Состояния автомата light
enum lightMode
{
  initGO,         //инициализация 
  checkRangeGO,  //проверка, включаться раньше чем выключаться или переход через полночь
  straightWorkerGO,  //Работа 
  invertedWorkerGO //Вентилятор работает в ручном режиме
};
lightMode light = initGO; // создаем автомат light c кейсами из списка enum

// состояния автомата hum
enum humMode 
{
  init1GO, // инициализация 
  checkMaxHumGO // проверка превышения максимального значения
};
humMode hum = init1GO;


uint16_t m    = 0;
uint16_t h    = 0;

uint32_t ms         = 0;
uint32_t ms1        = 0; // для опроса датчика DHT
uint32_t ms2        = 0; // для запроса реального времени к NTP серверу
uint32_t ms3        = 0; // обновление значений с датчиков, а так же часов:минут из секунд
uint32_t lightMs    = 0;
uint32_t humMs      = 0;
int Hum             = 0;
int HumPrev         = 0;
int Temp            = 0;
int Avalue          = 0;
int Count2          = 0;
uint32_t first_tm  = 0;
uint32_t tm        = 0;
uint32_t t_cur      = 0;
long  t_correct     = 0;
uint16_t err_count  = 0;
bool points         = true;
uint8_t disp_mode   = 0;

bool press_button1  = false;
uint32_t ms_button1 = 0;

// Флаг указывающий что яркость превышена для необходимости включения света ( это из исхода, не мое)
bool flag_light = false;
// флаг указывающий состояние света
bool light_state = false;
// Флаг нажатия кнопки
bool flag_btn   = false;
// Флаг срабатывания по влажности
bool hum_state   = false;
// Флаг работы вентилятора
bool flag_cool  = false;
// Количество часов работы таймера света
uint16_t timer   = 18;
// Вреия включения света, в который час
uint16_t timer_start = 8; // 8:00
// в который час отключать свет ( рассчитывается в программе)
uint16_t timer_stop    = 2; // 2:00
//флаг для перерасчета времени остановки света в случае если юзер ввел на фронте новые значения
bool new_timer_time_came = 0;




void setup() {
  // Последовательный порт для отладки
  Serial.begin(115200);
  Serial.printf("\n\nFree memory %d\n", ESP.getFreeHeap());



  // Инициализация EEPROM
  EC_begin();
  EC_read();
  //  Инициализация датчика DHT11
  Serial.printf("Init DHT11on %d pin ...\n", PIN_DHT);
  dht.begin();
  // Инициируем выход управления
  Serial.printf("Init humidity relay %d pin ...\n", PIN_HUMER);
  pinMode(PIN_HUMER, OUTPUT);
  digitalWrite(PIN_HUMER, REL_OFF);
  Serial.printf("Init light relay %d pin ...\n", PIN_LIGHT);
  pinMode(PIN_LIGHT, OUTPUT);
  digitalWrite(PIN_LIGHT, REL_OFF);

  // Подключаемся к WiFi
  WiFi_begin();
  delay(2000);
  // Старт внутреннего WEB-сервера
  HTTP_begin();
  char str[20];

  // Инициализация UDP клиента для NTP
  NTP_begin();
}//setup

void loop() {
  ms = millis();

  // корректировка времени раз в 0,5 секунды
  if (ms < ms1 || (ms - ms1) > 500l ) {
    ms1 = ms;
    t_cur = ms / 1000; //секунды взятые от millis()
    tm    = t_cur + t_correct; // реальное время(для юзера)  = millis()/1000 + поправка рассчитанная во время запроса с ntp
  }//if ms1 ( renew time)


  // Каждые 600 секунд считываем время в интернете, рассчитываем поправку t_cur
  if ( !isAP && (ms < ms2 || (ms - ms2) > EC_Config.TIMEOUT_NTP || ms2 == 0 ) ) {
    err_count++;
    // Делаем три  попытки синхронизации с интернетом
    uint32_t t = GetNTP();
    if ( t != 0 ) {
      ms2 = ms;
      if ( first_tm == 0 ) first_tm = t; // эту переменную исход использовал дял отправки на сервер
      err_count = 0;
      t_correct = t - t_cur; // рассчет поправки ко времени
    }// t!=0
    Serial.printf("Get NTP time. Error = %d\n", err_count);
  }// if (ms > 600) ==> get ntp


  // обновление значений с датчиков
  if ((ms - ms3) > 2000l) {
    ms3 = ms;
    Avalue = analogRead(A0);
    if ( Avalue > EC_Config.LIGHT_LIMIT )flag_light = false;
    else flag_light = true;
    int h     = (int)dht.readHumidity();
    int t     = (int)dht.readTemperature();
    if ( h != ERROR_VALUE ) {
      Hum  = h;
      Temp = t - deltaTemper; // поправка на датчик
    }//if not err reading DHT11
    else Serial.println("bad DHT11");
    // правильное конечное время
    m = ( tm / 60 ) % 60;
    h = ( tm / 3600 ) % 24;
#ifdef DEBUG
    Serial.printf("Time: %02d:%02d:%02d\n", h, m, tm % 60);
    Serial.printf("Analog = %d temp=%d hum=%d\n", Avalue, Temp, Hum);
    Serial.printf("Timer. Start = %d how much=%d stop=%d\n", timer_start, timer, timer_stop);
#endif
  }//if ms3

  HTTP_loop();
  //delay(50);
  //automatics();
  delay(100);
}//loop


/*   исходники из loop
  // Фиксируем нажатие кнопки
  if ( digitalRead(PIN_BUTTON) == LOW && press_button1 == false ) {
  Serial.println("Press button");
  press_button1 = true;
  ms_button1 = ms;
  }
  // Фиксируем отпускание кнопки
  if ( digitalRead(PIN_BUTTON) == HIGH && press_button1 == true && (ms - ms_button1) > 200 ) {
  press_button1 = false;
  uint16_t dt = ms - ms_button1;
  Serial.printf("Fixed press button %d ms", dt);
  // Кортокое нажатие кнопки
  flag_btn = true;
  //      if( dt < timePress ){
  }

      int h    = (int)dht.readHumidity();
      /int t   = (int)dht.readTemperature();
      if ( h != ERROR_VALUE ) {
      Hum  = h;
      Temp = t;
      }


  // Переключение режимов отображения
  if (ms < ms3 || (ms - ms3) > EC_Config.TIMEOUT_DISPLAY  ) {
    ms3 = ms;

    switch ( disp_mode ) {
      case 0 : disp_mode = 1; break;
      case 1 : disp_mode = 2; break;
      default: disp_mode = 0;
    }
    Serial.printf("Analog = %d temp=%d hum=%d\n", Avalue, Temp, Hum);
    m = ( tm/ 60 ) % 60;
    h = ( tm/ 3600 ) % 24;
    Serial.printf("Time: %02d:%02d\n", h, m);
  }

  // Проверка датчика влажности
  if ((ms - ms4) > EC_Config.TIMEOUT_CHANGE_HUM ) {
    ms4 = ms;
    if ( Hum != 0 && HumPrev != 0 &&
         ( Hum - HumPrev > EC_Config.HUM_DELTA  ||
           Hum > EC_Config.HUM_MAXIMUM ) )hum_state = true;
    HumPrev = Hum;
  }
  // Отправка на VPS сервер
  if (ms < ms5 || (ms - ms5) > EC_Config.TIMEOUT_SEND1 ||
      (ms - ms5) > EC_Config.TIMEOUT_SEND2 && (flag_light || flag_cool) ) {
    Serial.printf("%ld %ld %ld %ld %d %d\n", ms, ms5, EC_Config.TIMEOUT_SEND1, EC_Config.TIMEOUT_SEND2,
                  (int)flag_light, (int)flag_cool);
    ms5 = ms;
    if ( SetParamHTTP() ) {
  //        Count1 = 0;
      Count2 = 0;
    }

  }
  из loop */

/* Функция перехода автомата состояний
  //// из исходников

  void SetStatusFSM() {
  //  if ( flag_light )Count2++;
  //   Serial.printf("Mode=%d, Timer=%d, Light=%d, Button=%d, HumFlag=%d\n",
  //       mode, timer, (int)flag_light,(int)flag_btn,(int)hum_state);
  switch (mode) {
    // Режим ожидания
    case tmWait :
      digitalWrite(PIN_HUMER, LOW);
      flag_cool = false;
      // Нажата кнопка
      if ( flag_btn ) {
        timer = EC_Config.TIMER_PERIOD * 2;
        mode  = tmManualPower;
      }
      // Если сработал датчик влажности
      else if ( hum_state ) {
        timer = EC_Config.TIMER_PERIOD * 2;
        mode  = tmNeedPower;
      }
      break;
    // Состояние ожилания работы вентилятора
    case tmNeedPower:
      digitalWrite(PIN_HUMER, LOW);
      flag_cool = false;
      // Нажата кнопка
      if ( flag_btn ) {
        mode  = tmManualPower;
      }
      // Свет выключен
      else if ( !flag_light ) {
        mode  = tmAutoPower;
      }
      break;
    // Состояние "Вентилятор работает в автомате"
    case tmAutoPower:
      // Включить вентилятор
      digitalWrite(PIN_HUMER, HIGH);
      flag_cool = true;
      // Таймер считает
      if ( timer > 0 )timer--;
      // Нажата кнопка
      if ( flag_btn ) {
        mode  = tmWait;
        timer = 0;
      }
      // Включился чвет
      else if ( flag_light ) {
        mode  = tmNeedPower;
      }
      // Таймер сработал
      else if ( timer <= 0 ) {
        timer = 0;
        mode  = tmWait;
      }
      break;
    // Состояние "Вентилятор работает в ручном режиме"
    case tmManualPower:
      // Включить вентилятор
      digitalWrite(PIN_HUMER, HIGH);
      flag_cool = true;
      // Таймер считает
      if ( timer > 0 )timer--;
      // Нажата кнопка
      if ( flag_btn ) {
        mode  = tmWait;
        timer = 0;
      }
      // Таймер сработал
      else if ( timer <= 0 ) {
        timer = 0;
        mode  = tmWait;
      }
      break;
  }
  // Сбросить флани кнопка и влажность
  flag_btn = false;
  hum_state = false;

  }//

*/

