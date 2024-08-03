#ifndef __MAIN_H
#define __MAIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <freertos/event_groups.h>

extern QueueHandle_t sensorDataQueue;
extern EventGroupHandle_t xZigbeeEvents;
extern QueueHandle_t zigbeeReportDataQueue;

#endif