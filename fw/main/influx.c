#include "influx.h"
#include <unistd.h>
#include <stdio.h>
#include "esp_request.h"

bool publish_measurements(char* mac, char* humidity, char* temperature, char* voltage)
{
    request_t* req;
    char url[256];
    snprintf(url, sizeof(url), "%s/write?db=%s&u=%s&p=%s",
            CONFIG_INFLUXDB_BASE_URL,
            CONFIG_INFLUXDB_DATABASE,
            CONFIG_INFLUXDB_USERNAME,
            CONFIG_INFLUXDB_PASSWORD);

    req = req_new(url);
    req_setopt(req, REQ_SET_METHOD, "POST");
    char write_line[256];
    snprintf(write_line, 
            sizeof(write_line),
            "measurements,mac=%s temperature=%s,humidity=%s,voltage=%s",
            mac, temperature, humidity, voltage);
    req_setopt(req, REQ_SET_POSTFIELDS, write_line);

    int status = req_perform(req);
    req_clean(req);
    return status == 0;
}
