#ifndef SENSOR_H
#define SENSOR_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        float temperature;
        float humidity;
        float pressure;
    } sensor_data_t;

    void bme280_sensor_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
#endif