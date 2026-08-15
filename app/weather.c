/*
 * DHT11 Weather (Temperature + Humidity) Module Implementation
 * Hardware: DHT11 sensor via GPIO
 * Device: /dev/humidity
 * Read returns 4 bytes: hum_int, hum_dec, temp_int, temp_dec
 * Temperature range: 0~50°C, Humidity: 20~90%
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "weather.h"

static int weather_fd = -1;

int weather_init(void)
{
    weather_fd = open(WEATHER_DEVICE, O_RDWR);
    if (weather_fd < 0) {
        perror("open " WEATHER_DEVICE);
        return -1;
    }
    printf("Weather (DHT11): %s opened\n", WEATHER_DEVICE);
    return 0;
}

void weather_close(void)
{
    if (weather_fd >= 0) {
        close(weather_fd);
        weather_fd = -1;
    }
}

int weather_read(float *temp, float *humidity)
{
    unsigned long raw_data = 0;
    unsigned char hum_int, hum_dec, temp_int, temp_dec;
    int ret;

    if (weather_fd < 0) {
        weather_init();
        if (weather_fd < 0)
            return -1;
    }

    if (!temp || !humidity)
        return -1;

    ret = read(weather_fd, &raw_data, sizeof(raw_data));
    if (ret < 0) {
        perror("read DHT11");
        return -1;
    }

    /* Parse: raw_data = (hum_int << 24) | (hum_dec << 16) | (temp_int << 8) | temp_dec */
    hum_int  = (raw_data & 0xFF000000) >> 24;
    hum_dec  = (raw_data & 0x00FF0000) >> 16;
    temp_int = (raw_data & 0x0000FF00) >> 8;
    temp_dec = (raw_data & 0x000000FF);

    /* Check for valid range */
    if (temp_int > 100 || hum_int > 100) {
        /* Likely a read error, DHT11 only goes to 50°C / 90% */
        return -1;
    }

    *humidity = (float)hum_int + (float)hum_dec / 100.0f;
    *temp     = (float)temp_int + (float)temp_dec / 100.0f;

    printf("DHT11: temp=%d.%d°C, hum=%d.%d%%\n",
           temp_int, temp_dec, hum_int, hum_dec);

    return 0;
}
