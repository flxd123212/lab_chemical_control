/**
 * hardwaremanager.cpp - 硬件管理实现
 */
#include "hardwaremanager.h"
#include "rfidcard.h"
#include "gasmonitor.h"
#include "weather.h"
#include "voc21sensor.h"
#include "alarm.h"

#include <QDebug>
#include <QDateTime>

HardwareManager::HardwareManager(QObject *parent)
    : QObject(parent)
{
#ifdef SIMULATE_HARDWARE
    m_simTimer = new QTimer(this);
    connect(m_simTimer, &QTimer::timeout, [this]() {
        /* 模拟传感器轻微波动 */
        m_simTemp += (qrand() % 10 - 5) * 0.1;
        m_simHum  += (qrand() % 10 - 5) * 0.2;
        m_simHcho += (qrand() % 10 - 5) * 0.01f;
        m_simCo2  += (qrand() % 20 - 10) * 10.0f;
        if (m_simHcho < 0.0f)   m_simHcho = 0.0f;
        if (m_simHcho > 1.0f)   m_simHcho = 1.0f;
        if (m_simCo2 < 300.0f)  m_simCo2 = 300.0f;
        if (m_simCo2 > 5000.0f) m_simCo2 = 5000.0f;
    });
    m_simTimer->start(500);
#endif
}

HardwareManager::~HardwareManager()
{
}

void HardwareManager::init()
{
    qDebug() << "[HW] Initializing hardware...";

#ifndef SIMULATE_HARDWARE
    rfid_init();
    rfid_load_default_whitelist();
    gas_init();
    weather_init();
    voc21_init();
    alarm_init();
#endif

    qDebug() << "[HW] All modules initialized.";
}

/* ======================== 桌面模拟触发器 ======================== */
#ifdef SIMULATE_HARDWARE
void HardwareManager::simTriggerGas()
{
    qDebug() << "[SIM] Gas alarm triggered!";
    m_data.gasAlarm = 1;
    emit sensorUpdated(m_data);
    checkAlarm();
}

void HardwareManager::simClearGas()
{
    qDebug() << "[SIM] Gas alarm cleared";
    m_data.gasAlarm = 0;
    emit sensorUpdated(m_data);
    checkAlarm();
}

void HardwareManager::simTriggerHcho()
{
    qDebug() << "[SIM] HCHO alarm triggered!";
    m_simHcho = 950;  // 超过阈值
    m_data.hcho      = m_simHcho;
    m_data.hchoAlarm = 1;
    m_data.co2       = m_simCo2;
    emit sensorUpdated(m_data);
    checkAlarm();
}

void HardwareManager::simClearHcho()
{
    qDebug() << "[SIM] HCHO alarm cleared";
    m_simHcho = 120;
    m_data.hcho      = m_simHcho;
    m_data.hchoAlarm = 0;
    m_data.co2       = m_simCo2;
    emit sensorUpdated(m_data);
    checkAlarm();
}

void HardwareManager::simSwipeCard(const QString &cardId, bool granted)
{
    qDebug() << "[SIM] Card swipe:" << cardId << (granted ? "GRANTED" : "REJECTED");
    emit cardDetected(cardId, granted);

    if (granted) {
        qDebug() << "[SIM] Door opened (5s auto-close)";
        // 模拟5秒后自动关门
        QTimer::singleShot(5000, [this]() {
            qDebug() << "[SIM] Door auto-closed";
        });
    }
}

void HardwareManager::simResetAll()
{
    qDebug() << "[SIM] Reset all sensors";
    m_data.gasAlarm   = 0;
    m_data.hchoAlarm  = 0;
    m_data.hcho       = 120;
    m_data.co2        = 400;
    m_simHcho = 120;
    m_simCo2  = 400;
    m_simTemp = 25.0;
    m_simHum  = 60.0;
    emit sensorUpdated(m_data);
    checkAlarm();
}
#endif

void HardwareManager::shutdown()
{
    qDebug() << "[HW] Shutting down...";

#ifndef SIMULATE_HARDWARE
    alarm_deactivate();
    alarm_close();
    rfid_close();
    gas_close();
    weather_close();
    voc21_close();
#endif

    qDebug() << "[HW] Shutdown complete.";
}

void HardwareManager::poll()
{
    pollSensors();
    pollRfid();
    checkAlarm();
}

// ======================== 传感器轮询 ========================
void HardwareManager::pollSensors()
{
    SensorData newData;

#ifdef SIMULATE_HARDWARE
    /* 桌面模拟 */
    newData.temperature = m_simTemp;
    newData.humidity    = m_simHum;
    newData.gasAlarm    = m_data.gasAlarm;     // 保持报警状态
    newData.hchoAlarm   = m_data.hchoAlarm;
    newData.hcho        = m_simHcho;
    newData.co2         = m_simCo2;
#else
    /* 真实硬件 */
    int gasVal = gas_read();
    if (gasVal >= 0)
        newData.gasAlarm = gasVal;
    else
        newData.gasAlarm = m_data.gasAlarm;    // 保持上次值

    /* 21VOC 传感器 (HCHO + CO2) */
    float hchoVal, co2Val;
    if (voc21_read(&hchoVal, &co2Val) == 0) {
        newData.hcho = hchoVal;
        newData.co2  = co2Val;
        newData.hchoAlarm = (hchoVal >= HCHO_THRESHOLD) ? 1 : 0;
    } else {
        newData.hcho      = m_data.hcho;
        newData.co2       = m_data.co2;
        newData.hchoAlarm = m_data.hchoAlarm;
    }

    /* DHT11 每3秒读一次 */
    static int lastWeatherTime = 0;
    int now = QDateTime::currentDateTime().toSecsSinceEpoch();
    if (now - lastWeatherTime >= 3) {
        float temp, hum;
        if (weather_read(&temp, &hum) == 0) {
            newData.temperature = temp;
            newData.humidity    = hum;
        } else {
            newData.temperature = m_data.temperature;
            newData.humidity    = m_data.humidity;
        }
        lastWeatherTime = now;
    } else {
        newData.temperature = m_data.temperature;
        newData.humidity    = m_data.humidity;
    }
#endif

    /* 检查是否有变化 */
    bool changed = (newData.gasAlarm   != m_data.gasAlarm)   ||
                   (newData.hchoAlarm  != m_data.hchoAlarm)  ||
                   (qAbs(newData.temperature - m_data.temperature) > 0.2f) ||
                   (qAbs(newData.humidity    - m_data.humidity)    > 0.5f);

    m_data = newData;

    if (changed) {
        emit sensorUpdated(m_data);
    }
}

// ======================== RFID轮询 ========================
void HardwareManager::pollRfid()
{
#ifdef SIMULATE_HARDWARE
    /* 桌面模拟不主动刷卡，由外部调用模拟 */
    Q_UNUSED(pollRfid);
#else
    char card_id[MAX_CARD_LEN];
    int ret = rfid_read_card(card_id, sizeof(card_id));
    if (ret == 1) {
        bool granted = rfid_verify_card(card_id);

        qDebug() << "[RFID] Card:" << card_id << (granted ? "GRANTED" : "REJECTED");
        emit cardDetected(QString(card_id), granted);

        if (granted) {
            alarm_relay_on();
            alarm_indicate_door_open();
            // 5秒后自动关门
            QTimer::singleShot(5000, [this]() {
                alarm_relay_off();
            });
        } else {
            alarm_red_led_on();
            QTimer::singleShot(300, [this]() {
                alarm_red_led_off();
            });
        }
    }
#endif
}

// ======================== 报警逻辑 ========================
void HardwareManager::checkAlarm()
{
    int shouldAlarm = (m_data.gasAlarm || m_data.hchoAlarm);

    if (shouldAlarm && !m_alarmActive) {
        m_alarmActive = 1;
        qDebug() << "[ALARM] TRIGGERED! gas=" << m_data.gasAlarm << "hcho=" << m_data.hchoAlarm;
#ifndef SIMULATE_HARDWARE
        alarm_activate();
#endif
        emit alarmChanged(true);

    } else if (!shouldAlarm && m_alarmActive) {
        m_alarmActive = 0;
        qDebug() << "[ALARM] Cleared.";
#ifndef SIMULATE_HARDWARE
        alarm_deactivate();
#endif
        emit alarmChanged(false);
    }
}
