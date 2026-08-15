/*
 * DHT11 Weather (Temperature + Humidity) Module
 * Hardware: DHT11 sensor via GPIO
 * Device: /dev/humidity
 * Read returns 4 bytes: hum_int, hum_dec, temp_int, temp_dec
 */

#ifndef WEATHER_H
#define WEATHER_H

#define WEATHER_DEVICE "/dev/humidity"

int  weather_init(void);
void weather_close(void);

/* Read temperature and humidity. Returns 0 on success, -1 on error.
 * temp / hum are in Celsius and percentage, with decimal part. */
int  weather_read(float *temp, float *humidity);

#endif /* WEATHER_H */
