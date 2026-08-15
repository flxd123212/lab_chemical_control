/**
 * rfidcard.cpp - RFID刷卡模块
 *
 * 通过 UART (/dev/ttySAC1) 读取RFID卡号，
 * 与白名单比对决定是否授权。
 */
#include "rfidcard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

static int rfid_fd = -1;

/* 白名单 (用 rfid_dump 工具读取卡号后添加) */
static const char *whitelist[] = {
    "12005DE95B",   /* ← 用户卡片 */
    NULL
};

void rfid_init(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[RFID] Simulated mode - no hardware\n");
    return;
#endif
    rfid_fd = open("/dev/ttySAC1", O_RDWR | O_NOCTTY | O_NDELAY);
    if (rfid_fd < 0) {
        perror("[RFID] Failed to open /dev/ttySAC1");
        return;
    }

    struct termios options;
    tcgetattr(rfid_fd, &options);

    /* 9600 baud, 8N1 */
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag &= ~PARENB;     /* No parity */
    options.c_cflag &= ~CSTOPB;     /* 1 stop bit */
    options.c_cflag &= ~CSIZE;
    options.c_cflag |=  CS8;        /* 8 data bits */
    options.c_cflag |=  (CLOCAL | CREAD);

    /* Raw input */
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    tcsetattr(rfid_fd, TCSANOW, &options);
    tcflush(rfid_fd, TCIOFLUSH);

    printf("[RFID] /dev/ttySAC1 opened (9600 8N1)\n");
}

void rfid_close(void)
{
    if (rfid_fd >= 0) {
        close(rfid_fd);
        rfid_fd = -1;
        printf("[RFID] Closed\n");
    }
}

void rfid_load_default_whitelist(void)
{
    printf("[RFID] Whitelist loaded (%zu cards)\n",
           sizeof(whitelist) / sizeof(whitelist[0]) - 1);
}

int rfid_read_card(char *buf, int buf_size)
{
#ifdef SIMULATE_HARDWARE
    (void)buf;
    (void)buf_size;
    return 0;   /* 模拟模式: 返回无卡 */
#endif
    if (rfid_fd < 0) return -1;

    unsigned char raw[64];
    int n = read(rfid_fd, raw, sizeof(raw) - 1);
    if (n <= 0) return 0;

    raw[n] = '\0';

    /*
     * 常见RFID模块帧格式:
     *   STX(0x02) + ASCII卡号 + 校验 + ETX(0x03)
     * 或直接输出卡号字符串 + \r\n
     *
     * 两种都兼容处理:
     *   1) 0x02...0x03 帧: 提取中间ASCII部分
     *   2) 纯文本: 直接取
     */
    const unsigned char *start = raw;
    const unsigned char *end = raw + n;

    /* 如果以 0x02 开头, 找 0x03 结尾 */
    if (raw[0] == 0x02) {
        start = raw + 1;                 /* 跳过 STX */
        for (const unsigned char *p = start; p < end; p++) {
            if (*p == 0x03) {
                end = p;                 /* ETX 为止 */
                break;
            }
        }
    }

    /* 拷贝提取到的卡号 (仅保留可打印字符) */
    int out_len = 0;
    for (const unsigned char *p = start; p < end && out_len < buf_size - 1; p++) {
        if (*p >= 0x20 && *p <= 0x7E) {
            buf[out_len++] = (char)*p;
        }
    }
    buf[out_len] = '\0';

    return (out_len > 0) ? 1 : 0;
}

int rfid_verify_card(const char *card_id)
{
    for (int i = 0; whitelist[i] != NULL; i++) {
        if (strcmp(card_id, whitelist[i]) == 0) {
            return 1;   /* 授权通过 */
        }
    }
    return 0;           /* 未授权 */
}
