#pragma once

#include <stdbool.h>
#include <driver/i2c.h>

bool si7021_read(i2c_port_t i2c_port, float* humidity, float* temperature);
