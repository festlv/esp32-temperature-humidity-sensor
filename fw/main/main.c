/* ESP32 temperature and humidity sensor

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/adc.h"
#include "esp_event_loop.h"
#include "esp_deep_sleep.h"
#include "influx.h"
#include "si7021.h"

static const char *TAG = "sens";

static void led_blink(void)
{
    gpio_set_level(CONFIG_LED_GPIO_NUM, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(CONFIG_LED_GPIO_NUM, 0);
}

bool read_done = false;
float temperature_reading, humidity_reading, battery_voltage_reading;

#define I2C_MASTER_PORT 1

static void i2c_init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 23;
    conf.sda_pullup_en = 0;
    conf.scl_io_num = 22;
    conf.scl_pullup_en = 0;
    conf.master.clk_speed = 400000;
    i2c_param_config(I2C_MASTER_PORT, &conf);
    uint32_t rxbuf_len = 0;
    uint32_t txbuf_len = 0;
    i2c_driver_install(I2C_MASTER_PORT, conf.mode, rxbuf_len, txbuf_len, 0);
}

static void start_sleep(void)
{
    esp_deep_sleep_enable_timer_wakeup(1000000LL * CONFIG_WAKEUP_INTERVAL);
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
    esp_deep_sleep_enable_ext0_wakeup(CONFIG_WAKEUP_GPIO_NUM, CONFIG_WAKEUP_GPIO_LEVEL);
    ESP_LOGI(TAG, "entering deep sleep mode");
    esp_deep_sleep_start();
}

void timer_callback(TimerHandle_t timer)
{
    (void)timer;
    ESP_LOGI(TAG, "sleep from timer");
    start_sleep();
}

char mac_str[32];

void user_publish(void)
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);

    snprintf(mac_str, sizeof(mac_str), "%02x-%02x-%02x-%02x-%02x-%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    char temp_str[16];
    char hum_str[16];
    char volt_str[16];

    snprintf(temp_str, sizeof(temp_str), "%.2f", temperature_reading);
    snprintf(hum_str, sizeof(hum_str), "%.2f", humidity_reading);
    snprintf(volt_str, sizeof(volt_str), "%.2f", battery_voltage_reading);
    ESP_LOGI(TAG, "publishing measurements");
    publish_measurements(mac_str, hum_str, temp_str, volt_str);
    ESP_LOGI(TAG, "measurements published");
    start_sleep();
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
	ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
	ESP_ERROR_CHECK(esp_wifi_connect());
	break;
    case SYSTEM_EVENT_STA_GOT_IP:
    {
	ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
	ESP_LOGI(TAG, "got ip:%s\n",
		ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

    led_blink();

    user_publish();

    }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
	ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
    //start_sleep();
	ESP_ERROR_CHECK(esp_wifi_connect());
	break;
    default:
        break;
    }
    return ESP_OK;
}




static void wifi_init(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
	.sta = {
	    .ssid = CONFIG_WIFI_SSID,
	    .password = CONFIG_WIFI_PASSWORD
	},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
TimerHandle_t timer;
void app_main()
{
    gpio_pad_select_gpio(CONFIG_LED_GPIO_NUM);
    gpio_set_direction(CONFIG_LED_GPIO_NUM, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_LED_GPIO_NUM, 0);
    
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_CHANNEL_6,ADC_ATTEN_0db);
    int adc_counts = adc1_get_voltage(ADC1_CHANNEL_6);
    battery_voltage_reading  = ((adc_counts / 4096.0f) * 3.3f) / 0.659f;

    led_blink();

    i2c_init();
    vTaskDelay(80 / portTICK_PERIOD_MS);
    bool ret = si7021_read(I2C_MASTER_PORT, &humidity_reading, &temperature_reading);
    if (ret)
    {
        ESP_LOGI(TAG,"humidity=%.2f%%, temperature=%.2fdegC",
                humidity_reading, temperature_reading);
        read_done = true;
        wifi_init();
    } else {
        ESP_LOGE(TAG, "could not read si7021");
        read_done = false;
        start_sleep();
    }
    timer = xTimerCreate("sleeptimer", 5000 / portTICK_PERIOD_MS, false, 0, timer_callback);
    xTimerStart(timer, 0);
}
