#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "hardwaremanager.h"

/**
 * MainWindow - 系统主界面 (800x480)
 *
 * 布局:
 *   TitleBar     (40px)    标题栏
 *   MainArea     (390px)   左:系统状态  右:传感器数据
 *   CardInfoBar  (30px)    时间 + 刷卡信息
 *   FooterBar    (20px)    运行时长 + 系统名称
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTimerTick();
    void onSensorUpdate(const SensorData &data);
    void onCardDetected(const QString &cardId, bool granted);
    void onAlarmChanged(bool active);

    /* 桌面调试按钮 */
#ifdef SIMULATE_HARDWARE
    void onDebugGasTrigger();
    void onDebugGasClear();
    void onDebugHchoTrigger();
    void onDebugHchoClear();
    void onDebugCardGrant();
    void onDebugCardReject();
    void onDebugReset();
#endif

private:
    void setupUI();
    void setupStyles();
    void setupHardware();

    /* ========== UI Widgets ========== */
    /* 左侧面板 */
    QLabel *m_labelGasStatus;       // Gas 状态
    QLabel *m_labelToxicStatus;     // Toxic 状态
    QLabel *m_labelGateStatus;      // Gate 状态
    QLabel *m_labelAlarmStatus;     // 总报警状态

    /* 右侧面板 */
    QLabel *m_labelTemperature;     // 温度值
    QLabel *m_labelHumidity;        // 湿度值
    QLabel *m_labelGasReading;      // 气体读数
    QLabel *m_labelHchoReading;     // HCHO 读数
    QLabel *m_labelCo2Reading;      // CO2 读数

    /* 底部 */
    QLabel *m_labelClock;           // 时间
    QLabel *m_labelCardInfo;        // 刷卡信息
    QLabel *m_labelUptime;          // 运行时长

    /* Timer */
    QTimer *m_timer;                // 主循环定时器 (100ms)

    /* 硬件 */
    HardwareManager *m_hw;
    QDateTime m_startTime;
};

#endif // MAINWINDOW_H
