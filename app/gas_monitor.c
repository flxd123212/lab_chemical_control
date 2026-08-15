/*
 * Gas Sensor Monitor Module Implementation
 * Hardware: MQ-2 gas/smoke sensor via GPIO
 * Device: /dev/gec_gas_drv
 * Read returns: '0' = normal, '1' = alarm
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "gas_monitor.h"

static int gas_fd = -1;

int gas_init(void)
{
    gas_fd = open(GAS_DEVICE, O_RDONLY);
    if (gas_fd < 0) {
        perror("open " GAS_DEVICE);
        return -1;
    }
    printf("Gas sensor: %s opened\n", GAS_DEVICE);
    return 0;
}

void gas_close(void)
{
    if (gas_fd >= 0) {
        close(gas_fd);
        gas_fd = -1;
    }
}

int gas_read(void)
{
    char buf[4];
    int  ret;

    if (gas_fd < 0) {
        /* Try to re-open */
        gas_init();
        if (gas_fd < 0)
            return -1;
    }

    lseek(gas_fd, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    ret = read(gas_fd, buf, sizeof(buf) - 1);
    if (ret < 0) {
        perror("read gas sensor");
        return -1;
    }

    if (buf[0] == '0')
        return 0;   /* Normal */
    else
        return 1;   /* Gas detected / alarm */
}
