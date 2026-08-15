/*
 * Alarm Module Implementation
 * Hardware:
 *   - Buzzer: /dev/pwm (PWM2, GPIO C+14)
 *   - LEDs:   /dev/Led (6_9_led_driver)
 *       LED1(GPIOE13) = Red LED  (D7)
 *       LED2(GPIOC17) = Green LED (D8)
 *   - Relay: /dev/relay (GPIO B+28)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "alarm.h"

static int pwm_fd     = -1;
static int led_fd     = -1;     /* /dev/Led (统一 LED 驱动) */
static int relay_fd   = -1;

/* ====== PWM / Buzzer ====== */
int alarm_pwm_init(void)
{
    pwm_fd = open("/dev/pwm", O_RDWR);
    if (pwm_fd < 0) {
        perror("open /dev/pwm");
        return -1;
    }
    return 0;
}

void alarm_beep_on(int freq_hz)
{
    if (pwm_fd < 0) return;
    ioctl(pwm_fd, PWM_IOCTL_SET_FREQ, (unsigned long)freq_hz);
}

void alarm_beep_off(void)
{
    if (pwm_fd < 0) return;
    ioctl(pwm_fd, PWM_IOCTL_STOP, 0);
}

/* ====== LED 控制 (统一 /dev/Led 驱动) ====== */
int alarm_led_init(void)
{
    led_fd = open("/dev/Led", O_RDWR);
    if (led_fd < 0) {
        perror("open /dev/Led (6_9_led_driver)");
        return -1;
    }
    /* 初始: 全灭 */
    ioctl(led_fd, LED1, 1);   /* 红灯灭 */
    ioctl(led_fd, LED2, 1);   /* 绿灯灭 */
    return 0;
}

void alarm_red_led_on(void)
{
    if (led_fd < 0) return;
    ioctl(led_fd, LED1, 0);   /* arg=0 = ON */
}

void alarm_red_led_off(void)
{
    if (led_fd < 0) return;
    ioctl(led_fd, LED1, 1);   /* arg=1 = OFF */
}

void alarm_green_led_on(void)
{
    if (led_fd < 0) return;
    ioctl(led_fd, LED2, 0);   /* arg=0 = ON */
}

void alarm_green_led_off(void)
{
    if (led_fd < 0) return;
    ioctl(led_fd, LED2, 1);   /* arg=1 = OFF */
}

/* ====== Relay (/dev/relay) ====== */
int alarm_relay_init(void)
{
    relay_fd = open("/dev/relay", O_RDWR);
    if (relay_fd < 0) {
        perror("open /dev/relay");
        return -1;
    }
    /* Initial state: relay off */
    ioctl(relay_fd, 0, 0);
    return 0;
}

void alarm_relay_on(void)
{
    if (relay_fd < 0) return;
    ioctl(relay_fd, 1, 0);  /* cmd=1 = on */
    printf("Relay: ON\n");
}

void alarm_relay_off(void)
{
    if (relay_fd < 0) return;
    ioctl(relay_fd, 0, 0);  /* cmd=0 = off */
    printf("Relay: OFF\n");
}

/* ====== Combined Init / Actions ====== */
int alarm_init(void)
{
    int ok = 1;

    if (alarm_pwm_init() < 0) ok = 0;
    if (alarm_led_init() < 0)  ok = 0;
    if (alarm_relay_init() < 0) ok = 0;

    /* Start in normal state */
    alarm_beep_off();
    alarm_red_led_off();
    alarm_green_led_on();
    alarm_relay_off();

    return ok ? 0 : -1;
}

void alarm_close(void)
{
    alarm_beep_off();
    alarm_red_led_off();
    alarm_green_led_off();
    alarm_relay_off();

    if (pwm_fd >= 0)     { close(pwm_fd);     pwm_fd = -1; }
    if (led_fd >= 0)     { close(led_fd);     led_fd = -1; }
    if (relay_fd >= 0)   { close(relay_fd);   relay_fd = -1; }
}

void alarm_activate(void)
{
    printf("ALARM ACTIVATED!\n");
    alarm_beep_on(2000);      /* 2kHz buzzer */
    alarm_red_led_on();       /* Red LED on */
    alarm_green_led_off();    /* Green LED off */
    alarm_relay_on();         /* Activate relay (e.g., cut gas/fan on) */
}

void alarm_deactivate(void)
{
    printf("Alarm deactivated\n");
    alarm_beep_off();
    alarm_red_led_off();
    alarm_green_led_on();
    alarm_relay_off();
}

void alarm_indicate_normal(void)
{
    alarm_red_led_off();
    alarm_green_led_on();
    alarm_beep_off();
}

void alarm_indicate_door_open(void)
{
    /* Quick green flash to indicate door access granted */
    alarm_green_led_on();
    usleep(200000);
    alarm_green_led_off();
    usleep(100000);
    alarm_green_led_on();
    usleep(200000);
    alarm_green_led_off();
}
