/**
* Контроллер освещения и влажности Версия 2.0 WiFi
*/

#include "WC_EEPROM.h"

struct WC_Config EC_Config;

/**
 * Инициализация EEPROM
 */
void EC_begin(void){
   size_t sz1 = sizeof(EC_Config);
   EEPROM.begin(sz1);   
   Serial.printf("EEPROM init. Size = %d\n",(int)sz1);

}

/**
 * Читаем конфигурацию из EEPROM в память
 */
void EC_read(void){
   size_t sz1 = sizeof(EC_Config);
   for( int i=0; i<sz1; i++ ){
       uint8_t c = EEPROM.read(i);
       *((uint8_t*)&EC_Config + i) = c; 
    }  
    uint16_t src = EC_SRC();
    if( EC_Config.SRC == src ){
       Serial.printf("EEPROM Config is correct\n");
    }
    else {
       Serial.printf("EEPROM SRC is not valid: %d %d\n",src,EC_Config.SRC);
       EC_default();
       EC_save();
    }        
}

/**
 * Устанавливаем значения конфигурации по-умолчанию
 */
void EC_default(void){
   size_t sz1 = sizeof(EC_Config);
   memset( &EC_Config, '\0',sz1);
//   for( int i=0, byte *p = (byte *)&EC_Config; i<sz1; i++, p++) 
//       *p = 0;   
     
   strcpy(EC_Config.ESP_NAME,"Nicelight_hydro_01");
   strcpy(EC_Config.ESP_PASS,"admin");
   strcpy(EC_Config.AP_SSID, "none");
   strcpy(EC_Config.AP_PASS, "1234567890=");
   EC_Config.IP[0]      = 192;   
   EC_Config.IP[1]      = 168;   
   EC_Config.IP[2]      = 1;     
   EC_Config.IP[3]      = 4;
   EC_Config.MASK[0]    = 255; 
   EC_Config.MASK[1]    = 255; 
   EC_Config.MASK[2]    = 255; 
   EC_Config.MASK[3]    = 0;
   EC_Config.GW[0]      = 192;   
   EC_Config.GW[1]      = 168;   
   EC_Config.GW[2]      = 1;     
   EC_Config.GW[3]      = 1;
   strcpy(EC_Config.NTP_SERVER1, "0.ru.pool.ntp.org");
   strcpy(EC_Config.NTP_SERVER2, "1.ru.pool.ntp.org");
   strcpy(EC_Config.NTP_SERVER3, "2.ru.pool.ntp.org");
   EC_Config.TZ         = 5;
   EC_Config.LIGHT_LIMIT          = 900;
   EC_Config.TIMEOUT_CHANGE_HUM   = 60000;
   EC_Config.TIMEOUT_DISPLAY      = 5000;
   EC_Config.TIMEOUT_NTP          = 600000;
   
   EC_Config.TIMER_PERIOD         = 18; // время работы освещенности в часах
   EC_Config.TIMER_START_TIME         = 8; // время старта включения света ( в 8 утра включаем)
   EC_Config.TIMER_STOP_TIME         = 2; // время выключения света ( в 2 ночи тушим) 
   
   EC_Config.HUM_DELTA            = 3;
   EC_Config.HUM_MAXIMUM          = 85;  
   EC_Config.TIMEOUT_SEND1        = 900000; 
   EC_Config.TIMEOUT_SEND2        = 60000; 
   strcpy(EC_Config.HTTP_SERVER,"service.samopal.pro");
//   strcpy(EC_Config.HTTP_REQUEST,"/save3?id=%s&h=%d&t=%d&a=%d&tm1=%d&tm2=%d&uptime=%ld");  
}

/**
 * Сохраняем значение конфигурации в EEPROM
 */
void EC_save(void){
   EC_Config.SRC = EC_SRC();
   size_t sz1 = sizeof(EC_Config);
   for( int i=0; i<sz1; i++)
      EEPROM.write(i, *((uint8_t*)&EC_Config + i));
   EEPROM.commit();     
   Serial.printf("Save Config to EEPROM. SCR=%d\n",EC_Config.SRC);   
}

/**
 * Сохраняем значение конфигурации в EEPROM
 */
uint16_t EC_SRC(void){
   uint16_t src = 0;
   size_t sz1 = sizeof(EC_Config);
   uint16_t src_save = EC_Config.SRC;
   EC_Config.SRC = 0;
   for( int i=0; i<sz1; i++)src +=*((uint8_t*)&EC_Config + i);
   Serial.printf("SCR=%d\n",src); 
   EC_Config.SRC = src_save;
   return src;  
}

