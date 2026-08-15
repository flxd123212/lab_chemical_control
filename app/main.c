/*
 * 实验室危化品库房环境智能管控系统
 * Laboratory Hazardous Chemical Warehouse Environmental Intelligent Control System
 *
 * Target: GEC6818 Development Board (800x480 LCD)
 * Compiler: arm-linux-gnueabihf-gcc
 *
 * Features:
 *  - RFID card access control
 *  - Gas leak monitoring (MQ-2)
 *  - Temperature & humidity monitoring (DHT11)
 *  - Water leak / liquid detection (ADC)
 *  - Audio-visual alarm (PWM buzzer + LED)
 *  - Relay control (door/gas valve/fan)
 *  - Real-time LCD display UI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

#include "ui_lcd.h"
#include "rfid_card.h"
#include "gas_monitor.h"
#include "weather.h"
#include "water_sensor.h"
#include "alarm.h"

/* ======================== UI Layout Constants ======================== */
#define TITLE_BAR_H       40
#define FOOTER_BAR_H      20
#define CARD_INFO_BAR_H   30
#define MAIN_AREA_Y       (TITLE_BAR_H)
#define MAIN_AREA_H       (SCREEN_HEIGHT - TITLE_BAR_H - CARD_INFO_BAR_H - FOOTER_BAR_H)
#define CARD_INFO_Y       (SCREEN_HEIGHT - FOOTER_BAR_H - CARD_INFO_BAR_H)
#define FOOTER_Y          (SCREEN_HEIGHT - FOOTER_BAR_H)

#define LEFT_PANEL_W      280
#define RIGHT_PANEL_X     (LEFT_PANEL_W + 4)
#define RIGHT_PANEL_W     (SCREEN_WIDTH - RIGHT_PANEL_X - 4)

/* ======================== Global State ======================== */
static volatile int running = 1;

/* Sensor data */
typedef struct {
    int  gas_alarm;          /* 0=normal, 1=alarm */
    int  water_alarm;        /* 0=normal, 1=alarm */
    int  water_raw;          /* Raw ADC value */
    float temperature;       /* Celsius */
    float humidity;          /* Percentage */
    int   weather_valid;     /* 0=stale, 1=fresh */
} sensor_data_t;

static sensor_data_t sensors;
static int alarm_active = 0;
static char last_card_id[MAX_CARD_LEN] = "";
static int  last_card_verified = 0;
static time_t last_card_time = 0;
static time_t start_time;
static time_t last_weather_time = 0;

/* ======================== UI Drawing Functions ======================== */

void draw_title_bar(void)
{
    char buf[64];

    /* Background */
    lcd_fill_rect(0, 0, SCREEN_WIDTH, TITLE_BAR_H, COLOR_NAVY);
    /* Bottom border line */
    lcd_draw_rect(0, 0, SCREEN_WIDTH, TITLE_BAR_H, COLOR_BLUE);

    /* Title text */
    snprintf(buf, sizeof(buf), "  \xb7\xc0\xca\xd4\xc6\xb7\xbf\xe2\xb7\xbf\xbb\xb7\xbe\xb3\xd6\xc7\xc4\xdc\xbf\xd8\xcf\xb5\xcd\xb3");
    lcd_draw_string(10, 8, "Lab Chemical Warehouse Env Control System v1.0", COLOR_WHITE, COLOR_NAVY);
}

void draw_status_panel(int y, int h)
{
    int x = 4;
    int w = LEFT_PANEL_W;

    /* Panel background */
    lcd_fill_rect(x, y, w, h, COLOR_BLACK);
    /* Border */
    lcd_draw_rect(x, y, w, h, COLOR_DARK_GRAY);

    /* Panel title */
    lcd_fill_rect(x + 1, y + 1, w - 2, 20, COLOR_DARK_BLUE);
    lcd_draw_string(x + 6, y + 4, "System Status", COLOR_WHITE, COLOR_DARK_BLUE);
}

void draw_data_panel(int y, int h)
{
    /* Panel background */
    lcd_fill_rect(RIGHT_PANEL_X, y, RIGHT_PANEL_W, h, COLOR_BLACK);
    /* Border */
    lcd_draw_rect(RIGHT_PANEL_X, y, RIGHT_PANEL_W, h, COLOR_DARK_GRAY);

    /* Panel title */
    lcd_fill_rect(RIGHT_PANEL_X + 1, y + 1, RIGHT_PANEL_W - 2, 20, COLOR_DARK_BLUE);
    lcd_draw_string(RIGHT_PANEL_X + 6, y + 4, "Sensor Monitor", COLOR_WHITE, COLOR_DARK_BLUE);
}

void draw_card_info_bar(void)
{
    char buf[100];
    time_t now;
    struct tm *tm_info;

    lcd_fill_rect(0, CARD_INFO_Y, SCREEN_WIDTH, CARD_INFO_BAR_H, COLOR_DARK_GRAY);
    lcd_draw_rect(0, CARD_INFO_Y, SCREEN_WIDTH, CARD_INFO_BAR_H, COLOR_GRAY);

    now = time(NULL);
    tm_info = localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    lcd_draw_string(10, CARD_INFO_Y + 6, buf, COLOR_WHITE, COLOR_DARK_GRAY);

    if (last_card_time > 0) {
        snprintf(buf, sizeof(buf), "Card: %s [%s]",
                 last_card_id,
                 last_card_verified ? "GRANTED" : "REJECTED");
        lcd_draw_string(250, CARD_INFO_Y + 6, buf,
                        last_card_verified ? COLOR_GREEN : COLOR_RED,
                        COLOR_DARK_GRAY);
    } else {
        lcd_draw_string(250, CARD_INFO_Y + 6, "Card: -- No card read --",
                        COLOR_GRAY, COLOR_DARK_GRAY);
    }
}

void draw_footer(void)
{
    char buf[64];
    time_t uptime = time(NULL) - start_time;
    int h = uptime / 3600;
    int m = (uptime % 3600) / 60;
    int s = uptime % 60;

    lcd_fill_rect(0, FOOTER_Y, SCREEN_WIDTH, FOOTER_BAR_H, COLOR_NAVY);
    snprintf(buf, sizeof(buf), "Uptime: %02d:%02d:%02d", h, m, s);
    lcd_draw_string(10, FOOTER_Y + 2, buf, COLOR_LIGHT_GRAY, COLOR_NAVY);
    lcd_draw_string(SCREEN_WIDTH - 200, FOOTER_Y + 2,
                    "GEC6818 - Hazard Chemical Control", COLOR_LIGHT_GRAY, COLOR_NAVY);
}

void update_status_display(int y, int h)
{
    char buf[64];
    int line_y = y + 24;
    int line_h = 20;

    /* Gas status */
    lcd_fill_rect(8, line_y, LEFT_PANEL_W - 12, line_h, COLOR_BLACK);
    if (sensors.gas_alarm) {
        snprintf(buf, sizeof(buf), "Gas:  ALARM! (leak detected)");
        lcd_draw_string(10, line_y + 2, buf, COLOR_RED, COLOR_BLACK);
    } else {
        snprintf(buf, sizeof(buf), "Gas:  \xE2\x9C\x93 Normal");
        lcd_draw_string(10, line_y + 2, buf, COLOR_GREEN, COLOR_BLACK);
    }
    line_y += line_h + 2;

    /* Water status */
    lcd_fill_rect(8, line_y, LEFT_PANEL_W - 12, line_h, COLOR_BLACK);
    if (sensors.water_alarm) {
        snprintf(buf, sizeof(buf), "Water: ALARM! (leak detected)");
        lcd_draw_string(10, line_y + 2, buf, COLOR_RED, COLOR_BLACK);
    } else {
        snprintf(buf, sizeof(buf), "Water: \xE2\x9C\x93 Normal (%d)", sensors.water_raw);
        lcd_draw_string(10, line_y + 2, buf, COLOR_GREEN, COLOR_BLACK);
    }
    line_y += line_h + 2;

    /* Gate status */
    lcd_fill_rect(8, line_y, LEFT_PANEL_W - 12, line_h, COLOR_BLACK);
    if (last_card_verified) {
        snprintf(buf, sizeof(buf), "Gate:  OPEN (access granted)");
        lcd_draw_string(10, line_y + 2, buf, COLOR_CYAN, COLOR_BLACK);
    } else {
        snprintf(buf, sizeof(buf), "Gate:  \xE2\x9C\x93 Locked");
        lcd_draw_string(10, line_y + 2, buf, COLOR_GREEN, COLOR_BLACK);
    }
    line_y += line_h + 2;

    /* Alarm state */
    lcd_fill_rect(8, line_y, LEFT_PANEL_W - 12, line_h, COLOR_BLACK);
    if (alarm_active) {
        lcd_fill_rect(8, line_y, LEFT_PANEL_W - 12, line_h, COLOR_DARK_RED);
        lcd_draw_string(10, line_y + 2, "!!! ALARM ACTIVE !!!", COLOR_WHITE, COLOR_DARK_RED);
    } else {
        lcd_draw_string(10, line_y + 2, "Status: \xE2\x9C\x93 All Normal", COLOR_GREEN, COLOR_BLACK);
    }
}

void update_data_display(int y, int h)
{
    char buf[64];
    int line_y = y + 24;
    int line_h = 24;
    int x = RIGHT_PANEL_X + 10;

    /* Temperature */
    lcd_fill_rect(x, line_y, RIGHT_PANEL_W - 20, line_h, COLOR_BLACK);
    if (sensors.weather_valid) {
        snprintf(buf, sizeof(buf), "Temperature:  %.1f \xE2\x84\x83", sensors.temperature);
    } else {
        snprintf(buf, sizeof(buf), "Temperature:  -- \xE2\x84\x83");
    }
    lcd_draw_string(x, line_y + 4, buf, COLOR_CYAN, COLOR_BLACK);
    line_y += line_h + 2;

    /* Humidity */
    lcd_fill_rect(x, line_y, RIGHT_PANEL_W - 20, line_h, COLOR_BLACK);
    if (sensors.weather_valid) {
        snprintf(buf, sizeof(buf), "Humidity:     %.1f %%", sensors.humidity);
    } else {
        snprintf(buf, sizeof(buf), "Humidity:     -- %%");
    }
    lcd_draw_string(x, line_y + 4, buf, COLOR_CYAN, COLOR_BLACK);
    line_y += line_h + 2;

    /* Gas reading */
    lcd_fill_rect(x, line_y, RIGHT_PANEL_W - 20, line_h, COLOR_BLACK);
    if (sensors.gas_alarm) {
        lcd_fill_rect(x, line_y, RIGHT_PANEL_W - 20, line_h, COLOR_DARK_RED);
        lcd_draw_string(x, line_y + 4, "Gas/Smoke:   *** DETECTED ***", COLOR_WHITE, COLOR_DARK_RED);
    } else {
        lcd_draw_string(x, line_y + 4, "Gas/Smoke:   Clear (Normal)", COLOR_GREEN, COLOR_BLACK);
    }
    line_y += line_h + 2;

    /* Water reading */
    lcd_fill_rect(x, line_y, RIGHT_PANEL_W - 20, line_h, COLOR_BLACK);
    if (sensors.water_alarm) {
        lcd_fill_rect(x, line_y, RIGHT_PANEL_W - 20, line_h, COLOR_DARK_RED);
        lcd_draw_string(x, line_y + 4, "Water Level: *** LEAK DETECTED ***", COLOR_WHITE, COLOR_DARK_RED);
    } else {
        snprintf(buf, sizeof(buf), "Water Level: Normal (ADC: %d)", sensors.water_raw);
        lcd_draw_string(x, line_y + 4, buf, COLOR_GREEN, COLOR_BLACK);
    }
    line_y += line_h + 2;
}

void draw_initial_ui(void)
{
    lcd_clear(COLOR_BLACK);

    draw_title_bar();

    draw_status_panel(MAIN_AREA_Y, MAIN_AREA_H);
    draw_data_panel(MAIN_AREA_Y, MAIN_AREA_H);

    /* Initial data */
    update_status_display(MAIN_AREA_Y, MAIN_AREA_H);
    update_data_display(MAIN_AREA_Y, MAIN_AREA_H);

    draw_card_info_bar();
    draw_footer();
}

/* ======================== Signal Handler ======================== */
void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* ======================== Main Application Logic ======================== */

void process_rfid(void)
{
    char card_id[MAX_CARD_LEN];
    int ret;

    ret = rfid_read_card(card_id, sizeof(card_id));
    if (ret == 1) {
        /* Card detected */
        strncpy(last_card_id, card_id, MAX_CARD_LEN - 1);
        last_card_id[MAX_CARD_LEN - 1] = '\0';
        last_card_time = time(NULL);

        last_card_verified = rfid_verify_card(card_id);
        if (last_card_verified) {
            /* Authorized: open door (relay for 5 seconds) */
            printf("Door access GRANTED for card %s\n", card_id);
            alarm_relay_on();
            alarm_indicate_door_open();
            sleep(5);
            alarm_relay_off();
        } else {
            /* Unauthorized: flash red LED, no beep */
            printf("Door access DENIED for card %s\n", card_id);
            alarm_red_led_on();
            usleep(300000);
            alarm_red_led_off();
        }
    }
}

void process_sensors(void)
{
    time_t now = time(NULL);

    /* Gas sensor - poll every cycle */
    int gas_val = gas_read();
    if (gas_val >= 0)
        sensors.gas_alarm = gas_val;

    /* Water sensor - poll every cycle */
    int water_val = water_read();
    if (water_val >= 0) {
        sensors.water_raw = water_val;
        sensors.water_alarm = (water_val >= WATER_THRESHOLD) ? 1 : 0;
    }

    /* DHT11 - poll every 3 seconds */
    if (now - last_weather_time >= 3) {
        float temp, hum;
        if (weather_read(&temp, &hum) == 0) {
            sensors.temperature = temp;
            sensors.humidity = hum;
            sensors.weather_valid = 1;
        } else {
            sensors.weather_valid = 0;
        }
        last_weather_time = now;
    }
}

void check_alarm(void)
{
    int should_alarm = (sensors.gas_alarm || sensors.water_alarm);

    if (should_alarm && !alarm_active) {
        /* Trigger alarm */
        alarm_active = 1;
        alarm_activate();
        printf("!!! ALARM TRIGGERED: gas=%d water=%d\n",
               sensors.gas_alarm, sensors.water_alarm);
    } else if (!should_alarm && alarm_active) {
        /* Clear alarm */
        alarm_active = 0;
        alarm_deactivate();
        printf("Alarm cleared: conditions normal\n");
    }
}

int main(void)
{
    struct sigaction sa;

    /* Record start time */
    start_time = time(NULL);

    /* Set up signal handler for clean shutdown */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    printf("============================================\n");
    printf("  Lab Chemical Warehouse Control System\n");
    printf("  Initializing...\n");
    printf("============================================\n");

    /* Initialize LCD */
    if (lcd_init() < 0) {
        fprintf(stderr, "FATAL: LCD init failed\n");
        return 1;
    }
    printf("LCD initialized (%dx%d)\n", lcd_get_width(), lcd_get_height());
    draw_initial_ui();

    /* Initialize modules */
    rfid_init();
    rfid_load_default_whitelist();
    gas_init();
    weather_init();
    water_init();
    alarm_init();

    printf("\nSystem running. Press Ctrl+C to exit.\n");
    printf("============================================\n\n");

    /* ======================== Main Loop ======================== */
    while (running) {
        /* Process sensors */
        process_sensors();

        /* Process RFID (non-blocking) */
        process_rfid();

        /* Check and update alarm status */
        check_alarm();

        /* Update LCD display */
        update_status_display(MAIN_AREA_Y, MAIN_AREA_H);
        update_data_display(MAIN_AREA_Y, MAIN_AREA_H);
        draw_card_info_bar();
        draw_footer();

        /* Wait before next cycle */
        usleep(100000);  /* 100ms = ~10Hz update rate */
    }

    /* ======================== Clean Shutdown ======================== */
    printf("\nShutting down...\n");

    alarm_deactivate();
    alarm_close();

    rfid_close();
    gas_close();
    weather_close();
    water_close();

    /* Clear screen and show shutdown message */
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(200, 220, "System Shutdown", COLOR_RED, COLOR_BLACK);
    lcd_draw_string(200, 240, "Please wait...", COLOR_GRAY, COLOR_BLACK);
    sleep(1);
    lcd_close();

    printf("Bye.\n");
    return 0;
}
