#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <QObject>
#include <QTimer>

/* 传感器数据结构 */
struct SensorData {
    float temperature = 0;
    float humidity    = 0;
    int   gasAlarm   = 0;   // 0=正常 1=报警
    int   hchoAlarm  = 0;   // 0=正常 1=报警
    float hcho       = 0;   // HCHO浓度 (ppm)
    float co2        = 0;   // CO2浓度 (ppm)
};

/**
 * HardwareManager - 硬件管理类
 *
 * 封装所有传感器和RFID的轮询逻辑，通过信号通知UI更新。
 * 使用 #ifdef SIMULATE_HARDWARE 支持桌面模拟调试。
 */
class HardwareManager : public QObject
{
    Q_OBJECT

public:
    explicit HardwareManager(QObject *parent = nullptr);
    ~HardwareManager();

    void init();
    void shutdown();
    void poll();            // 主循环轮询 (由QTimer每100ms调用)

    /* ---- 桌面模拟触发器 (仅在 SIMULATE_HARDWARE 生效) ---- */
#ifdef SIMULATE_HARDWARE
    void simTriggerGas();
    void simClearGas();
    void simTriggerHcho();
    void simClearHcho();
    void simSwipeCard(const QString &cardId, bool granted);
    void simResetAll();
#endif

signals:
    void sensorUpdated(const SensorData &data);
    void cardDetected(const QString &cardId, bool granted);
    void alarmChanged(bool active);

private:
    void pollSensors();
    void pollRfid();
    void checkAlarm();

#ifdef SIMULATE_HARDWARE
    /* 桌面模拟：用定时器模拟传感器变化 */
    QTimer *m_simTimer;
    float   m_simTemp  = 25.0;
    float   m_simHum   = 60.0;
    float   m_simHcho  = 0.05;  // HCHO模拟值 (ppm)
    float   m_simCo2   = 400.0; // CO2模拟值 (ppm)
#endif

    SensorData m_data;
    int        m_alarmActive    = 0;
    int        m_prevGasAlarm   = -1;
    int        m_prevHchoAlarm = -1;
    int        m_pollCount      = 0;
};

#endif // HARDWAREMANAGER_H
