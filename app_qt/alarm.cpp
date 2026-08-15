/**
 * alarm.cpp - 声光报警 + 继电器控制实现
 */
#include "alarm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

/* 设备文件描述符 */
static int buzzer_fd  = -1;     // /dev/pwm
static int led_fd     = -1;     // /dev/Led (6_9_led_driver)
static int relay_fd   = -1;     // /dev/relay

void alarm_init(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Simulated mode\n");
    return;
#endif
    buzzer_fd  = open("/dev/pwm", O_RDWR);
    led_fd     = open("/dev/Led", O_RDWR);
    relay_fd   = open("/dev/relay", O_RDWR);

    if (buzzer_fd  < 0) perror("[ALARM] /dev/pwm");
    if (led_fd     < 0) perror("[ALARM] /dev/Led");
    if (relay_fd   < 0) perror("[ALARM] /dev/relay");

    /* 初始熄灭 */
    if (led_fd >= 0) {
        ioctl(led_fd, LED1, 1);   /* 红灯灭 */
        ioctl(led_fd, LED2, 1);   /* 绿灯灭 */
    }

    printf("[ALARM] Initialized\n");
}

void alarm_close(void)
{
    /* 关闭前确保所有设备归位 */
    alarm_deactivate();
    alarm_relay_off();

    if (buzzer_fd >= 0) { close(buzzer_fd);  buzzer_fd  = -1; }
    if (led_fd    >= 0) { close(led_fd);     led_fd     = -1; }
    if (relay_fd  >= 0) { close(relay_fd);   relay_fd   = -1; }

    printf("[ALARM] Closed\n");
}

/* ---- 蜂鸣器控制 ---- */
static void buzzer_on(int freq)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Buzzer ON (%d Hz)\n", freq);
    return;
#endif
    if (buzzer_fd >= 0)
        ioctl(buzzer_fd, 1, freq);   /* cmd=1: 开 */
}

static void buzzer_off(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Buzzer OFF\n");
    return;
#endif
    if (buzzer_fd >= 0)
        ioctl(buzzer_fd, 0, 0);      /* cmd=0: 关 */
}

/* ---- LED 控制 (统一 /dev/Led 驱动) ---- */
void alarm_red_led_on(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Red LED ON\n");
    return;
#endif
    if (led_fd >= 0)
        ioctl(led_fd, LED1, 0);      /* arg=0: ON */
}

void alarm_red_led_off(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Red LED OFF\n");
    return;
#endif
    if (led_fd >= 0)
        ioctl(led_fd, LED1, 1);      /* arg=1: OFF */
}

void alarm_green_led_on(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Green LED ON\n");
    return;
#endif
    if (led_fd >= 0)
        ioctl(led_fd, LED2, 0);      /* arg=0: ON */
}

void alarm_green_led_off(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Green LED OFF\n");
    return;
#endif
    if (led_fd >= 0)
        ioctl(led_fd, LED2, 1);      /* arg=1: OFF */
}

/* ---- 继电器控制 ---- */
void alarm_relay_on(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Relay ON (door open)\n");
    return;
#endif
    if (relay_fd >= 0)
        ioctl(relay_fd, 1, 0);
}

void alarm_relay_off(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Relay OFF (door closed)\n");
    return;
#endif
    if (relay_fd >= 0)
        ioctl(relay_fd, 0, 0);
}

/* ---- 绿灯指示 (开门) ---- */
void alarm_indicate_door_open(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[ALARM] Green LED: door open indicator\n");
    return;
#endif
    if (led_fd < 0) return;
    /* 闪烁3次 */
    for (int i = 0; i < 3; i++) {
        ioctl(led_fd, LED2, 0);      /* 亮 */
        usleep(150000);
        ioctl(led_fd, LED2, 1);      /* 灭 */
        usleep(100000);
    }
}

/* ---- 报警激活/解除 ---- */
void alarm_activate(void)
{
    printf("[ALARM] Activating all alarms!\n");
    buzzer_on(2000);         /* 2kHz 蜂鸣 */
    alarm_red_led_on();      /* 红灯常亮 */
    alarm_green_led_off();   /* 绿灯灭 */
    alarm_relay_on();        /* 继电器动作 (可接排风扇或阀门) */
}

void alarm_deactivate(void)
{
    printf("[ALARM] Deactivating all alarms\n");
    buzzer_off();
    alarm_red_led_off();
    alarm_green_led_on();
    alarm_relay_off();
}
