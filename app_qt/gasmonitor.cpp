/**
 * gasmonitor.cpp - MQ-2气体传感器
 */
#include "gasmonitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static int gas_fd = -1;

void gas_init(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[GAS] Simulated mode\n");
    return;
#endif
    gas_fd = open("/dev/gec_gas_drv", O_RDONLY);
    if (gas_fd < 0) {
        perror("[GAS] Failed to open /dev/gec_gas_drv");
        return;
    }
    printf("[GAS] /dev/gec_gas_drv opened\n");
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
#ifdef SIMULATE_HARDWARE
    /* 模拟: 大部分时间正常，偶尔报警 */
    static int count = 0;
    count++;
    /* 默认正常 */
    return 0;
#endif
    if (gas_fd < 0) return -1;

    char buf[4];
    int n = read(gas_fd, buf, sizeof(buf) - 1);
    if (n <= 0) return -1;

    buf[n] = '\0';
    if (buf[0] == '1') return 1;   /* 报警 */
    return 0;                       /* 正常 */
}
