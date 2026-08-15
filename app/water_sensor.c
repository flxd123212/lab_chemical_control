/*
 * Water / Liquid Level Sensor Module Implementation
 * Hardware: Water level sensor via ADC (channel 2)
 * Device: /dev/gec6818_adc
 * Uses ioctl to select channel, then read for raw value
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "water_sensor.h"

static int adc_fd = -1;

/* IOCTL commands from the ADC driver */
#define Channels1 _IO('k', 0x1a)
#define Channels2 _IO('k', 0x1b)

int water_init(void)
{
    adc_fd = open(WATER_DEVICE, O_RDWR);
    if (adc_fd < 0) {
        perror("open " WATER_DEVICE);
        return -1;
    }

    /* Select ADC channel */
    if (ioctl(adc_fd, Channels2, ADC_CHANNEL) < 0) {
        perror("ioctl set ADC channel");
        /* Non-fatal, driver may use default */
    }

    printf("Water sensor: %s opened, channel=%d\n", WATER_DEVICE, ADC_CHANNEL);
    return 0;
}

void water_close(void)
{
    if (adc_fd >= 0) {
        close(adc_fd);
        adc_fd = -1;
    }
}

int water_read(void)
{
    unsigned int value = 0;
    int ret;

    if (adc_fd < 0) {
        water_init();
        if (adc_fd < 0)
            return -1;
    }

    ret = read(adc_fd, &value, sizeof(value));
    if (ret < 0) {
        perror("read ADC");
        return -1;
    }

    return (int)value;
}

int water_is_detected(void)
{
    int val = water_read();
    if (val < 0)
        return -1;

    return (val >= WATER_THRESHOLD) ? 1 : 0;
}
