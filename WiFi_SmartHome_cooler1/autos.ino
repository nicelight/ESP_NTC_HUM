
void automatics(void) {
  switch (light) {
    case initGO:
      lightMs = ms;
      light = checkRangeGO;
      break;
    // проверим, если значение включения света ниже, чем значение выключения, то отправимся в автоматы straight
    // иначе отправляемся в автоматы inverted
    // если время работы таймера равно нулю, остаемся здесь же
    case checkRangeGO:
      if (timer_start < timer_stop) light = straightWorkerGO;
      else if (timer_start > timer_stop) light = invertedWorkerGO;
      break;
    // работа когда вкл в 10 часов, допустим, а выкл в 15 часов
    case straightWorkerGO:
      if ((h >= timer_start) && ( h < timer_stop))   {
        digitalWrite(PIN_LIGHT, REL_ON);
        light_state = 1;// флаг  свет включен
      } else {
        digitalWrite(PIN_LIGHT, REL_OFF);
        light_state = 0;
      }
      light = checkRangeGO; // повторно проверяем если вдруг значения таймера изменились
      break;
    // работа когда вкл в 15 часов, допустим, а выкл в 3 часа утра
    case invertedWorkerGO:
      if ((h >= timer_stop) && ( h < timer_start))   {
        digitalWrite(PIN_LIGHT, REL_OFF);
        light_state = 0;// флаг  свет выключен
      } else {
        digitalWrite(PIN_LIGHT, REL_ON);
        light_state = 1;
      }
      light = checkRangeGO; // повторно проверяем если вдруг значения таймера изменились
      break;
  }//switch light

  switch (hum) {
    case init1GO:
      humMs = ms;
      hum = checkMaxHumGO;
      break;
    case checkMaxHumGO:
      if ((ms - humMs) >= EC_Config.TIMEOUT_CHANGE_HUM) {
        humMs = ms;
//        if ( Hum != 0 && HumPrev != 0 && Hum <= EC_Config.HUM_MAXIMUM ) {
        if ( Hum != 0 && Hum <= EC_Config.HUM_MAXIMUM ) {
          digitalWrite(PIN_HUMER, REL_ON);
          hum_state = false;
        }
        else {
          digitalWrite(PIN_HUMER, REL_OFF);
          hum_state = true;
        }
        HumPrev = Hum;
      }//if ms
      break;
  }//switch(hum){
}//automatics()
