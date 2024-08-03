#include "nvs_flash.h"
#include "esp_check.h"
#include "esp_log.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "esp_zigbee_core.h"
#include "driver/gpio.h"
#include "string.h"

#include <zigbee.h>
#include <main.h>

static const char *TAG = "Zigbee";

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message);
static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message);

/* initialize Zigbee stack with Zigbee end-device config */
void zigbee_task(void *pvParameters)
{
    xEventGroupSetBits(xZigbeeEvents, ZIGBEE_INIT_IN_PROGRESS);

    /* initialize Zigbee stack with Zigbee end-device config */
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    /* Enable zigbee light sleep */
    esp_zb_sleep_enable(true);
    esp_zb_init(&zb_nwk_cfg);

    // ------------------------------ Cluster BASIC ------------------------------
    esp_zb_basic_cluster_cfg_t basic_cluster_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 0x03,
    };
    uint32_t ApplicationVersion = 0x0001;
    uint32_t StackVersion = 0x0002;
    uint32_t HWVersion = 0x0002;
    uint8_t ManufacturerName[] = {9, 'E', 's', 'p', 'r', 'e', 's', 's', 'i', 'f'}; // warning: this is in format {length, 'string'} :
    uint8_t ModelIdentifier[] = {8, 'E', 'S', 'P', '3', '2', '-', 'H', '2'};
    uint8_t DateCode[] = {8, '2', '0', '2', '4', '0', '3', '1', '6'};
    esp_zb_attribute_list_t *esp_zb_basic_cluster = esp_zb_basic_cluster_create(&basic_cluster_cfg);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_APPLICATION_VERSION_ID, &ApplicationVersion);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_STACK_VERSION_ID, &StackVersion);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_HW_VERSION_ID, &HWVersion);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, ManufacturerName);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, ModelIdentifier);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_DATE_CODE_ID, DateCode);

    // ------------------------------ Cluster IDENTIFY ------------------------------
    esp_zb_identify_cluster_cfg_t identify_cluster_cfg = {
        .identify_time = 0,
    };
    esp_zb_attribute_list_t *esp_zb_identify_cluster = esp_zb_identify_cluster_create(&identify_cluster_cfg);

    // ------------------------------ Cluster Temperature ------------------------------
    esp_zb_temperature_meas_cluster_cfg_t temperature_meas_cfg = {
        .measured_value = 0x8000,
        .min_value = -50,
        .max_value = 150,
    };
    //uint16_t temperature_tolerance = 1;
    esp_zb_attribute_list_t *esp_zb_temperature_meas_cluster = esp_zb_temperature_meas_cluster_create(&temperature_meas_cfg);
    //ESP_ERROR_CHECK(esp_zb_temperature_meas_cluster_add_attr(esp_zb_temperature_meas_cluster, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_ID, &temperature_tolerance));

    // ------------------------------ Cluster Humidity ------------------------------
    esp_zb_humidity_meas_cluster_cfg_t humidity_meas_cfg = {
        .measured_value = 0xFFFF,
        .min_value = 0,
        .max_value = 10000,
    };
    //uint16_t humidity_tolerance = 1;
    esp_zb_attribute_list_t *esp_zb_humidity_meas_cluster = esp_zb_humidity_meas_cluster_create(&humidity_meas_cfg);
    //ESP_ERROR_CHECK(esp_zb_humidity_meas_cluster_add_attr(esp_zb_humidity_meas_cluster, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_TOLERANCE_ID, &humidity_tolerance));

    // ------------------------------ Cluster Pressure ------------------------------
    esp_zb_pressure_meas_cluster_cfg_t pressure_meas_cfg = {
        .measured_value = 0x8000,
        .min_value = 200,
        .max_value = 1300,
    };
    // uint16_t pressure_tolerance = (CONFIG_PRJ_PRESSURE_SENSOR_ABSOLUTE_PRECISION / 100.0 + 0.5);
    int16_t pressure_scaled_value = 0x8000; // unit: 0.1hPa, scale=1
    int16_t pressure_min_scaled_value = 2000;
    int16_t pressure_max_scaled_value = 13000;
    //uint16_t pressure_scaled_tolerance = (1 / 10.0 + 0.5);
    uint8_t pressure_scale = 1;
    esp_zb_attribute_list_t *esp_zb_pressure_meas_cluster = esp_zb_pressure_meas_cluster_create(&pressure_meas_cfg);
    // ESP_ERROR_CHECK(esp_zb_pressure_meas_cluster_add_attr(esp_zb_pressure_meas_cluster, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_TOLERANCE_ID, &pressure_tolerance));  // not supported
    ESP_ERROR_CHECK(esp_zb_pressure_meas_cluster_add_attr(esp_zb_pressure_meas_cluster, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_SCALED_VALUE_ID, &pressure_scaled_value));
    ESP_ERROR_CHECK(esp_zb_pressure_meas_cluster_add_attr(esp_zb_pressure_meas_cluster, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_MIN_SCALED_VALUE_ID, &pressure_min_scaled_value));
    ESP_ERROR_CHECK(esp_zb_pressure_meas_cluster_add_attr(esp_zb_pressure_meas_cluster, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_MAX_SCALED_VALUE_ID, &pressure_max_scaled_value));
    //ESP_ERROR_CHECK(esp_zb_pressure_meas_cluster_add_attr(esp_zb_pressure_meas_cluster, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_SCALED_TOLERANCE_ID, &pressure_scaled_tolerance));
    ESP_ERROR_CHECK(esp_zb_pressure_meas_cluster_add_attr(esp_zb_pressure_meas_cluster, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_SCALE_ID, &pressure_scale));

    // ------------------------------ Create cluster list ------------------------------
    esp_zb_cluster_list_t *esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, esp_zb_basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list, esp_zb_identify_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_temperature_meas_cluster(esp_zb_cluster_list, esp_zb_temperature_meas_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_humidity_meas_cluster(esp_zb_cluster_list, esp_zb_humidity_meas_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_pressure_meas_cluster(esp_zb_cluster_list, esp_zb_pressure_meas_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // ------------------------------ Create endpoint list ------------------------------
    esp_zb_ep_list_t *esp_zb_ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t ep_config = {
        .endpoint = HA_SENSOR_ENDPOINT,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list, ep_config);

    // ------------------------------ Register Device ------------------------------
    esp_zb_device_register(esp_zb_ep_list);
    esp_zb_core_action_handler_register(zb_action_handler);

    esp_zb_zcl_reporting_info_t reporting_info = {
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
        .ep = HA_SENSOR_ENDPOINT,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 0,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 0,
        .u.send_info.delta.u16 = 100,
        .attr_id = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
        .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
    };

    esp_zb_zcl_update_reporting_info(&reporting_info);

    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);

    ESP_ERROR_CHECK(esp_zb_start(false));
        while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    esp_zb_main_loop_iteration();
}

esp_err_t writeAttribute(uint8_t endpoint, uint16_t clusterID, uint16_t attributeID, void *value)
{
    esp_zb_zcl_status_t status;
    esp_zb_lock_acquire(portMAX_DELAY);
    status = esp_zb_zcl_set_attribute_val(endpoint, clusterID, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, attributeID, value, false);
    esp_zb_lock_release();

    if (status != ESP_ZB_ZCL_STATUS_SUCCESS)
    {
        ESP_LOGE(TAG, "Setting attribute %04x:%04x failed(0x%02x)!", clusterID, attributeID, status);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t reportAttribute(uint8_t endpoint, uint16_t clusterID, uint16_t attributeID)
{
    esp_zb_zcl_status_t status;
    esp_zb_zcl_report_attr_cmd_t cmd = {
        .zcl_basic_cmd = {
            // .dst_addr_u.addr_short = 0x0000,
            // .dst_endpoint = endpoint,
            .src_endpoint = endpoint,
        },
        .address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
        .clusterID = clusterID,
        .attributeID = attributeID,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    };
    esp_zb_lock_acquire(portMAX_DELAY);
    status = esp_zb_zcl_report_attr_cmd_req(&cmd);
    esp_zb_lock_release();

    if (status != ESP_ZB_ZCL_STATUS_SUCCESS)
    {
        ESP_LOGE(TAG, "Updating attribute %04x:%04x failed(0x%02x)!", clusterID, attributeID, status);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(0x%x), cluster(0x%x), attribute(0x%x), data size(%d)", message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size);
    // if (message->info.dst_endpoint == HA_SENSOR_ENDPOINT)
    // {
    //     if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF)
    //     {
    //         if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL)
    //         {
    //             light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;
    //             gpio_set_level(GPIO_NUM_0, light_state);
    //             ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");
    //         }
    //     }
    // }
    
    return ret;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id)
    {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    case ESP_ZB_CORE_CMD_DEFAULT_RESP_CB_ID:
        break;
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type)
    {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Zigbee stack initialized");
        xEventGroupSetBits(xZigbeeEvents, ZIGBEE_INIT_SUCCESS);
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK)
        {
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");

            ESP_LOGI(TAG, "Start network steering");
            xEventGroupSetBits(xZigbeeEvents, ZIGBEE_STEERING_START);
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        }
        else
        {
            /* commissioning failed */
            xEventGroupSetBits(xZigbeeEvents, ZIGBEE_INIT_FAILED);
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK)
        {
            xEventGroupSetBits(xZigbeeEvents, ZIGBEE_NETWORK_JOINED);
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel());
        }
        else
        {
            xEventGroupSetBits(xZigbeeEvents, ZIGBEE_STEERING_FAILED);
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
        //ESP_LOGI(TAG, "Zigbee can sleep");
        esp_zb_sleep_now();
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}


static void app_zb_report_temperature(float temperature_c)
{
    int16_t temperature_int = temperature_c * 100 + 0.5;
    writeAttribute(HA_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &temperature_int);
    reportAttribute(HA_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID);
    return;
}

static void app_zb_report_humidity(float humidity_percent)
{
    uint16_t humidity_int = humidity_percent * 100 + 0.5;
    writeAttribute(HA_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, &humidity_int);
    reportAttribute(HA_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID);
    return;
}

static void app_zb_report_pressure(float pressure_hpa)
{
    int16_t pressure_hpa_int = pressure_hpa + 0.5;
    int16_t pressure_0_1_hpa_int = pressure_hpa * 10 + 0.5;
    ESP_LOGI(TAG, "Pre: %.2f", pressure_hpa);
    writeAttribute(HA_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID, &pressure_hpa_int); // basic
    reportAttribute(HA_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID);

    writeAttribute(HA_SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT, ESP_ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_SCALED_VALUE_ID, &pressure_0_1_hpa_int); // PRESSURE_MEASUREMENT_SCALED_VALUE is not reportable
    return;
}


void zigbee_report_task(void *pvParameters)
{
    while(1)
    {
        zigbee_report_data_t report_data;
        if(xQueueReceive(zigbeeReportDataQueue, &report_data, portMAX_DELAY) == pdPASS)
        {
            app_zb_report_temperature(report_data.temperature);
            app_zb_report_humidity(report_data.humidity);
            app_zb_report_pressure(report_data.pressure);
        }
    }
}