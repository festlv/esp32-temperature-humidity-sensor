#include "si7021.h"

#include "esp_log.h"

static const char *TAG = "si7021";

#define SI7021_ADDR (0x40)
#define SI7021_MEASURE_HUM_NO_HOLD  (0xF5)
#define SI7021_READ_TEMPERATURE     (0xE0)
#define SI7021_MEASURE_TEMP_NO_HOLD (0xF3)

bool si7021_read(i2c_port_t i2c_num, float* humidity, float* temperature)
{
    //start humidity conversion
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SI7021_ADDR << 1 | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, SI7021_MEASURE_HUM_NO_HOLD, 1);
    i2c_master_stop(cmd);

    int ret = i2c_master_cmd_begin(i2c_num, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "failed to start humidity conversion");
        return false;
    }

    //wait for conversion end
    vTaskDelay(30 / portTICK_PERIOD_MS);
    
    //read humidity value
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SI7021_ADDR << 1 | I2C_MASTER_READ, 1);
    uint8_t hum_low, hum_high;
    i2c_master_read_byte(cmd, &hum_high, 1);
    i2c_master_read_byte(cmd, &hum_low, 0);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "failed to read humidity result");
        return false;
    }
    //convert humidity according to formula in datasheet
    int32_t hum = ((int32_t)hum_high) << 8 | hum_low;

    *humidity = (125.0f * hum) / (65536.0f) - 6.0f;
    
    //start temperature conversion
    //reading temperature associated with previous conversion was quite erratic
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SI7021_ADDR << 1 | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, SI7021_MEASURE_TEMP_NO_HOLD, 1);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "failed to start temperature conversion");
        return false;
    }
 
    //wait for conversion end
    vTaskDelay(15 / portTICK_PERIOD_MS);
 
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    uint8_t temp_low, temp_high;
    i2c_master_write_byte(cmd, SI7021_ADDR << 1 | I2C_MASTER_READ, 1);
    i2c_master_read_byte(cmd, &temp_high, 1);
    i2c_master_read_byte(cmd, &temp_low, 0);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "failed to read temperature measurement");
        return false;
    }
    int32_t temp = ((int32_t)temp_high) << 8 | temp_low;

    *temperature = ((175.72f * temp) / 65536.0f) - 46.85f;

    return true;
}
