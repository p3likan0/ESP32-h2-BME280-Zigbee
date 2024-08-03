#include "main.h"
#include "sensor.h"
#include "bmp280.h"
#include "esp_log.h"


static const char *TAG = "Sensor";


void bme280_sensor_task(void *pvParameters)
{
    bmp280_params_t params;
    bmp280_init_default_params(&params);
    bmp280_t dev = { 0 };

    ESP_ERROR_CHECK(bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, 0, 2, 1));
    ESP_ERROR_CHECK(bmp280_init(&dev, &params));

    static sensor_data_t data = {0};

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));


        if (bmp280_read_float(&dev, &data.temperature, &data.pressure, &data.humidity) != ESP_OK)
        {
            printf("Temperature/pressure reading failed\n");
            continue;
        }
        ESP_LOGI(TAG, "Hum: %.2f Tmp: %.2f Pre: %.2f", data.humidity, data.temperature, data.pressure);

        // if (xQueueSend(sensorDataQueue, &data, portMAX_DELAY) != pdPASS)
        //     assert(0);
    }
}