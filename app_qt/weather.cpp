/**
 * weather.cpp - DHT11温湿度传感器 (UART)
 *
 * 通过 UART (/dev/ttySAC3, 9600 8N1) 读取 DHT11 模块数据，
 * 支持常见的文本输出格式:
 *   "T:25.3 H:60.2\r\n"
 *   "T25.3H60.2\r\n"
 *   "Temp:25.3 Hum:60.2\r\n"
 */
#include "weather.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define DHT11_DEVICE  "/dev/ttySAC3"
#define DHT11_BAUD    B9600

static int weather_fd = -1;

/* ---------- UART 初始化 ---------- */
static int uart_open(const char *device, speed_t baud)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) return -1;

    struct termios opt;
    tcgetattr(fd, &opt);

    cfsetispeed(&opt, baud);
    cfsetospeed(&opt, baud);

    /* 8N1: 8数据位, 无校验, 1停止位 */
    opt.c_cflag &= ~PARENB;       /* 无校验 */
    opt.c_cflag &= ~CSTOPB;       /* 1停止位 */
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |=  CS8;          /* 8数据位 */

    /* 关闭硬件流控 */
    opt.c_cflag &= ~CRTSCTS;

    /* 本地连接, 启用接收 */
    opt.c_cflag |=  (CLOCAL | CREAD);

    /* 原始输入模式 */
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* 原始输出模式 */
    opt.c_oflag &= ~OPOST;

    /* 按字节读取, 非规范模式 */
    opt.c_cc[VMIN]  = 0;          /* 非阻塞 */
    opt.c_cc[VTIME] = 10;         /* 1秒超时 (10*100ms) */

    tcsetattr(fd, TCSANOW, &opt);
    tcflush(fd, TCIOFLUSH);

    return fd;
}

/* ---------- DHT11 文本解析 ---------- */
/*
 * 在原始行数据中搜索温度/湿度值。
 * 支持格式举例:
 *   T25.3H60.2
 *   T:25.3 H:60.2
 *   Temp:25.3 Hum:60.2
 *   温度:25.3 湿度:60.2
 *
 * 返回值: 0=成功, -1=解析失败
 */
static int parse_dht11_line(const char *line, float *temperature, float *humidity)
{
    float t = 0.0f, h = 0.0f;
    int   got_t = 0, got_h = 0;

    /* 尝试匹配 "Txx.x" 或 "T:xx.x" 或 "Temp:xx.x" */
    const char *p = line;
    while (*p) {
        if ((p[0] == 'T' || p[0] == 't') &&
            (p[1] == ':' || p[1] == '=' || (p[1] >= '0' && p[1] <= '9'))) {
            const char *val = p + 1;
            if (*val == ':' || *val == '=') val++;
            if (sscanf(val, "%f", &t) == 1) {
                got_t = 1;
            }
        }

        if ((p[0] == 'H' || p[0] == 'h') &&
            (p[1] == ':' || p[1] == '=' || (p[1] >= '0' && p[1] <= '9'))) {
            const char *val = p + 1;
            if (*val == ':' || *val == '=') val++;
            if (sscanf(val, "%f", &h) == 1) {
                got_h = 1;
            }
        }

        /* 尝试匹配中文 "温度:" / "湿度:" */
        if (strncmp(p, "温度:", 6) == 0) {
            if (sscanf(p + 6, "%f", &t) == 1) got_t = 1;
        }
        if (strncmp(p, "湿度:", 6) == 0) {
            if (sscanf(p + 6, "%f", &h) == 1) got_h = 1;
        }

        /* 尝试匹配 "Temp:" / "Hum:" */
        if (strncasecmp(p, "Temp:", 5) == 0) {
            if (sscanf(p + 5, "%f", &t) == 1) got_t = 1;
        }
        if (strncasecmp(p, "Hum:", 4) == 0) {
            if (sscanf(p + 4, "%f", &h) == 1) got_h = 1;
        }

        p++;
    }

    if (!got_t || !got_h) return -1;

    /* 合理性校验 */
    if (t < -40.0f || t > 80.0f)   return -1;
    if (h < 0.0f   || h > 100.0f)  return -1;

    *temperature = t;
    *humidity    = h;
    return 0;
}

/* ---------- 公开接口 ---------- */

void weather_init(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[WEATHER] Simulated mode\n");
    return;
#endif
    weather_fd = uart_open(DHT11_DEVICE, DHT11_BAUD);
    if (weather_fd < 0) {
        perror("[WEATHER] Failed to open " DHT11_DEVICE);
        return;
    }
    printf("[WEATHER] %s opened (9600 8N1)\n", DHT11_DEVICE);
}

void weather_close(void)
{
    if (weather_fd >= 0) {
        close(weather_fd);
        weather_fd = -1;
    }
}

int weather_read(float *temperature, float *humidity)
{
#ifdef SIMULATE_HARDWARE
    /* 模拟值已由 HardwareManager 生成 */
    return -1;
#endif
    if (weather_fd < 0) return -1;

    char buf[128];
    int  pos = 0;
    int  retries = 3;

    while (retries--) {
        /* 逐字节读取，直到遇到换行或缓冲区满 */
        while (pos < (int)sizeof(buf) - 1) {
            char ch;
            int n = read(weather_fd, &ch, 1);
            if (n <= 0) break;   /* 超时或无数据 */

            if (ch == '\n' || ch == '\r') {
                if (pos > 0) {   /* 一行结束 */
                    buf[pos] = '\0';
                    if (parse_dht11_line(buf, temperature, humidity) == 0)
                        return 0;
                    /* 解析失败，继续读下一行 */
                    pos = 0;
                }
                continue;
            }

            buf[pos++] = ch;
        }

        /* 缓冲区满或没有更多数据，尝试解析已有内容 */
        if (pos > 0) {
            buf[pos] = '\0';
            if (parse_dht11_line(buf, temperature, humidity) == 0)
                return 0;
        }

        /* 再等一会儿重试 */
        usleep(100000); /* 100ms */
        pos = 0;
    }

    return -1; /* 多次重试后仍失败 */
}
