/**
 * mainwindow.cpp - 主界面实现
 */
#include "mainwindow.h"

#include <QApplication>
#include <QDateTime>
#include <QFont>
#include <QDebug>

// ======================== 颜色常量 ========================
static const char *COLOR_NAVY       = "#000080";
static const char *COLOR_BLUE       = "#0000ff";
static const char *COLOR_DARK_BLUE  = "#00008b";
static const char *COLOR_DARK_GRAY  = "#555555";
static const char *COLOR_BLACK      = "#000000";
static const char *COLOR_WHITE      = "#ffffff";
static const char *COLOR_GREEN      = "#00ff00";
static const char *COLOR_RED        = "#ff0000";
static const char *COLOR_CYAN       = "#00ffff";
static const char *COLOR_DARK_RED   = "#8b0000";
static const char *COLOR_GRAY       = "#888888";
static const char *COLOR_LIGHT_GRAY = "#aaaaaa";

// ======================== 构造函数 ========================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_timer(new QTimer(this))
    , m_hw(nullptr)
{
    setFixedSize(800, 480);
    setWindowTitle("Lab Chemical Control System");
    setWindowFlags(Qt::FramelessWindowHint);  // 全屏无边框

    m_startTime = QDateTime::currentDateTime();

    setupStyles();
    setupUI();
    setupHardware();

    /* 主循环定时器 100ms = 10Hz */
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTimerTick);
    m_timer->start(100);
}

MainWindow::~MainWindow()
{
    if (m_hw) {
        m_hw->shutdown();
        delete m_hw;
    }
}

// ======================== 界面样式 ========================
void MainWindow::setupStyles()
{
    qApp->setStyleSheet(
        "QMainWindow { background: black; }"
        "QLabel { background: transparent; }"
    );
}

// ======================== 构建界面 ========================
void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    /* ---- Title Bar (40px) ---- */
    QFrame *titleBar = new QFrame;
    titleBar->setFixedHeight(40);
    titleBar->setStyleSheet(
        QString("background:%1; border-bottom:1px solid %2;")
            .arg(COLOR_NAVY, COLOR_BLUE));
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 10, 0);

    QLabel *titleLabel = new QLabel("Lab Chemical Warehouse Env Control System v1.0");
    titleLabel->setStyleSheet(QString("color:%1; font-size:16px; font-weight:bold;").arg(COLOR_WHITE));
    titleLayout->addWidget(titleLabel);
    mainLayout->addWidget(titleBar);

    /* ---- Main Area (390px / 350px with debug) ---- */
    QFrame *mainArea = new QFrame;
#ifdef SIMULATE_HARDWARE
    mainArea->setFixedHeight(350);
#else
    mainArea->setFixedHeight(390);
#endif
    QHBoxLayout *mainHLayout = new QHBoxLayout(mainArea);
    mainHLayout->setContentsMargins(2, 0, 2, 0);
    mainHLayout->setSpacing(2);

    /* -- Left Panel: System Status (280px) -- */
    QFrame *leftPanel = new QFrame;
    leftPanel->setFixedWidth(280);
    leftPanel->setStyleSheet(QString("background:%1; border:1px solid %2;")
                                .arg(COLOR_BLACK, COLOR_DARK_GRAY));
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    // Header
    QLabel *leftHeader = new QLabel("  System Status");
    leftHeader->setFixedHeight(22);
    leftHeader->setStyleSheet(QString("background:%1; color:%2; font-weight:bold; font-size:13px;")
                                 .arg(COLOR_DARK_BLUE, COLOR_WHITE));
    leftLayout->addWidget(leftHeader);

    // Status rows
    QVBoxLayout *statusLayout = new QVBoxLayout;
    statusLayout->setContentsMargins(10, 6, 10, 6);
    statusLayout->setSpacing(4);

    auto makeStatusRow = [&](const QString &label) -> QLabel* {
        QLabel *lbl = new QLabel(label);
        lbl->setFixedHeight(22);
        lbl->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_LIGHT_GRAY));
        statusLayout->addWidget(lbl);
        return lbl;
    };
    m_labelGasStatus     = makeStatusRow("Gas:   ✔ Normal");
    m_labelToxicStatus   = makeStatusRow("Toxic: ✔ Normal");
    m_labelGateStatus    = makeStatusRow("Gate:  ✔ Locked");

    // Alarm row (特殊: 可切换红色背景)
    m_labelAlarmStatus = new QLabel("Status: ✔ All Normal");
    m_labelAlarmStatus->setFixedHeight(28);
    m_labelAlarmStatus->setAlignment(Qt::AlignCenter);
    m_labelAlarmStatus->setStyleSheet(
        QString("background:black; color:%1; font-size:14px; font-weight:bold; border-radius:2px;")
            .arg(COLOR_GREEN));
    statusLayout->addWidget(m_labelAlarmStatus);

    leftLayout->addLayout(statusLayout);
    mainHLayout->addWidget(leftPanel);

    /* -- Right Panel: Sensor Monitor (剩余宽度) -- */
    QFrame *rightPanel = new QFrame;
    rightPanel->setStyleSheet(QString("background:%1; border:1px solid %2;")
                                 .arg(COLOR_BLACK, COLOR_DARK_GRAY));
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Header
    QLabel *rightHeader = new QLabel("  Sensor Monitor");
    rightHeader->setFixedHeight(22);
    rightHeader->setStyleSheet(QString("background:%1; color:%2; font-weight:bold; font-size:13px;")
                                  .arg(COLOR_DARK_BLUE, COLOR_WHITE));
    rightLayout->addWidget(rightHeader);

    // Sensor data rows
    QVBoxLayout *sensorLayout = new QVBoxLayout;
    sensorLayout->setContentsMargins(12, 6, 12, 6);
    sensorLayout->setSpacing(6);

    auto makeSensorRow = [&](const QString &label) -> QLabel* {
        QLabel *lbl = new QLabel(label);
        lbl->setFixedHeight(26);
        lbl->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_CYAN));
        sensorLayout->addWidget(lbl);
        return lbl;
    };
    m_labelTemperature = makeSensorRow("Temperature:  25.3 ℃");
    m_labelHumidity    = makeSensorRow("Humidity:     60.5 %");
    m_labelGasReading  = makeSensorRow("Gas/Smoke:   Clear (Normal)");
    m_labelGasReading->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GREEN));
    m_labelHchoReading = makeSensorRow("HCHO (甲醛): 0.03 mg/m³");
    m_labelHchoReading->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GREEN));
    m_labelCo2Reading  = makeSensorRow("CO2 (二氧化碳): 400 ppm");
    m_labelCo2Reading->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GREEN));

    rightLayout->addLayout(sensorLayout);
    mainHLayout->addWidget(rightPanel);
    mainLayout->addWidget(mainArea);

    /* ---- Card Info Bar (30px) ---- */
    QFrame *cardBar = new QFrame;
    cardBar->setFixedHeight(30);
    cardBar->setStyleSheet(QString("background:%1; border-top:1px solid %2;")
                              .arg(COLOR_DARK_GRAY, COLOR_GRAY));
    QHBoxLayout *cardLayout = new QHBoxLayout(cardBar);
    cardLayout->setContentsMargins(10, 0, 10, 0);

    m_labelClock = new QLabel();
    m_labelClock->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_WHITE));
    cardLayout->addWidget(m_labelClock);

    m_labelCardInfo = new QLabel("Card: -- No card read --");
    m_labelCardInfo->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GRAY));
    cardLayout->addWidget(m_labelCardInfo);
    cardLayout->addStretch();
    mainLayout->addWidget(cardBar);

    /* ---- Footer Bar (20px) ---- */
    QFrame *footerBar = new QFrame;
    footerBar->setFixedHeight(20);
    footerBar->setStyleSheet(QString("background:%1;").arg(COLOR_NAVY));
    QHBoxLayout *footerLayout = new QHBoxLayout(footerBar);
    footerLayout->setContentsMargins(10, 0, 10, 0);

    m_labelUptime = new QLabel("Uptime: 00:00:00");
    m_labelUptime->setStyleSheet(QString("color:%1; font-size:11px;").arg(COLOR_LIGHT_GRAY));
    footerLayout->addWidget(m_labelUptime);

    QLabel *footerSys = new QLabel("GEC6818 - Hazard Chemical Control");
    footerSys->setStyleSheet(QString("color:%1; font-size:11px;").arg(COLOR_LIGHT_GRAY));
    footerLayout->addWidget(footerSys);
    mainLayout->addWidget(footerBar);

#ifdef SIMULATE_HARDWARE
    /* ---- Debug Toolbar (40px) 桌面模拟调试按钮 ---- */
    QFrame *debugBar = new QFrame;
    debugBar->setFixedHeight(40);
    debugBar->setStyleSheet(
        QString("background:%1; border-top:1px solid %2;")
            .arg("#1a1a2e", COLOR_DARK_GRAY));
    QHBoxLayout *debugLayout = new QHBoxLayout(debugBar);
    debugLayout->setContentsMargins(4, 2, 4, 2);
    debugLayout->setSpacing(4);

    /* 按钮通用样式 */
    QString btnStyle = QString(
        "QPushButton {"
        "  color: white; background: #333366; border:1px solid #5555aa;"
        "  font-size:11px; font-weight:bold; padding:1px 6px; border-radius:2px;"
        "  min-height:22px;"
        "}"
        "QPushButton:hover { background: #444488; }"
        "QPushButton:pressed { background: #222244; }"
    );
    QString btnRedStyle = QString(
        "QPushButton {"
        "  color: white; background: #660000; border:1px solid #aa0000;"
        "  font-size:11px; font-weight:bold; padding:1px 6px; border-radius:2px;"
        "  min-height:22px;"
        "}"
        "QPushButton:hover { background: #880000; }"
        "QPushButton:pressed { background: #440000; }"
    );
    QString btnGreenStyle = QString(
        "QPushButton {"
        "  color: white; background: #006600; border:1px solid #00aa00;"
        "  font-size:11px; font-weight:bold; padding:1px 6px; border-radius:2px;"
        "  min-height:22px;"
        "}"
        "QPushButton:hover { background: #008800; }"
        "QPushButton:pressed { background: #004400; }"
    );

    auto makeBtn = [&](const QString &text, const char *tip, const QString &style) -> QPushButton* {
        QPushButton *btn = new QPushButton(text);
        btn->setToolTip(tip);
        btn->setStyleSheet(style);
        debugLayout->addWidget(btn);
        return btn;
    };

    QPushButton *btnGasTrig  = makeBtn("Gas Alarm",       "模拟气体泄漏触发",  btnRedStyle);
    QPushButton *btnGasClr   = makeBtn("Gas Clear",       "清除气体报警",     btnGreenStyle);
    QPushButton *btnHchoTrig = makeBtn("HCHO Alarm",      "模拟HCHO超标触发",  btnRedStyle);
    QPushButton *btnHchoClr  = makeBtn("HCHO Clear",      "清除HCHO报警",     btnGreenStyle);
    QPushButton *btnCardOk   = makeBtn("Card Grant",      "模拟授权卡刷卡",   btnGreenStyle);
    QPushButton *btnCardNo   = makeBtn("Card Reject",     "模拟未授权卡刷卡",  btnRedStyle);
    QPushButton *btnReset    = makeBtn("Reset All",       "重置所有传感器",   btnStyle);

    debugLayout->addStretch();

    /* 标签 */
    QLabel *debugLabel = new QLabel("🔧 DEBUG");
    debugLabel->setStyleSheet(QString("color:%1; font-size:10px; font-weight:bold;").arg(COLOR_GRAY));
    debugLayout->addWidget(debugLabel);

    connect(btnGasTrig, &QPushButton::clicked, this, &MainWindow::onDebugGasTrigger);
    connect(btnGasClr,  &QPushButton::clicked, this, &MainWindow::onDebugGasClear);
    connect(btnHchoTrig, &QPushButton::clicked, this, &MainWindow::onDebugHchoTrigger);
    connect(btnHchoClr,  &QPushButton::clicked, this, &MainWindow::onDebugHchoClear);
    connect(btnCardOk,  &QPushButton::clicked, this, &MainWindow::onDebugCardGrant);
    connect(btnCardNo,  &QPushButton::clicked, this, &MainWindow::onDebugCardReject);
    connect(btnReset,   &QPushButton::clicked, this, &MainWindow::onDebugReset);

    mainLayout->addWidget(debugBar);
#endif
}

// ======================== 硬件初始化 ========================
void MainWindow::setupHardware()
{
    m_hw = new HardwareManager(this);

    connect(m_hw, &HardwareManager::sensorUpdated,
            this, &MainWindow::onSensorUpdate);
    connect(m_hw, &HardwareManager::cardDetected,
            this, &MainWindow::onCardDetected);
    connect(m_hw, &HardwareManager::alarmChanged,
            this, &MainWindow::onAlarmChanged);

    m_hw->init();
}

// ======================== 定时器回调 (10Hz) ========================
void MainWindow::onTimerTick()
{
    /* 更新时间和运行时长 */
    QDateTime now = QDateTime::currentDateTime();
    m_labelClock->setText(now.toString("yyyy-MM-dd hh:mm:ss"));

    int secs = m_startTime.secsTo(now);
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = secs % 60;
    m_labelUptime->setText(QString("Uptime: %1:%2:%3")
                               .arg(h, 2, 10, QChar('0'))
                               .arg(m, 2, 10, QChar('0'))
                               .arg(s, 2, 10, QChar('0')));

    /* 触发硬件轮询 */
    m_hw->poll();
}

// ======================== 传感器数据更新 ========================
void MainWindow::onSensorUpdate(const SensorData &d)
{
    /* ---- 左侧面板 ---- */
    // Gas
    if (d.gasAlarm) {
        m_labelGasStatus->setText("Gas:   ALARM! (leak detected)");
        m_labelGasStatus->setStyleSheet(QString("color:%1; font-size:13px; font-weight:bold;")
                                            .arg(COLOR_RED));
    } else {
        m_labelGasStatus->setText("Gas:   ✔ Normal");
        m_labelGasStatus->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GREEN));
    }

    // HCHO / Toxic Gas
    if (d.hchoAlarm) {
        m_labelToxicStatus->setText("HCHO:  ALARM! (toxic gas detected)");
        m_labelToxicStatus->setStyleSheet(QString("color:%1; font-size:13px; font-weight:bold;")
                                              .arg(COLOR_RED));
    } else {
        m_labelToxicStatus->setText(QString("HCHO:  ✔ Normal (%1)").arg(d.hcho, 0, 'f', 2));
        m_labelToxicStatus->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GREEN));
    }

    // Gate
    // (刷卡事件更新)

    /* ---- 右侧面板 ---- */
    m_labelTemperature->setText(QString("Temperature:  %1 ℃").arg(d.temperature, 5, 'f', 1));

    m_labelHumidity->setText(QString("Humidity:     %1 %").arg(d.humidity, 5, 'f', 1));

    if (d.gasAlarm) {
        m_labelGasReading->setText("Gas/Smoke:   *** DETECTED ***");
        m_labelGasReading->setStyleSheet(
            QString("background:%1; color:%2; font-size:13px; font-weight:bold;")
                .arg(COLOR_DARK_RED, COLOR_WHITE));
    } else {
        m_labelGasReading->setText("Gas/Smoke:   Clear (Normal)");
        m_labelGasReading->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GREEN));
    }

    /* ---- HCHO + CO2 ---- */
    if (d.hchoAlarm) {
        m_labelHchoReading->setText(QString("HCHO (甲醛): *** %1 mg/m³ ***").arg(d.hcho, 0, 'f', 2));
        m_labelHchoReading->setStyleSheet(
            QString("background:%1; color:%2; font-size:13px; font-weight:bold;")
                .arg(COLOR_DARK_RED, COLOR_WHITE));
    } else {
        m_labelHchoReading->setText(QString("HCHO (甲醛): %1 mg/m³").arg(d.hcho, 0, 'f', 2));
        m_labelHchoReading->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GREEN));
    }
    m_labelCo2Reading->setText(QString("CO2 (二氧化碳): %1 ppm").arg(d.co2, 0, 'f', 0));
}

// ======================== 刷卡事件 ========================
void MainWindow::onCardDetected(const QString &cardId, bool granted)
{
    if (granted) {
        m_labelCardInfo->setText(QString("Card: %1 [GRANTED]").arg(cardId));
        m_labelCardInfo->setStyleSheet(
            QString("color:%1; font-size:13px; font-weight:bold;").arg(COLOR_GREEN));
        m_labelGateStatus->setText("Gate:  OPEN (access granted)");
        m_labelGateStatus->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_CYAN));
    } else {
        m_labelCardInfo->setText(QString("Card: %1 [REJECTED]").arg(cardId));
        m_labelCardInfo->setStyleSheet(
            QString("color:%1; font-size:13px;").arg(COLOR_RED));
    }
}

// ======================== 报警状态变化 ========================
void MainWindow::onAlarmChanged(bool active)
{
    if (active) {
        m_labelAlarmStatus->setText("!!! ALARM ACTIVE !!!");
        m_labelAlarmStatus->setStyleSheet(
            QString("background:%1; color:%2; font-size:14px; font-weight:bold;")
                .arg(COLOR_DARK_RED, COLOR_WHITE));
    } else {
        m_labelAlarmStatus->setText("Status: ✔ All Normal");
        m_labelAlarmStatus->setStyleSheet(
            QString("background:black; color:%1; font-size:14px; font-weight:bold;")
                .arg(COLOR_GREEN));
    }
}

/* ======================== 桌面调试按钮回调 ======================== */
#ifdef SIMULATE_HARDWARE
void MainWindow::onDebugGasTrigger()
{
    m_hw->simTriggerGas();
}

void MainWindow::onDebugGasClear()
{
    m_hw->simClearGas();
}

void MainWindow::onDebugHchoTrigger()
{
    m_hw->simTriggerHcho();
}

void MainWindow::onDebugHchoClear()
{
    m_hw->simClearHcho();
}

void MainWindow::onDebugCardGrant()
{
    m_hw->simSwipeCard("A1B2C3D4", true);
}

void MainWindow::onDebugCardReject()
{
    m_hw->simSwipeCard("DEADBEEF", false);
}

void MainWindow::onDebugReset()
{
    m_hw->simResetAll();
    /* 恢复门禁状态 */
    m_labelGateStatus->setText("Gate:  ✔ Locked");
    m_labelGateStatus->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GREEN));
    m_labelCardInfo->setText("Card: -- Reset --");
    m_labelCardInfo->setStyleSheet(QString("color:%1; font-size:13px;").arg(COLOR_GRAY));
}
#endif
