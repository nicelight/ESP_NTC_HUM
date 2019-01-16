/**
  Контроллер управления вытяжным вентилятором. Версия 2.0 WiFi
  Copyright (C) 2016 Алексей Шихарбеев
  http://samopal.pro
*/

#include "WC_HTTP.h"
#include "WC_EEPROM.h"
#include "WC_NTP.h"

ESP8266WebServer server(80);
bool isAP = false;
String authPass = "";

/**
   Старт WiFi
*/
void WiFi_begin(void) {
  // Подключаемся к WiFi
  isAP = false;
  if ( ! ConnectWiFi()  ) {
    Serial.printf("Start AP %s\n", EC_Config.ESP_NAME);
    WiFi.mode(WIFI_STA);
    WiFi.softAP(EC_Config.ESP_NAME);
    isAP = true;
    Serial.printf("Open http://192.168.4.1 in your browser\n");
  }
  else {
    // Получаем статический IP если нужно
    if ( EC_Config.IP != 0 ) {
      WiFi.config(EC_Config.IP, EC_Config.GW, EC_Config.MASK);
      Serial.print("Open http://");
      Serial.print(WiFi.localIP());
      Serial.println(" in your browser");
    }
  }
  // Запускаем MDNS
  MDNS.begin(EC_Config.ESP_NAME);
  Serial.printf("Or by name: http://%s.local\n", EC_Config.ESP_NAME);



}

/**
   Соединение с WiFi
*/
bool ConnectWiFi(void) {

  // Если WiFi не сконфигурирован
  if ( strcmp(EC_Config.AP_SSID, "none") == 0 ) {
    Serial.printf("WiFi is not config ...\n");
    return false;
  }

  WiFi.mode(WIFI_STA);

  // Пытаемся соединиться с точкой доступа
  Serial.printf("\nConnecting to: %s/%s\n", EC_Config.AP_SSID, EC_Config.AP_PASS);
  WiFi.begin(EC_Config.AP_SSID, EC_Config.AP_PASS);
  delay(1000);

  // Максиммум N раз проверка соединения (12 секунд)
  for ( int j = 0; j < 15; j++ ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nWiFi connect: ");
      Serial.print(WiFi.localIP());
      Serial.print("/");
      Serial.print(WiFi.subnetMask());
      Serial.print("/");
      Serial.println(WiFi.gatewayIP());
      return true;
    }
    delay(1000);
    Serial.print(WiFi.status());
  }
  Serial.printf("\nConnect WiFi failed ...\n");
  return false;
}

/**
   Старт WEB сервера
*/
void HTTP_begin(void) {
  // Поднимаем WEB-сервер
  server.on ( "/", HTTP_handleRoot );
  server.on ( "/config", HTTP_handleConfig );
  server.on ( "/configAsu", HTTP_handleConfigASU );
  server.on ( "/save", HTTP_handleSave );
  server.on ( "/reboot", HTTP_handleReboot );
  server.on ( "/default", HTTP_handleDefault );
  server.on ( "/check_lamp", HTTP_handleCheckLamp);
  server.on ( "/check_hum", HTTP_handleCheckLamp);
  //server.on ( "/check_hum", HTTP_handleCheckHumer);
  server.onNotFound ( HTTP_handleRoot );
  server.begin();
  Serial.printf( "HTTP server started ...\n" );

}

/**
   Обработчик событий WEB-сервера
*/
void HTTP_loop(void) {
  server.handleClient();
}

/*
   Оработчик главной страницы сервера
*/
void HTTP_handleRoot(void) {
  String out = "";

  out =
    "<html>\
  <head>\
    <meta charset=\"utf-8\" />\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #c8e8f8; font-family: Georgia, Times, serif; Color: #206080; }\
    </style>\
  </head>\
  <body>\
   <h1>WiFi контроллер</h1>\n";
  out += "<h2>";
  out += EC_Config.ESP_NAME;
  out += "</h2>\n";

  if ( isAP ) {
    out += "<p>Режим точки доступа: ";
    out += EC_Config.ESP_NAME;
    out += " </p>";
  }
  else {
    out += "<p>Подключено к ";
    out += EC_Config.AP_SSID;
    out += " </p>";
  }
  char str[50];
  /*
     Здесь выводим что-нибудь свое

  */
  out += "<h2>";
  sprintf(str, "Время: %02d:%02d</br>", ( tm / 3600 ) % 24, ( tm / 60 ) % 60);
  out += str;
  sprintf(str, "T=%02d H=%d Analog=%d<br>", Temp, Hum, Avalue);
  out += str;

  sprintf(str, "Включение света : %02d:00</br>", timer_start);
  out += str;
  sprintf(str, "Свет проработает в часах: %02d</br>", timer);
  out += str;
  sprintf(str, "Потухнуть в: %02d:00</br>", timer_stop);
  out += str;


  out += "сейчас свет: ";
  if ( light_state )out += "ON";
  else out += "OFF";
  out += "</h2>\n";
  out += "<br><br>";
  out += "<a href='/check_lamp'>Проверить свет</a>\n";
  out += "<br><br>";
  out += "<a href='/check_hum'>Проверить увлажнитель</a>\n";
  out += "<br><br><br><br><br>";





  out += "\
     <p><a href=\"/config\">Настройка параметров сети</a></p>\
     <p><a href=\"/configAsu\">Настройка автоматизации</a></p>\
  </body>\
</html>";
  server.send ( 200, "text/html", out );
}



/*
   Оработчик страницы настройки параметров WIFI сети
*/
void HTTP_handleConfig(void) {
  String out = "";
  char str[10];
  out =
    "<html>\
  <head>\
    <meta charset=\"utf-8\" />\
    <title>ESP8266 SmartHome Controller Config</title>\
    <style>\
      body { background-color: #c8e8f8; font-family: Georgia, Times, serif; Color: #206080; }\
    </style>\
  </head>\
  <body>\
    <h1>Настройка параметров сети</h1>\
     <ul>\
     <p><a href=\"/\">Главная</a></p>\
     <p><a href=\"/configAsu\">Настройка автоматизации</a></p>\
     <p><a href=\"/reboot\">Перезагрузка</a></p>\
     <p><a href=\"/default\">Сброс настроек</a></p>\
     </ul>\n";

  out +=
    "     <form method='get' action='/save'>\
         <p><b>Параметры ESP модуля</b></p>\
         <table border=0><tr><td>Наименование:</td><td><input name='esp_name' length=32 value='";
  out += EC_Config.ESP_NAME;
  out += "'></td></tr>\
         <tr><td>Пароль admin:</td><td><input name='esp_pass' length=32 value='";
  out += EC_Config.ESP_PASS;
  out += "'></td></tr></table><br>\
         <p><b>Параметры WiFi подключения</b></p>\
         <table border=0><tr><td>Имя сети: </td><td><input name='ap_ssid' length=32 value='";
  out += EC_Config.AP_SSID;
  out += "'></td></tr>\
         <tr><td>Пароль:</td><td><input name='ap_pass' length=32 value='";
  out += EC_Config.AP_PASS;
  out += "'></td></tr>\
        <tr><td>IP-адрес:</td><td><input name='ip1' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.IP[0]);
  out += str;
  out += "'>.<input name='ip2' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.IP[1]);
  out += str;
  out += "'>.<input name='ip3' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.IP[2]);
  out += str;
  out += "'>.<input name='ip4' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.IP[3]);
  out += str;
  out += "'></td></tr>\
        <tr><td>IP-маска:</td><td><input name='mask1' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.MASK[0]);
  out += str;
  out += "'>.<input name='mask2' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.MASK[1]);
  out += str;
  out += "'>.<input name='mask3' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.MASK[2]);
  out += str;
  out += "'>.<input name='mask4' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.MASK[3]);
  out += str;
  out += "'></td></tr>\
        <tr><td>IP-шлюз:</td><td><input name='gw1' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.GW[0]);
  out += str;
  out += "'>.<input name='gw2' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.GW[1]);
  out += str;
  out += "'>.<input name='gw3' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.GW[2]);
  out += str;
  out += "'>.<input name='gw4' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.GW[3]);
  out += str;
  out += "'></td></tr></table>\n\
         <p><b>Параметры сервера времени</b></p>\
         <table border=0><tr><td>NTP сервер 1:</td><td><input name='ntp_server1' length=32 value='";
  out += EC_Config.NTP_SERVER1;
  out += "'></td></tr>\
         <tr><td>NTP сервер 2:</td><td><input name='ntp_server2' length=32 value='";
  out += EC_Config.NTP_SERVER2;
  out += "'></td></tr>\
         <tr><td>NTP сервер 3:</td><td><input name='ntp_server3' length=32 value='";
  out += EC_Config.NTP_SERVER3;
  out += "'></td></tr>\
         <tr><td>Таймзона:</td><td><input name='tz' length=4 size 4 value='";
  sprintf(str, "%d", EC_Config.TZ);
  out += str;
  out += "'></td></tr>\
         <tr><td>Интервал опроса NTP, сек:</td><td><input name='tm_ntp' length=32 value='";
  sprintf(str, "%d", EC_Config.TIMEOUT_NTP / 1000);
  out += str;
  out += "'></td></tr></table>\
     <input type='submit' value='Сохранить'>\
     </form>\
  </body>\
</html>";
  server.send ( 200, "text/html", out );
}

/*
   Оработчик страницы настройки АСУ контроллера
*/
void HTTP_handleConfigASU(void) {
  String out = "";
  char str[10];
  out =
    "<html>\
  <head>\
    <meta charset=\"utf-8\" />\
    <title>ESP8266 SmartHome Controller Adv Config</title>\
    <style>\
      body { background-color: #c8e8f8; font-family: Georgia, Times, serif; Color: #206080; }\
    </style>\
  </head>\
  <body>\
    <h1>Настройка контроллера вентилятора</h1>\
     <ul>\
     <p><a href=\"/\">Главная</a></p>\
     <p><a href=\"/config\">Настройка параметров сети</a></p>\
     <p><a href=\"/reboot\">Перезагрузка</a></p>\
     <p><a href=\"/default\">Сброс настроек</a></p>\
     </ul>";

  out +=
    "     <form method='get' action='/save'>\
         <p><b>Параметры работы контроллера</b></p>\
         <table border=0><tr><td>Значение АЦП включения света:</td><td><input name='light_limit' length=32 value='";
  sprintf(str, "%d", EC_Config.LIGHT_LIMIT);
  out += str;
  out += "'></td></tr>\
         <tr><td>Интервал проверки влажности, сек:</td><td><input name='tm_change_hum' length=32 value='";
  sprintf(str, "%d", EC_Config.TIMEOUT_CHANGE_HUM / 1000);
  out += str;
  out += "'></td></tr>\
         <tr><td>Интервал смены дисплея, сек:</td><td><input name='tm_display' length=32 value='";
  sprintf(str, "%d", EC_Config.TIMEOUT_DISPLAY / 1000);
  out += str;


  out += "'></td></tr>\
         <tr><td>Вклюить свет во сколько часов (без минут) :</td><td><input name='timer_start' length=32 value='";
  sprintf(str, "%d", EC_Config.TIMER_START_TIME);
  out += str;
  out += "'></td></tr>\
         <tr><td>Время работы освещения, часов:</td><td><input name='timer' length=32 value='";
  sprintf(str, "%d", EC_Config.TIMER_PERIOD);
  out += str;
  out += "'></td></tr>\
         <tr><td>Дельта изменения влажности, %:</td><td><input name='hum_delta' length=32 value='";
  sprintf(str, "%d", EC_Config.HUM_DELTA);
  out += str;
  out += "'></td></tr>\
         <tr><td>Максимальное значение влажности, %:</td><td><input name='hum_max' length=32 value='";
  sprintf(str, "%d", EC_Config.HUM_MAXIMUM);
  out += str;
  /*
    out += "'></td></tr></table>\
           <p><b>Параметры сохранения на сервер</b></p>\
           <table border=0><tr><td>Интервал отправки 1:</td><td><input name='tm_send1' length=32 value='";
    sprintf(str,"%d",EC_Config.TIMEOUT_SEND1/1000);
    out += str;
    out += "'></td></tr>\
           <tr><td>Интервал отправки 2:</td><td><input name='tm_send2' length=32 value='";
    sprintf(str,"%d",EC_Config.TIMEOUT_SEND2/1000);
    out += str;
    out += "'></td></tr>\
           <tr><td>URL сеовера:</td><td><input name='http_serv' length=64 size=64 value='";
    out += EC_Config.HTTP_SERVER;
    //  out += "'></td></tr>\
    //        <tr><td>URL сеовера:</td><td><input name='http_req' length=128 size=64 value='";
    //  out += EC_Config.HTTP_REQUEST;
  */
  out += "'></td></tr></table>\
     <input type='submit' value='Сохранить'>\
     </form>\
  </body>\
</html>";
  server.send ( 200, "text/html", out );
}


/*
   Оработчик сохранения в базу
*/
void HTTP_handleSave(void) {

  if ( server.hasArg("esp_name")     )strcpy(EC_Config.ESP_NAME, server.arg("esp_name").c_str());
  if ( server.hasArg("esp_pass")     )strcpy(EC_Config.ESP_PASS, server.arg("esp_pass").c_str());
  if ( server.hasArg("ap_ssid")      )strcpy(EC_Config.AP_SSID, server.arg("ap_ssid").c_str());
  if ( server.hasArg("ap_pass")      )strcpy(EC_Config.AP_PASS, server.arg("ap_pass").c_str());
  if ( server.hasArg("ip1")          )EC_Config.IP[0] = atoi(server.arg("ip1").c_str());
  if ( server.hasArg("ip2")          )EC_Config.IP[1] = atoi(server.arg("ip2").c_str());
  if ( server.hasArg("ip3")          )EC_Config.IP[2] = atoi(server.arg("ip3").c_str());
  if ( server.hasArg("ip4")          )EC_Config.IP[3] = atoi(server.arg("ip4").c_str());
  if ( server.hasArg("mask1")        )EC_Config.MASK[0] = atoi(server.arg("mask1").c_str());
  if ( server.hasArg("mask2")        )EC_Config.MASK[1] = atoi(server.arg("mask2").c_str());
  if ( server.hasArg("mask3")        )EC_Config.MASK[2] = atoi(server.arg("mask3").c_str());
  if ( server.hasArg("mask4")        )EC_Config.MASK[3] = atoi(server.arg("mask4").c_str());
  if ( server.hasArg("gw1")          )EC_Config.GW[0] = atoi(server.arg("gw1").c_str());
  if ( server.hasArg("gw2")          )EC_Config.GW[1] = atoi(server.arg("gw2").c_str());
  if ( server.hasArg("gw3")          )EC_Config.GW[2] = atoi(server.arg("gw3").c_str());
  if ( server.hasArg("gw4")          )EC_Config.GW[3] = atoi(server.arg("gw4").c_str());
  if ( server.hasArg("ntp_server1")  )strcpy(EC_Config.NTP_SERVER1, server.arg("ntp_server1").c_str());
  if ( server.hasArg("ntp_server2")  )strcpy(EC_Config.NTP_SERVER2, server.arg("ntp_server2").c_str());
  if ( server.hasArg("ntp_server3")  )strcpy(EC_Config.NTP_SERVER3, server.arg("ntp_server3").c_str());
  if ( server.hasArg("tz")           )EC_Config.TZ = atoi(server.arg("tz").c_str());
  if ( server.hasArg("tm_ntp")       )EC_Config.TIMEOUT_NTP  = atoi(server.arg("tm_ntp").c_str()) * 1000;
  if ( server.hasArg("light_limit")  )EC_Config.LIGHT_LIMIT  = atoi(server.arg("light_limit").c_str());
  if ( server.hasArg("tm_change_hum"))EC_Config.TIMEOUT_CHANGE_HUM  = atoi(server.arg("tm_change_hum").c_str()) * 1000;
  if ( server.hasArg("tm_display")   )EC_Config.TIMEOUT_DISPLAY  = atoi(server.arg("tm_display").c_str()) * 1000;

  if ( server.hasArg("timer_start")  ) {
    EC_Config.TIMER_START_TIME  = atoi(server.arg("timer_start").c_str());
    new_timer_time_came = 1; // флаг
  }//hasArg("timer_start")
  if ( server.hasArg("timer")        ) {
    EC_Config.TIMER_PERIOD  = atoi(server.arg("timer").c_str());
    new_timer_time_came = 1; //флаг
  }//hasArg("timer")
  // перерассчет  времени выключения света
  if (new_timer_time_came) {
    new_timer_time_came = 0;
    EC_Config.TIMER_STOP_TIME  = EC_Config.TIMER_START_TIME + EC_Config.TIMER_PERIOD;
    if (EC_Config.TIMER_STOP_TIME > 23) EC_Config.TIMER_STOP_TIME = EC_Config.TIMER_STOP_TIME - 24; // еслиесть переход через полночь
    timer = EC_Config.TIMER_PERIOD;
    timer_start = EC_Config.TIMER_START_TIME;
    timer_stop = EC_Config.TIMER_STOP_TIME;
    Serial.printf("\nTimer setup again. \nNew period %d hours", EC_Config.TIMER_PERIOD);
    Serial.printf("\nStart timer at %d : 00", EC_Config.TIMER_START_TIME);
    Serial.printf("\nStop timer at %d : 00\n", EC_Config.TIMER_STOP_TIME);
  }//if need change

  if ( server.hasArg("hum_delta")    )EC_Config.HUM_DELTA  = atoi(server.arg("hum_delta").c_str());
  if ( server.hasArg("hum_max")      )EC_Config.HUM_MAXIMUM  = atoi(server.arg("hum_max").c_str());
  if ( server.hasArg("tm_send1")      )EC_Config.TIMEOUT_SEND1  = atoi(server.arg("tm_send1").c_str()) * 1000;
  if ( server.hasArg("tm_send2")      )EC_Config.TIMEOUT_SEND2  = atoi(server.arg("tm_send2").c_str()) * 1000;
  if ( server.hasArg("http_serv")  )strcpy(EC_Config.HTTP_SERVER, server.arg("http_serv").c_str());
  //  if( server.hasArg("http_req")  )strcpy(EC_Config.HTTP_REQUEST,server.arg("http_req").c_str());
  EC_save();

  String out = "";

  out =
    "<html>\
  <head>\
    <meta charset='utf-8' />\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #c8e8f8; font-family: Georgia, Times, serif; Color: #206080; }\
    </style>\
  </head>\
  <body>\
    <h1>WiFi контроллер</h1>\
     <h2>Конфигурация сохранена</h2>\
     <nav>\
     <ul>\
     <p><a href=\"/reboot\">Перезагрузка</a></p>\
     <p><a href=\"/config\">Настройка параметров сети</a></p>\
     <p><a href=\"/configAsu\">Настройка автоматизации</a></p>\
     <p><a href=\"/\">Главная</a></p>\
     </ul>\
     </nav>\
  </body>\
</html>";
  server.send ( 200, "text/html", out );

}

/*
   Сброс настрое по умолчанию
*/
void HTTP_handleDefault(void) {
  EC_default();
  HTTP_handleConfig();
}

/*
   Перезагрузка часов
*/
void HTTP_handleReboot(void) {

  String out = "";

  out =
    "<html>\
  <head>\
    <meta charset='utf-8' />\
    <meta http-equiv='refresh' content='30;URL=/'>\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #c8e8f8; font-family: Georgia, Times, serif; Color: #206080; }\
    </style>\
  </head>\
  <body>\
    <h1>Перезагрузка контроллера</h1>\
     <p><a href=\"/\">Через 30 сек будет переадресация на главную</a></p>\
   </body>\
</html>";
  server.send ( 200, "text/html", out );
  ESP.reset();

}

/*
   кнопка проверки лампы
*/
void HTTP_handleCheckLamp(void) {
  String out = "";

  out =
    "<html>\
  <head>\
    <meta charset='utf-8' />\
    <meta http-equiv='refresh' content='2;URL=/'>\
    <title>NiceGrover</title>\
    <style>\
      body { background-color: #c8e8f8; font-family: Georgia, Times, serif; Color: #206080; }\
    </style>\
  </head>\
  <body>\
    <h1>Статус</h1>\
    <h2>Свет поменял свое состояние на 2 секунды</h2>\
    <br><br>\
     <p><a href=\"/\">Вернуться на главную</a></p>\
   </body>\
</html>";
  server.send ( 200, "text/html", out );
  //digitalWrite(PIN_LIGHT, light_state);
  delay(2000);
}//HTTP_handleCheckLamp()

/*
   кнопка проверки увлажнителя

  void HTTP_handleCheckHumer(void) {
  String out = "";

  out =
    "<html>\
  <head>\
    <meta charset='utf-8' />\
    <meta http-equiv='refresh' content='2;URL=/'>\
    <title>NiceGrover</title>\
    <style>\
      body { background-color: #c8e8f8; font-family: Georgia, Times, serif; Color: #206080; }\
    </style>\
  </head>\
  <body>\
    <h1>Статус</h1>\
    <h2>Увлажнитель поменял свое состояние на 2 секунды</h2>\
    <br><br>\
     <p><a href=\"/\">Вернуться на главную</a></p>\
   </body>\
  </html>";
  server.send ( 200, "text/html", out );
  //digitalWrite(PIN_LIGHT, !hum_state);
  delay(2000);
  }//HTTP_handleCheckLamp()
*/


/**
  Сохранить параметры на HTTP сервере
*/
bool SetParamHTTP() {

  WiFiClient client;
  IPAddress ip1;
  WiFi.hostByName(EC_Config.HTTP_SERVER, ip1);
  Serial.print("IP=");
  Serial.println(ip1);

  String out = "";
  char str[256];
  if (!client.connect(ip1, 80)) {
    Serial.printf("Connection %s failed", EC_Config.HTTP_SERVER);
    return false;
  }
  out += "GET ";
  out += "http://";
  out += EC_Config.HTTP_SERVER;
  // Формируем строку запроса

  sprintf(str, "/save3.php?id=%s&h=%d&t=%d&a=%d&tm1=%d&tm2=%d&uptime=%ld", EC_Config.ESP_NAME, Hum, Temp, Avalue, Count1 / 2, Count2 / 2, (tm - first_tm));
  out += str;
  out += " HTTP/1.0\r\n\r\n";
  Serial.print(out);
  client.print(out);
  delay(100);
  return true;


}

/**
   Конвертирование time_t в строку
*/
/*
  void Time2Str(char *s,time_t t){
  sprintf(s,"%02d.%02d.%02d %02d:%02d:%02d",
      day(t),
      month(t),
      year(t),
      hour(t),
      minute(t),
      second(t));
  }
*/

// преобразование переменной float x в строку
//float x= 12.728;
//dtostrf(x, 2, 3, myStr3);  // выводим в строку myStr3 2 разряда до, 3 разряда после запятой

