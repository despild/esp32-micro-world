// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include "Measurement.h"

struct AdcFrame { int32_t raw; uint32_t ms; bool fault, gap; };
static QueueHandle_t adcQueue;
static portMUX_TYPE adcMux=portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t adcDropped=0;
static bool adcTaskOk=false;

static void adcTask(void *) {
  pinMode(cfg::dout,INPUT_PULLUP);pinMode(cfg::sck,OUTPUT);digitalWrite(cfg::sck,LOW);
  bool primed=false,gap=false;
  uint32_t settle=0,last=millis();
  for(;;){
    const uint32_t now=uint32_t(esp_timer_get_time()/1000);
    if(gpio_get_level(gpio_num_t(cfg::dout))==0){
      uint32_t bits=0;
      // HX710B: 24 data clocks + 3 selection clocks = pressure, 40 SPS.
      // Keep every HIGH under 50 us; >60 us would enter power-down.
      portENTER_CRITICAL(&adcMux);
      for(unsigned i=0;i<27;i++){
        gpio_set_level(gpio_num_t(cfg::sck),1);delayMicroseconds(1);
        if(i<24)bits=(bits<<1)|gpio_get_level(gpio_num_t(cfg::dout));
        gpio_set_level(gpio_num_t(cfg::sck),0);delayMicroseconds(1);
      }
      const bool stuck=gpio_get_level(gpio_num_t(cfg::dout))==0;
      portEXIT_CRITICAL(&adcMux);
      if(stuck){
        if(uint32_t(now-last)>=500){AdcFrame f{0,now,true,gap};if(xQueueSend(adcQueue,&f,0)!=pdTRUE)++adcDropped;last=now;}
        primed=false;
        vTaskDelay(pdMS_TO_TICKS(20));continue;
      }
      last=now;
      if(!primed){primed=true;settle=now;}
      else if(uint32_t(now-settle)>=100){
        AdcFrame f{eq::signed24(bits),now,false,gap};
        if(xQueueSend(adcQueue,&f,0)!=pdTRUE){++adcDropped;gap=true;}else gap=false;
      }
    }else if(uint32_t(now-last)>=500){
      AdcFrame f{0,now,true,gap};if(xQueueSend(adcQueue,&f,0)!=pdTRUE)++adcDropped;
      last=now;primed=false;
    }
    vTaskDelay(1);
  }
}
static void adcBegin(){
  adcQueue=xQueueCreate(128,sizeof(AdcFrame));
  if(adcQueue)adcTaskOk=xTaskCreate(adcTask,"hx710b",3072,nullptr,2,nullptr)==pdPASS;
}
