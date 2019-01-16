/**
* Контроллер управления вытяжным вентилятором. Версия 2.0 WiFi
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/

#ifndef WC_EEPROM_h
#define WC_EEPROM_h

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <EEPROM.h>

extern struct WC_Config EC_Config;
extern int Hum;
extern int Temp;
extern int Avalue;
extern uint32_t tm;
extern bool flag_light;
extern bool light_state;
extern bool hum_state;
extern bool flag_btn;
extern uint16_t timer;
extern uint16_t timer_start;
extern uint16_t timer_stop;
extern bool new_timer_time_came;
struct WC_Config{
// Наименование в режиме точки доступа  
   char ESP_NAME[32];
   char ESP_PASS[32];
// Параметры подключения в режиме клиента
   char AP_SSID[32];
   char AP_PASS[32];
   IPAddress IP;
   IPAddress MASK;
   IPAddress GW;
// Параметры NTP сервера
   char NTP_SERVER1[32];
   char NTP_SERVER2[32];
   char NTP_SERVER3[32];
   short int  TZ;
// Параметры работы вентилятора 
// Значение АЦП для срабатывания состояния света
   uint16_t LIGHT_LIMIT;
// Интервал проверки изменения влажности, мс
   uint32_t TIMEOUT_CHANGE_HUM;
// Интервал смены показа дисплея, мс
   uint32_t TIMEOUT_DISPLAY;
// Интервал опроса NTP
   uint32_t TIMEOUT_NTP;
// Период работы таймера, часов (1, 24)
   uint16_t TIMER_PERIOD;
// во сколько часов запускать свет (0, 23)
   uint16_t TIMER_START_TIME;
   // во сколько часов тушить свет (0, 23)
   uint16_t TIMER_STOP_TIME;
// Изменение владности, при котором запускается вентилятор
   uint16_t HUM_DELTA;
// Абслолютное зачение влажности при которм запускается таймер
   uint16_t HUM_MAXIMUM;   
// Интервал отправки сообщений на сервер, мс
   uint32_t TIMEOUT_SEND1;
// Интервал отправки сообщений на сервер при включенном свете или вентиляторе, мс
   uint32_t TIMEOUT_SEND2;
// Сервер куда отправлять статистику
   char     HTTP_SERVER[48];   
// Строка отправки параметров на сервер
//   char     HTTP_REQUEST[128];   
// Контрольная сумма   
   uint16_t SRC;   
};


void     EC_begin(void);
void     EC_read(void);
void     EC_save(void);
uint16_t EC_SRC(void);
void     EC_default(void);




#endif
