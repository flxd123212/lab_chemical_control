/*
 * Water / Liquid Level Sensor Module
 * Hardware: Water level sensor via ADC (channel 2)
 * Device: /dev/gec6818_adc
 * Reads analog value (0-4095 or similar), higher = more water
 * Threshold: define WATER_THRESHOLD based on calibration
 */

#ifndef WATER_SENSOR_H
#define WATER_SENSOR_H

#define WATER_DEVICE    "/dev/gec6818_adc"
#define ADC_CHANNEL     2           /* ADC channel for water sensor */

/* Default threshold - adjust after calibration */
#define WATER_THRESHOLD 800         /* Above this = water detected */

int  water_init(void);
void water_close(void);

/* Read water sensor. Returns raw ADC value (0~4095), -1 on error. */
int  water_read(void);

/* Check if water is detected. Returns 1 if water, 0 if dry, -1 on error. */
int  water_is_detected(void);

#endif /* WATER_SENSOR_H */
