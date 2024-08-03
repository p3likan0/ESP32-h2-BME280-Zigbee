#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <freertos/queue.h>
#include "sensor.h"
#include "zigbee.h"
#include "i2cdev.h"
#include "esp_zigbee_core.h"
#include <freertos/event_groups.h>

static const char *TAG = "Main";

QueueHandle_t sensorDataQueue;
QueueHandle_t zigbeeReportDataQueue;
EventGroupHandle_t xZigbeeEvents;



void main_task(void *pvParameters)
{
    bool zigbee_init_success = false;
    EventBits_t uxBits;
    uxBits = xEventGroupWaitBits(
        xZigbeeEvents,
        ZIGBEE_INIT_SUCCESS | ZIGBEE_INIT_FAILED,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(10 * 1000));

    if (!(uxBits & ZIGBEE_INIT_SUCCESS) && !(uxBits & ZIGBEE_INIT_FAILED))
    {
        printf("Zigbee init timeout");
        vTaskDelete(NULL);
    }
    else if (uxBits & ZIGBEE_INIT_SUCCESS)
    {
        printf("Zigbee init success");
        zigbee_init_success = true;
    }

    while (1)
    {
        sensor_data_t sensor_data;
        zigbee_report_data_t zigbee_report_data;
        xQueueReset(sensorDataQueue);
 
        


        if (xQueueReceive(sensorDataQueue, &sensor_data, pdMS_TO_TICKS(5000)) == pdPASS)
        {
            ESP_LOGI(TAG, "Hum: %.2f Tmp: %.2f Pre: %.2f", sensor_data.humidity, sensor_data.temperature, sensor_data.pressure);
          
            zigbee_report_data.temperature = sensor_data.temperature;
            zigbee_report_data.humidity = sensor_data.humidity;
            zigbee_report_data.pressure = sensor_data.pressure;
        } else
            ESP_LOGE(TAG, "No data from sensor");

        if (zigbee_init_success)
            xQueueSend(zigbeeReportDataQueue, &zigbee_report_data, 0);
        vTaskDelay(10 * 1000 / portTICK_PERIOD_MS);
    }
}


void app_main(void)
{
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    /* load Zigbee platform config to initialization */
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    /* hardware related and device init */

    xZigbeeEvents = xEventGroupCreate();
    sensorDataQueue = xQueueCreate(3, sizeof(sensor_data_t));
    zigbeeReportDataQueue = xQueueCreate(3, sizeof(zigbee_report_data_t));

    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreate(bme280_sensor_task, "bme280_sensor_task", 4096, NULL, 8, NULL);
    xTaskCreate(main_task, "Main_Task", 4096, NULL, 7, NULL);
    xTaskCreate(zigbee_task, "Zigbee_Task", 4096, NULL, 6, NULL);
    //xTaskCreate(zigbee_report_task, "Zigbee_ReportTask", 4096, NULL, 5, NULL);

    assert(sensorDataQueue != NULL);
    assert(xZigbeeEvents != NULL);
    assert(zigbeeReportDataQueue != NULL);
}
