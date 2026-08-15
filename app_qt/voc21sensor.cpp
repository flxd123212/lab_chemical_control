/**
 * voc21sensor.cpp - 21VOC五合一空气质量传感器实现
 *
 * 通过 UART2 (/dev/ttySAC2) 读取甲醛(HCHO)和CO2浓度。
 * 主动上报模式, 9600 8N1, 每帧9字节。
 *
 * 协议解析在 voc21_parse_frame(), 可根据实际传感器修改。
 */
#include "voc21sensor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

static int voc21_fd = -1;

/* 环形缓冲区 */
#define VOC21_BUF_SIZE  64
static unsigned char voc21_buf[VOC21_BUF_SIZE];
static int            voc21_buf_head = 0;
static int            voc21_buf_tail = 0;

/* ──────────────────────────────────────────
 * 协议解析: 从环形缓冲区找有效帧, 提取 HCHO 和 CO2
 *
 * 期望帧格式:
 *   AA 15 HCHO_L HCHO_H CO2_L CO2_H TVOC_L TVOC_H CS
 *
 * 返回: 1=解析成功, 0=未找到完整帧, -1=帧校验错误
 * ────────────────────────────────────────── */
static int voc21_parse_frame(float *hcho, float *co2)
{
    int buf_len = (voc21_buf_head >= voc21_buf_tail)
                      ? (voc21_buf_head - voc21_buf_tail)
                      : (VOC21_BUF_SIZE - voc21_buf_tail + voc21_buf_head);

    while (buf_len >= VOC21_FRAME_LEN) {
        /* 找帧头 0xAA */
        int found = 0;
        for (int i = 0; i < buf_len; i++) {
            int idx = (voc21_buf_tail + i) % VOC21_BUF_SIZE;
            if (voc21_buf[idx] == 0xAA) {
                found = i;
                break;
            }
        }
        if (buf_len - found < VOC21_FRAME_LEN) {
            /* 剩余不够一帧, 等下次 */
            return 0;
        }

        /* 从 found 处开始解析 */
        int start = (voc21_buf_tail + found) % VOC21_BUF_SIZE;

        unsigned char frame[VOC21_FRAME_LEN];
        for (int i = 0; i < VOC21_FRAME_LEN; i++) {
            frame[i] = voc21_buf[(start + i) % VOC21_BUF_SIZE];
        }

        /* 校验 */
        unsigned char sum = 0;
        for (int i = 0; i < VOC21_FRAME_LEN - 1; i++)
            sum += frame[i];
        if (sum != frame[VOC21_FRAME_LEN - 1]) {
            /* 校验失败, 跳过这个字节继续找 */
            voc21_buf_tail = (start + 1) % VOC21_BUF_SIZE;
            buf_len = (voc21_buf_head >= voc21_buf_tail)
                          ? (voc21_buf_head - voc21_buf_tail)
                          : (VOC21_BUF_SIZE - voc21_buf_tail + voc21_buf_head);
            continue;
        }

        /* 解析 HCHO = (byte3<<8 | byte2) / 100.0 mg/m³ */
        unsigned int hcho_raw = (unsigned int)(frame[3] << 8) | frame[2];
        *hcho = (float)hcho_raw / 100.0f;

        /* 解析 CO2 = (byte5<<8 | byte4) ppm */
        unsigned int co2_raw = (unsigned int)(frame[5] << 8) | frame[4];
        *co2 = (float)co2_raw;

        /* 移除已解析帧 */
        voc21_buf_tail = (start + VOC21_FRAME_LEN) % VOC21_BUF_SIZE;

        return 1;
    }

    return 0;
}

void voc21_init(void)
{
#ifdef SIMULATE_HARDWARE
    printf("[VOC21] Simulated mode\n");
    return;
#endif
    voc21_fd = open(VOC21_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (voc21_fd < 0) {
        perror("[VOC21] Failed to open " VOC21_DEVICE);
        return;
    }

    struct termios options;
    tcgetattr(voc21_fd, &options);

    cfsetispeed(&options, VOC21_BAUD);
    cfsetospeed(&options, VOC21_BAUD);
    options.c_cflag &= ~PARENB;     /* No parity */
    options.c_cflag &= ~CSTOPB;     /* 1 stop bit */
    options.c_cflag &= ~CSIZE;
    options.c_cflag |=  CS8;        /* 8 data bits */
    options.c_cflag |=  (CLOCAL | CREAD);

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    /* VMIN=0, VTIME=0 -> 非阻塞读 */
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 0;

    tcsetattr(voc21_fd, TCSANOW, &options);
    tcflush(voc21_fd, TCIOFLUSH);

    printf("[VOC21] %s opened (9600 8N1)\n", VOC21_DEVICE);
}

void voc21_close(void)
{
    if (voc21_fd >= 0) {
        close(voc21_fd);
        voc21_fd = -1;
        printf("[VOC21] Closed\n");
    }
}

int voc21_read(float *hcho, float *co2)
{
#ifdef SIMULATE_HARDWARE
    /* 模拟值由 HardwareManager 生成 */
    (void)hcho;
    (void)co2;
    return -1;
#endif
    if (voc21_fd < 0) return -1;

    /* 读UART数据入环形缓冲区 */
    unsigned char tmp[32];
    int n = read(voc21_fd, tmp, sizeof(tmp));
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            voc21_buf[voc21_buf_head] = tmp[i];
            voc21_buf_head = (voc21_buf_head + 1) % VOC21_BUF_SIZE;
            if (voc21_buf_head == voc21_buf_tail) {
                /* 缓冲区满, 丢弃最旧数据 */
                voc21_buf_tail = (voc21_buf_tail + 1) % VOC21_BUF_SIZE;
            }
        }
    }

    /* 尝试解析 */
    return voc21_parse_frame(hcho, co2);
}
