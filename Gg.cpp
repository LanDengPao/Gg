#include "Gg.h"
#include "WinDeviceInfo.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWindowStateChangeEvent>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winuser.h>
#include <QOperatingSystemVersion>

#ifndef MOUSE_MOVE_ABSOLUTE
#define MOUSE_MOVE_ABSOLUTE 0x00000001
#endif

namespace {
constexpr int kRefreshIntervalMs = 50;
constexpr int kDeviceInfoPageSize = 11;

QString formatHz(double hz)
{
    if (hz <= 0.0) {
        return QStringLiteral("Hz");
    }
    return QString::number(hz, 'f', 0) + QStringLiteral(" Hz");
}

QString formatSpeed(double speed)
{
    if (speed <= 0.0) {
        return QStringLiteral("px/s");
    }
    return QString::number(speed, 'f', 0) + QStringLiteral(" px/s");
}

QString formatScore(double score)
{
    return QString::number(score, 'f', 1);
}

QString formatJitter(double jitter)
{
    return QString::number(jitter, 'f', 2);
}

QString formatDistance(double dist)
{
    return QString::number(dist, 'f', 0) + QStringLiteral(" px");
}

QString formatDuration(qint64 ms)
{
    if (ms <= 0) {
        return QStringLiteral("-");
    }
    const int seconds = static_cast<int>(ms / 1000);
    const int minutes = seconds / 60;
    const int remainingSeconds = seconds % 60;
    if (minutes > 0) {
        return QStringLiteral("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(remainingSeconds, 2, 10, QChar('0'));
    }
    return QStringLiteral("0:%1").arg(remainingSeconds, 2, 10, QChar('0'));
}

void applyCardStyle(QWidget* card)
{
    card->setStyleSheet(R"(
        QWidget#card {
            background: #1E293B;
            border-radius: 8px;
            border: 1px solid #334155;
            padding: 12px;
        }
    )");
    card->setObjectName("card");
}

QLabel* makeCardLabel(const QString& text, bool bold = false, int fontSize = 14)
{
    auto* label = new QLabel(text);
    label->setStyleSheet(QString(R"(
        QLabel {
            color: #F1F5F9;
            font-size: %1pt;
            %2
        }
    )")
                             .arg(fontSize)
                             .arg(bold ? "font-weight: bold;" : ""));
    return label;
}

QWidget* makeStatCard(const QString& title, QLabel*& valueLabel, QLabel*& unitLabel)
{
    auto* card = new QWidget();
    card->setObjectName("card");
    applyCardStyle(card);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(2);

    auto* titleLabel = makeCardLabel(title, false, 11);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    valueLabel = new QLabel(QStringLiteral("-"));
    valueLabel->setStyleSheet(R"(
        QLabel {
            color: #38BDF8;
            font-size: 18pt;
            font-weight: bold;
            alignment: 'AlignCenter';
        }
    )");
    valueLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(valueLabel);

    unitLabel = new QLabel();
    unitLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(unitLabel);

    return card;
}

QString deviceConnectionText(const DeviceInfo& device, UiLanguage language)
{
    return device.connected
        ? (language == UiLanguage::Chinese ? QStringLiteral("已连接") : QStringLiteral("Connected"))
        : (language == UiLanguage::Chinese ? QStringLiteral("未连接") : QStringLiteral("Disconnected"));
}
}

Gg::Gg(QWidget* parent)
    : QMainWindow(parent)
{
    setAppUiLanguage(m_uiLanguage);
    setWindowTitle(QStringLiteral("Mouse Data"));
    resize(1280, 800);
    setMinimumSize(960, 600);

    m_controller = new AppController(this);
    connect(m_controller, &AppController::stateChanged, this, &Gg::refreshAll);
    connect(m_controller, &AppController::historyChanged, this, &Gg::refreshHistory);

    buildUi();
}

Gg::~Gg()
{
}

void Gg::buildUi()
{
    m_statusLabel = new QLabel(QStringLiteral("Idle"));
    m_modeLabel = new QLabel(QStringLiteral("Monitoring"));
    m_deviceLabel = new QLabel(QStringLiteral("No Device"));
    m_backgroundLabel = new QLabel();
    m_workspaceLabel = new QLabel();

    auto statusStyle = R"(QLabel { color: #94A3B8; font-size: 12px; })";
    m_statusLabel->setStyleSheet(R"(QLabel { color: #F59E0B; font-weight: bold; font-size: 12px; })");
    m_modeLabel->setStyleSheet(statusStyle);
    m_deviceLabel->setStyleSheet(statusStyle);
    m_backgroundLabel->setStyleSheet(R"(QLabel { color: #EF4444; font-weight: bold; font-size: 12px; })");
    m_workspaceLabel->setStyleSheet(statusStyle);

    auto* central = new QWidget;
    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_navList = new QListWidget;
    m_navList->setFixedWidth(180);
    m_navList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_navList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_navList->setSpacing(4);
    m_navList->insertItem(0, QStringLiteral("Dashboard"));
    m_navList->insertItem(1, QStringLiteral("Test"));
    m_navList->insertItem(2, QStringLiteral("History"));
    m_navList->insertItem(3, QStringLiteral("Compare"));
    m_navList->insertItem(4, QStringLiteral("Device"));
    m_navList->setCurrentRow(0);
    connect(m_navList, &QListWidget::currentRowChanged, this, [this](int index) {
        if (m_stack) m_stack->setCurrentIndex(index);
    });

    m_navList->setStyleSheet(R"(
        QListWidget { background: #0F172A; border: none; padding: 8px; }
        QListWidget::item { color: #94A3B8; padding: 10px 16px; border-radius: 6px; margin: 2px 0; }
        QListWidget::item:selected { background: #1E40AF; color: #FFFFFF; font-weight: bold; }
        QListWidget::item:hover:!selected { background: #1E293B; }
    )");

    m_stack = new QStackedWidget;
    mainLayout->addWidget(m_navList);
    mainLayout->addWidget(m_stack, 1);

    m_stack->addWidget(buildDashboardPage());
    m_stack->addWidget(buildTestPage());
    m_stack->addWidget(buildHistoryPage());
    m_stack->addWidget(buildComparePage());
    m_stack->addWidget(buildDevicePage());

    setCentralWidget(central);
    m_languageMenu = menuBar()->addMenu(QString());
    auto* languageGroup = new QActionGroup(this);
    languageGroup->setExclusive(true);
    m_englishAction = m_languageMenu->addAction(QString());
    m_englishAction->setCheckable(true);
    m_chineseAction = m_languageMenu->addAction(QString());
    m_chineseAction->setCheckable(true);
    languageGroup->addAction(m_englishAction);
    languageGroup->addAction(m_chineseAction);
    connect(m_englishAction, &QAction::triggered, this, &Gg::onSwitchToEnglish);
    connect(m_chineseAction, &QAction::triggered, this, &Gg::onSwitchToChinese);

    statusBar()->setStyleSheet("QStatusBar { background: #1E293B; }");
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addWidget(new QLabel("|"));
    statusBar()->addWidget(m_modeLabel);
    statusBar()->addWidget(new QLabel("|"));
    statusBar()->addWidget(m_deviceLabel);
    statusBar()->addWidget(new QLabel("|"));
    statusBar()->addWidget(m_backgroundLabel);
    statusBar()->addPermanentWidget(m_workspaceLabel, 1);

    retranslateUi();
}

QWidget* Gg::buildDashboardPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget;
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(12);

    auto* topRow = new QHBoxLayout;
    topRow->setSpacing(12);

    auto* lastSessionCard = new QWidget;
    lastSessionCard->setObjectName("card");
    applyCardStyle(lastSessionCard);
    auto* lastSessionLayout = new QVBoxLayout(lastSessionCard);
    lastSessionLayout->setSpacing(8);
    m_lastSessionTitleLabel = makeCardLabel(QStringLiteral("Last Session"), true, 14);
    lastSessionLayout->addWidget(m_lastSessionTitleLabel);
    m_lastSessionLabel = new QLabel(QStringLiteral("No Records"));
    m_lastSessionLabel->setStyleSheet("color: #94A3B8; font-size: 12px;");
    lastSessionLayout->addWidget(m_lastSessionLabel);
    auto* lastSessionButtons = new QHBoxLayout;
    m_lastSessionDetailsButton = new QPushButton(QStringLiteral("Details"));
    m_lastSessionCompareButton = new QPushButton(QStringLiteral("Compare"));
    connect(m_lastSessionDetailsButton, &QPushButton::clicked, this, [this] { m_navList->setCurrentRow(2); });
    connect(m_lastSessionCompareButton, &QPushButton::clicked, this, [this] { m_navList->setCurrentRow(3); });
    lastSessionButtons->addWidget(m_lastSessionDetailsButton);
    lastSessionButtons->addWidget(m_lastSessionCompareButton);
    lastSessionLayout->addLayout(lastSessionButtons);
    topRow->addWidget(lastSessionCard, 1);

    auto* deviceCard = new QWidget;
    deviceCard->setObjectName("card");
    applyCardStyle(deviceCard);
    auto* deviceLayout = new QVBoxLayout(deviceCard);
    deviceLayout->setSpacing(8);
    m_currentDeviceTitleLabel = makeCardLabel(QStringLiteral("Current Device"), true, 14);
    deviceLayout->addWidget(m_currentDeviceTitleLabel);
    m_deviceInfoLabel = new QLabel(QStringLiteral("No Mouse"));
    m_deviceInfoLabel->setStyleSheet("color: #94A3B8; font-size: 12px;");
    deviceLayout->addWidget(m_deviceInfoLabel);
    m_deviceMoreButton = new QPushButton(QStringLiteral("More"));
    connect(m_deviceMoreButton, &QPushButton::clicked, this, [this] { m_navList->setCurrentRow(4); });
    deviceLayout->addWidget(m_deviceMoreButton);
    topRow->addWidget(deviceCard, 1);

    contentLayout->addLayout(topRow);

    auto* statsRow = new QHBoxLayout;
    statsRow->setSpacing(12);

    QLabel *hzVal, *hzUnit;
    auto* hzCard = makeStatCard(QStringLiteral("Current Hz"), hzVal, hzUnit);
    hzUnit->setText(QStringLiteral("Hz"));
    m_dashboardHzTitleLabel = hzCard->findChild<QLabel*>();
    m_dashboardHzUnitLabel = hzUnit;
    m_dashboardHzLabel = hzVal;
    statsRow->addWidget(hzCard);

    QLabel *avgVal, *avgUnit;
    auto* avgCard = makeStatCard(QStringLiteral("30s Avg"), avgVal, avgUnit);
    avgUnit->setText(QStringLiteral("Hz"));
    m_dashboardAvgTitleLabel = avgCard->findChild<QLabel*>();
    m_dashboardAvgUnitLabel = avgUnit;
    m_dashboardAvgLabel = avgVal;
    statsRow->addWidget(avgCard);

    QLabel *stdVal, *stdUnit;
    auto* stdCard = makeStatCard(QStringLiteral("Std Dev"), stdVal, stdUnit);
    stdUnit->setText(QStringLiteral("Hz"));
    m_dashboardStdTitleLabel = stdCard->findChild<QLabel*>();
    m_dashboardStdUnitLabel = stdUnit;
    m_dashboardStdLabel = stdVal;
    statsRow->addWidget(stdCard);

    QLabel *speedVal, *speedUnit;
    auto* speedCard = makeStatCard(QStringLiteral("Speed"), speedVal, speedUnit);
    speedUnit->setText(QStringLiteral("px/s"));
    m_dashboardSpeedTitleLabel = speedCard->findChild<QLabel*>();
    m_dashboardSpeedUnitLabel = speedUnit;
    m_dashboardSpeedLabel = speedVal;
    statsRow->addWidget(speedCard);

    QLabel *jitterVal, *jitterUnit;
    auto* jitterCard = makeStatCard(QStringLiteral("Jitter"), jitterVal, jitterUnit);
    jitterUnit->setText(QStringLiteral("px"));
    m_dashboardJitterTitleLabel = jitterCard->findChild<QLabel*>();
    m_dashboardJitterUnitLabel = jitterUnit;
    m_dashboardJitterLabel = jitterVal;
    statsRow->addWidget(jitterCard);

    QLabel *clickVal, *clickUnit;
    auto* clickCard = makeStatCard(QStringLiteral("Clicks"), clickVal, clickUnit);
    clickUnit->setText(QStringLiteral(""));
    m_dashboardClicksTitleLabel = clickCard->findChild<QLabel*>();
    m_dashboardClicksUnitLabel = clickUnit;
    m_dashboardClicksLabel = clickVal;
    statsRow->addWidget(clickCard);

    contentLayout->addLayout(statsRow);

    auto* chartRow = new QHBoxLayout;
    chartRow->setSpacing(12);

    auto* chartCard = new QWidget;
    chartCard->setObjectName("card");
    applyCardStyle(chartCard);
    auto* chartLayout = new QVBoxLayout(chartCard);
    chartLayout->setSpacing(8);
    m_dashboardChartTitleLabel = makeCardLabel(QStringLiteral("Polling Chart"), false, 12);
    chartLayout->addWidget(m_dashboardChartTitleLabel);
    m_dashboardChart = new SparklineWidget;
    chartLayout->addWidget(m_dashboardChart);
    chartRow->addWidget(chartCard, 2);

    auto* trajCard = new QWidget;
    trajCard->setObjectName("card");
    applyCardStyle(trajCard);
    auto* trajLayout = new QVBoxLayout(trajCard);
    trajLayout->setSpacing(8);
    m_dashboardTrajectoryTitleLabel = makeCardLabel(QStringLiteral("Trajectory"), false, 12);
    trajLayout->addWidget(m_dashboardTrajectoryTitleLabel);
    m_dashboardTrajectory = new TrajectoryWidget;
    trajLayout->addWidget(m_dashboardTrajectory);
    chartRow->addWidget(trajCard, 1);

    contentLayout->addLayout(chartRow);
    contentLayout->addStretch();

    scroll->setWidget(content);
    layout->addWidget(scroll);

    page->setStyleSheet(R"(
        QScrollArea { background: #0F172A; }
        QPushButton { background: #3B82F6; color: white; border: none; padding: 6px 12px; border-radius: 4px; font-size: 12px; }
        QPushButton:hover { background: #2563EB; }
        QPushButton:pressed { background: #1D4ED8; }
    )");

    return page;
}

QWidget* Gg::buildTestPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget;
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(12);

    m_testModeGroup = new QGroupBox(QStringLiteral("Test Mode"));
    m_testModeGroup->setStyleSheet(R"(
        QGroupBox { color: #F1F5F9; border: 1px solid #334155; border-radius: 8px; margin-top: 8px; padding: 12px; font-size: 14px; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }
    )");
    auto* modeLayout = new QHBoxLayout(m_testModeGroup);
    m_testModeCombo = new QComboBox;
    m_testModeCombo->addItem(QStringLiteral("Polling Rate"), QVariant::fromValue(static_cast<int>(TestMode::PollingRate)));
    m_testModeCombo->addItem(QStringLiteral("Trajectory Stability"), QVariant::fromValue(static_cast<int>(TestMode::TrajectoryStability)));
    m_testModeCombo->setStyleSheet(R"(
        QComboBox { background: #1E293B; color: #F1F5F9; border: 1px solid #334155; border-radius: 4px; padding: 8px 12px; min-width: 200px; }
        QComboBox:hover { border-color: #3B82F6; }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView { background: #1E293B; color: #F1F5F9; selection-background-color: #3B82F6; }
    )");
    modeLayout->addWidget(m_testModeCombo);
    modeLayout->addStretch();
    contentLayout->addWidget(m_testModeGroup);

    m_controlGroup = new QGroupBox(QStringLiteral("Control"));
    m_controlGroup->setStyleSheet(m_testModeGroup->styleSheet());
    auto* controlLayout = new QVBoxLayout(m_controlGroup);
    auto* controlRow = new QHBoxLayout;
    m_startButton = new QPushButton(QStringLiteral("Start"));
    m_stopButton = new QPushButton(QStringLiteral("Stop"));
    m_stopButton->setEnabled(false);
    connect(m_startButton, &QPushButton::clicked, this, &Gg::onStartTest);
    connect(m_stopButton, &QPushButton::clicked, this, &Gg::onStopTest);
    controlRow->addWidget(m_startButton);
    controlRow->addWidget(m_stopButton);

    m_testStateLabel = new QLabel(QStringLiteral("Status: Idle"));
    controlRow->addWidget(m_testStateLabel);
    controlRow->addStretch();
    controlLayout->addLayout(controlRow);

    m_testMetricsLabel = new QLabel();
    m_testMetricsLabel->setStyleSheet("color: #94A3B8; font-size: 12px;");
    controlLayout->addWidget(m_testMetricsLabel);
    contentLayout->addWidget(m_controlGroup);

    m_metricsGroup = new QGroupBox(QStringLiteral("Metrics"));
    m_metricsGroup->setStyleSheet(m_testModeGroup->styleSheet());
    auto* metricsLayout = new QVBoxLayout(m_metricsGroup);
    m_testResultLabel = new QLabel(QStringLiteral("Start a test to collect data."));
    m_testResultLabel->setStyleSheet("color: #94A3B8; font-size: 13px;");
    metricsLayout->addWidget(m_testResultLabel);
    contentLayout->addWidget(m_metricsGroup);

    m_chartGroup = new QGroupBox(QStringLiteral("Chart"));
    m_chartGroup->setStyleSheet(m_testModeGroup->styleSheet());
    auto* chartLayout = new QHBoxLayout(m_chartGroup);
    m_testChart = new SparklineWidget;
    m_testTrajectory = new TrajectoryWidget;
    chartLayout->addWidget(m_testChart, 2);
    chartLayout->addWidget(m_testTrajectory, 1);
    contentLayout->addWidget(m_chartGroup);

    contentLayout->addStretch();

    scroll->setWidget(content);
    layout->addWidget(scroll);

    page->setStyleSheet(R"(
        QScrollArea { background: #0F172A; }
        QPushButton { background: #3B82F6; color: white; border: none; padding: 10px 24px; border-radius: 6px; font-size: 14px; font-weight: bold; }
        QPushButton:hover { background: #2563EB; }
        QPushButton:pressed { background: #1D4ED8; }
        QPushButton:disabled { background: #475569; color: #94A3B8; }
    )");

    return page;
}

QWidget* Gg::buildHistoryPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);

    m_historyTitleLabel = new QLabel(QStringLiteral("History"));
    m_historyTitleLabel->setStyleSheet("color: #F1F5F9; font-size: 20px; font-weight: bold;");
    layout->addWidget(m_historyTitleLabel);

    m_historyTable = new QTableWidget;
    m_historyTable->setColumnCount(8);
    m_historyTable->setHorizontalHeaderLabels({QStringLiteral("Time"), QStringLiteral("Mode"), QStringLiteral("Device"), QStringLiteral("Duration"), QStringLiteral("Avg Hz"), QStringLiteral("Stability"), QStringLiteral("Jitter"), QStringLiteral("Action")});
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::MultiSelection);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setShowGrid(false);
    m_historyTable->verticalHeader()->setVisible(false);

    auto* btnRow = new QHBoxLayout;
    m_historyExportButton = new QPushButton(QStringLiteral("Export"));
    m_historyCompareButton = new QPushButton(QStringLiteral("Compare"));
    m_historyRefreshButton = new QPushButton(QStringLiteral("Refresh"));
    connect(m_historyExportButton, &QPushButton::clicked, this, &Gg::onExportSelected);
    connect(m_historyCompareButton, &QPushButton::clicked, this, &Gg::onAddToCompare);
    connect(m_historyRefreshButton, &QPushButton::clicked, this, &Gg::refreshHistory);
    btnRow->addWidget(m_historyExportButton);
    btnRow->addWidget(m_historyCompareButton);
    btnRow->addWidget(m_historyRefreshButton);
    btnRow->addStretch();

    layout->addWidget(m_historyTable);
    layout->addLayout(btnRow);

    page->setStyleSheet(R"(
        QWidget { background: #0F172A; }
        QLabel { color: #F1F5F9; }
        QTableWidget { background: #1E293B; color: #F1F5F9; border: 1px solid #334155; border-radius: 8px; gridline-color: #334155; }
        QTableWidget::item { padding: 8px; }
        QTableWidget::item:selected { background: #3B82F6; }
        QHeaderView::section { background: #0F172A; color: #94A3B8; padding: 8px; border: none; border-bottom: 1px solid #334155; }
        QPushButton { background: #3B82F6; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-size: 12px; }
        QPushButton:hover { background: #2563EB; }
    )");

    return page;
}

QWidget* Gg::buildComparePage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);

    m_compareTitleLabel = new QLabel(QStringLiteral("Compare"));
    m_compareTitleLabel->setStyleSheet("color: #F1F5F9; font-size: 20px; font-weight: bold;");
    layout->addWidget(m_compareTitleLabel);

    m_compareSelectionLabel = new QLabel(QStringLiteral("Selected 0"));
    m_compareSelectionLabel->setStyleSheet("color: #94A3B8; font-size: 13px;");
    layout->addWidget(m_compareSelectionLabel);

    auto* btnRow = new QHBoxLayout;
    m_compareAddButton = new QPushButton(QStringLiteral("Add"));
    m_compareClearButton = new QPushButton(QStringLiteral("Clear"));
    connect(m_compareAddButton, &QPushButton::clicked, this, [this] { m_navList->setCurrentRow(2); });
    connect(m_compareClearButton, &QPushButton::clicked, this, [this] { m_compareSessionIds.clear(); refreshCompare(); });
    btnRow->addWidget(m_compareAddButton);
    btnRow->addWidget(m_compareClearButton);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    m_compareTable = new QTableWidget;
    m_compareTable->setColumnCount(9);
    m_compareTable->setHorizontalHeaderLabels({QStringLiteral("Time"), QStringLiteral("Mode"), QStringLiteral("Device"), QStringLiteral("Avg Hz"), QStringLiteral("Max Hz"), QStringLiteral("Min Hz"), QStringLiteral("StdDev"), QStringLiteral("Stability"), QStringLiteral("Jitter")});
    m_compareTable->horizontalHeader()->setStretchLastSection(true);
    m_compareTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_compareTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_compareTable->setShowGrid(false);
    m_compareTable->verticalHeader()->setVisible(false);
    layout->addWidget(m_compareTable);

    page->setStyleSheet(R"(
        QWidget { background: #0F172A; }
        QLabel { color: #F1F5F9; }
        QTableWidget { background: #1E293B; color: #F1F5F9; border: 1px solid #334155; border-radius: 8px; gridline-color: #334155; }
        QTableWidget::item { padding: 8px; }
        QTableWidget::item:selected { background: #3B82F6; }
        QHeaderView::section { background: #0F172A; color: #94A3B8; padding: 8px; border: none; border-bottom: 1px solid #334155; }
        QPushButton { background: #3B82F6; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-size: 12px; }
        QPushButton:hover { background: #2563EB; }
    )");

    return page;
}

QWidget* Gg::buildDevicePage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);

    m_deviceTitleLabel = new QLabel(QStringLiteral("Device"));
    m_deviceTitleLabel->setStyleSheet("color: #F1F5F9; font-size: 20px; font-weight: bold;");
    layout->addWidget(m_deviceTitleLabel);

    auto* infoCard = new QWidget;
    infoCard->setObjectName("card");
    applyCardStyle(infoCard);
    auto* infoLayout = new QVBoxLayout(infoCard);

    m_deviceBasicGroup = new QGroupBox(QStringLiteral("Basic"));
    m_deviceBasicGroup->setStyleSheet(R"(
        QGroupBox { color: #F1F5F9; border: 1px solid #475569; border-radius: 6px; margin-top: 8px; padding: 8px; font-size: 13px; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
    )");
    m_deviceBasicLayout = new QFormLayout(m_deviceBasicGroup);
    m_deviceNameValueLabel = makeCardLabel(QStringLiteral("-"), false, 12);
    m_deviceVendorValueLabel = makeCardLabel(QStringLiteral("-"), false, 12);
    m_deviceVidValueLabel = makeCardLabel(QStringLiteral("-"), false, 12);
    m_devicePidValueLabel = makeCardLabel(QStringLiteral("-"), false, 12);
    m_deviceBasicLayout->addRow(QStringLiteral("Name:"), m_deviceNameValueLabel);
    m_deviceBasicLayout->addRow(QStringLiteral("Vendor:"), m_deviceVendorValueLabel);
    m_deviceBasicLayout->addRow(QStringLiteral("VID:"), m_deviceVidValueLabel);
    m_deviceBasicLayout->addRow(QStringLiteral("PID:"), m_devicePidValueLabel);
    infoLayout->addWidget(m_deviceBasicGroup);

    m_deviceStatusGroup = new QGroupBox(QStringLiteral("Status"));
    m_deviceStatusGroup->setStyleSheet(m_deviceBasicGroup->styleSheet());
    m_deviceStatusLayout = new QFormLayout(m_deviceStatusGroup);
    m_deviceConnectedValueLabel = makeCardLabel(QStringLiteral("-"), false, 12);
    m_deviceTypeValueLabel = makeCardLabel(QStringLiteral("-"), false, 12);
    m_deviceStatusLayout->addRow(QStringLiteral("Connection:"), m_deviceConnectedValueLabel);
    m_deviceStatusLayout->addRow(QStringLiteral("Type:"), m_deviceTypeValueLabel);
    infoLayout->addWidget(m_deviceStatusGroup);

    layout->addWidget(infoCard);
    layout->addStretch();

    page->setStyleSheet(R"(
        QWidget { background: #0F172A; }
        QLabel { color: #F1F5F9; }
        QFormLayout { row-height: 28px; }
        QFormLayout label-item { color: #94A3B8; min-width: 80px; }
    )");

    return page;
}

void Gg::registerRawInput()
{
    if (m_rawInputRegistered) {
        return;
    }

    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = reinterpret_cast<HWND>(winId());

    if (!RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE))) {
        qWarning("Failed to register raw input");
        return;
    }

    m_rawInputRegistered = true;
}

void Gg::updateUiActiveState()
{
    const bool isActive = isActiveWindow() && !isMinimized();
    m_controller->setUiActive(isActive);
}

void Gg::setLanguage(UiLanguage language)
{
    if (m_uiLanguage == language) {
        return;
    }

    m_uiLanguage = language;
    setAppUiLanguage(language);
    retranslateUi();
    refreshAll();
    refreshHistory();
    refreshCompare();
}

void Gg::retranslateUi()
{
    setWindowTitle(uiText("Mouse Data", "鼠标数据"));

    if (m_navList) {
        m_navList->item(0)->setText(uiText("Dashboard", "首页"));
        m_navList->item(1)->setText(uiText("Test", "测试"));
        m_navList->item(2)->setText(uiText("History", "历史"));
        m_navList->item(3)->setText(uiText("Compare", "对比"));
        m_navList->item(4)->setText(uiText("Device", "设备"));
    }

    if (m_languageMenu) m_languageMenu->setTitle(uiText("Language", "语言"));
    if (m_englishAction) {
        m_englishAction->setText(QStringLiteral("English"));
        m_englishAction->setChecked(m_uiLanguage == UiLanguage::English);
    }
    if (m_chineseAction) {
        m_chineseAction->setText(QStringLiteral("中文"));
        m_chineseAction->setChecked(m_uiLanguage == UiLanguage::Chinese);
    }

    if (m_lastSessionTitleLabel) m_lastSessionTitleLabel->setText(uiText("Last Session", "最近一次测试"));
    if (m_lastSessionDetailsButton) m_lastSessionDetailsButton->setText(uiText("Details", "详情"));
    if (m_lastSessionCompareButton) m_lastSessionCompareButton->setText(uiText("Compare", "对比"));
    if (m_currentDeviceTitleLabel) m_currentDeviceTitleLabel->setText(uiText("Current Device", "当前设备"));
    if (m_deviceMoreButton) m_deviceMoreButton->setText(uiText("More", "更多"));

    if (m_dashboardHzTitleLabel) m_dashboardHzTitleLabel->setText(uiText("Current Hz", "当前 Hz"));
    if (m_dashboardAvgTitleLabel) m_dashboardAvgTitleLabel->setText(uiText("30s Avg", "30 秒平均"));
    if (m_dashboardStdTitleLabel) m_dashboardStdTitleLabel->setText(uiText("Std Dev", "标准差"));
    if (m_dashboardSpeedTitleLabel) m_dashboardSpeedTitleLabel->setText(uiText("Speed", "速度"));
    if (m_dashboardJitterTitleLabel) m_dashboardJitterTitleLabel->setText(uiText("Jitter", "抖动"));
    if (m_dashboardClicksTitleLabel) m_dashboardClicksTitleLabel->setText(uiText("Clicks", "点击"));
    if (m_dashboardHzUnitLabel) m_dashboardHzUnitLabel->setText(QStringLiteral("Hz"));
    if (m_dashboardAvgUnitLabel) m_dashboardAvgUnitLabel->setText(QStringLiteral("Hz"));
    if (m_dashboardStdUnitLabel) m_dashboardStdUnitLabel->setText(QStringLiteral("Hz"));
    if (m_dashboardSpeedUnitLabel) m_dashboardSpeedUnitLabel->setText(QStringLiteral("px/s"));
    if (m_dashboardJitterUnitLabel) m_dashboardJitterUnitLabel->setText(QStringLiteral("px"));
    if (m_dashboardClicksUnitLabel) m_dashboardClicksUnitLabel->setText(QString());
    if (m_dashboardChartTitleLabel) m_dashboardChartTitleLabel->setText(uiText("Polling Chart", "轮询曲线"));
    if (m_dashboardTrajectoryTitleLabel) m_dashboardTrajectoryTitleLabel->setText(uiText("Trajectory", "轨迹"));

    if (m_testModeGroup) m_testModeGroup->setTitle(uiText("Test Mode", "测试模式"));
    if (m_testModeCombo) {
        m_testModeCombo->setItemText(0, uiText("Polling Rate", "轮询率测试"));
        m_testModeCombo->setItemText(1, uiText("Trajectory Stability", "轨迹稳定性测试"));
    }
    if (m_controlGroup) m_controlGroup->setTitle(uiText("Control", "控制"));
    if (m_startButton) m_startButton->setText(uiText("Start", "开始"));
    if (m_stopButton) m_stopButton->setText(uiText("Stop", "停止"));
    if (m_metricsGroup) m_metricsGroup->setTitle(uiText("Metrics", "指标"));
    if (m_chartGroup) m_chartGroup->setTitle(uiText("Chart", "图表"));

    if (m_historyTitleLabel) m_historyTitleLabel->setText(uiText("History", "历史记录"));
    if (m_historyTable) {
        m_historyTable->setHorizontalHeaderLabels({
            uiText("Time", "时间"),
            uiText("Mode", "模式"),
            uiText("Device", "设备"),
            uiText("Duration", "时长"),
            uiText("Avg Hz", "平均 Hz"),
            uiText("Stability", "稳定度"),
            uiText("Jitter", "抖动"),
            uiText("Action", "操作")
        });
    }
    if (m_historyExportButton) m_historyExportButton->setText(uiText("Export", "导出"));
    if (m_historyCompareButton) m_historyCompareButton->setText(uiText("Compare", "对比"));
    if (m_historyRefreshButton) m_historyRefreshButton->setText(uiText("Refresh", "刷新"));

    if (m_compareTitleLabel) m_compareTitleLabel->setText(uiText("Compare", "对比"));
    if (m_compareAddButton) m_compareAddButton->setText(uiText("Add", "添加"));
    if (m_compareClearButton) m_compareClearButton->setText(uiText("Clear", "清空"));
    if (m_compareTable) {
        m_compareTable->setHorizontalHeaderLabels({
            uiText("Time", "时间"),
            uiText("Mode", "模式"),
            uiText("Device", "设备"),
            uiText("Avg Hz", "平均 Hz"),
            uiText("Max Hz", "最大 Hz"),
            uiText("Min Hz", "最小 Hz"),
            uiText("Std Dev", "标准差"),
            uiText("Stability", "稳定度"),
            uiText("Jitter", "抖动")
        });
    }

    if (m_deviceTitleLabel) m_deviceTitleLabel->setText(uiText("Device", "设备"));
    if (m_deviceBasicGroup) m_deviceBasicGroup->setTitle(uiText("Basic", "基础信息"));
    if (m_deviceStatusGroup) m_deviceStatusGroup->setTitle(uiText("Status", "状态"));
    if (m_deviceBasicLayout) {
        auto updateLabel = [](QFormLayout* layout, int row, const QString& text) {
            if (auto* item = layout->itemAt(row, QFormLayout::LabelRole)) {
                if (auto* label = qobject_cast<QLabel*>(item->widget())) {
                    label->setText(text);
                }
            }
        };
        updateLabel(m_deviceBasicLayout, 0, uiText("Name:", "名称:"));
        updateLabel(m_deviceBasicLayout, 1, uiText("Vendor:", "厂商:"));
        updateLabel(m_deviceBasicLayout, 2, QStringLiteral("VID:"));
        updateLabel(m_deviceBasicLayout, 3, QStringLiteral("PID:"));
    }
    if (m_deviceStatusLayout) {
        auto updateLabel = [](QFormLayout* layout, int row, const QString& text) {
            if (auto* item = layout->itemAt(row, QFormLayout::LabelRole)) {
                if (auto* label = qobject_cast<QLabel*>(item->widget())) {
                    label->setText(text);
                }
            }
        };
        updateLabel(m_deviceStatusLayout, 0, uiText("Connection:", "连接:"));
        updateLabel(m_deviceStatusLayout, 1, uiText("Type:", "类型:"));
    }

    if (m_workspaceLabel) {
        m_workspaceLabel->setText(uiText("Workspace: %1", "工作区: %1").arg(m_controller->workspacePath()));
    }
}

void Gg::refreshAll()
{
    if (!m_controller || !m_statusLabel || !m_modeLabel) return;

    const LiveSnapshot snap = m_controller->snapshot();

    if (m_statusLabel) m_statusLabel->setText(sessionStatusToDisplayString(snap.runStatus, m_uiLanguage));
    if (m_modeLabel) m_modeLabel->setText(testModeToDisplayString(snap.currentMode, m_uiLanguage));

    if (snap.recording) {
        if (m_statusLabel) m_statusLabel->setStyleSheet("color: #EF4444; font-weight: bold; font-size: 12px;");
    } else {
        if (m_statusLabel) m_statusLabel->setStyleSheet("color: #22C55E; font-weight: bold; font-size: 12px;");
    }

    if (m_deviceLabel) {
        m_deviceLabel->setText(snap.device.displayName.isEmpty() ? uiText("No Device", "无设备") : snap.device.displayName);
    }

    if (m_backgroundLabel) {
        m_backgroundLabel->setText((snap.recording && !snap.uiActive) ? uiText("Recording in Background", "后台录制中") : QString());
    }

    if (m_dashboardHzLabel) m_dashboardHzLabel->setText(formatHz(snap.currentHz));
    if (m_dashboardAvgLabel) m_dashboardAvgLabel->setText(formatHz(snap.avgHz30s));
    if (m_dashboardStdLabel) m_dashboardStdLabel->setText(formatHz(snap.stddevHz));
    if (m_dashboardSpeedLabel) m_dashboardSpeedLabel->setText(formatSpeed(snap.currentSpeed));
    if (m_dashboardJitterLabel) m_dashboardJitterLabel->setText(formatJitter(snap.jitterValue));
    if (m_dashboardClicksLabel) m_dashboardClicksLabel->setText(QString::number(snap.leftClickCount + snap.rightClickCount + snap.middleClickCount));

    if (m_dashboardChart) m_dashboardChart->setValues(snap.pollingHistory);
    if (m_dashboardTrajectory) m_dashboardTrajectory->setPoints(snap.trajectory);

    if (m_lastSessionLabel) {
        const SessionRecord last = m_controller->lastSession();
        if (!last.sessionId.isEmpty()) {
            m_lastSessionLabel->setText(QStringLiteral("%1 | %2 | %3 %4 Hz | %5 %6")
                                     .arg(last.startTimeUtc.toString(QStringLiteral("yyyy-MM-dd HH:mm")))
                                     .arg(testModeToDisplayString(last.mode, m_uiLanguage))
                                     .arg(uiText("Avg", "平均"))
                                     .arg(last.summary.avgHz, 0, 'f', 0)
                                     .arg(uiText("Score", "评分"))
                                     .arg(formatScore(last.summary.stabilityScore)));
        } else {
            m_lastSessionLabel->setText(uiText("No Records", "暂无记录"));
        }
    }

    if (m_deviceInfoLabel) {
        if (!snap.device.deviceId.isEmpty()) {
            m_deviceInfoLabel->setText(QStringLiteral("%1\nVID: %2 | PID: %3")
                                           .arg(snap.device.displayName)
                                           .arg(snap.device.vendorId)
                                           .arg(snap.device.productId));
        } else {
            m_deviceInfoLabel->setText(uiText("No Mouse", "未检测到鼠标"));
        }
    }

    if (m_deviceNameValueLabel) m_deviceNameValueLabel->setText(snap.device.displayName.isEmpty() ? QStringLiteral("-") : snap.device.displayName);
    if (m_deviceVendorValueLabel) m_deviceVendorValueLabel->setText(snap.device.vendorId.isEmpty() ? QStringLiteral("-") : snap.device.vendorId);
    if (m_deviceVidValueLabel) m_deviceVidValueLabel->setText(snap.device.vendorId.isEmpty() ? QStringLiteral("-") : snap.device.vendorId);
    if (m_devicePidValueLabel) m_devicePidValueLabel->setText(snap.device.productId.isEmpty() ? QStringLiteral("-") : snap.device.productId);
    if (m_deviceConnectedValueLabel) m_deviceConnectedValueLabel->setText(deviceConnectionText(snap.device, m_uiLanguage));
    if (m_deviceTypeValueLabel) m_deviceTypeValueLabel->setText(snap.device.connectionType.isEmpty() ? QStringLiteral("-") : snap.device.connectionType);

    if (m_startButton && m_stopButton) {
        const bool isRecording = snap.recording;
        m_startButton->setEnabled(!isRecording);
        m_stopButton->setEnabled(isRecording);

        if (isRecording) {
            if (m_testStateLabel) m_testStateLabel->setText(uiText("Status: Recording", "状态: 录制中"));
            if (m_testMetricsLabel && m_controller) {
                const qint64 elapsed = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - snap.recordingStartMs;
                m_testMetricsLabel->setText(QStringLiteral("%1: %2 | %3: %4 | %5: %6")
                                             .arg(uiText("Current", "当前"))
                                             .arg(formatHz(snap.currentHz))
                                             .arg(uiText("Avg", "平均"))
                                             .arg(formatHz(snap.avgHz30s))
                                             .arg(uiText("Time", "时间"))
                                             .arg(formatDuration(elapsed)));
            }
            if (m_testChart) m_testChart->setValues(snap.pollingHistory);
            if (m_testTrajectory) m_testTrajectory->setPoints(snap.trajectory);
        } else {
            if (m_testStateLabel) m_testStateLabel->setText(uiText("Status: Idle", "状态: 空闲"));
            if (m_testMetricsLabel) m_testMetricsLabel->setText(QString());
        }
    }

}

void Gg::refreshHistory()
{
    m_controller->reloadHistory();
    const QVector<SessionRecord> records = m_controller->history();

    m_historyTable->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const SessionRecord& rec = records.at(i);
        m_historyTable->setItem(i, 0, new QTableWidgetItem(rec.startTimeUtc.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
        m_historyTable->setItem(i, 1, new QTableWidgetItem(testModeToDisplayString(rec.mode, m_uiLanguage)));
        m_historyTable->setItem(i, 2, new QTableWidgetItem(rec.summary.device.displayName));
        m_historyTable->setItem(i, 3, new QTableWidgetItem(formatDuration(rec.durationMs)));
        m_historyTable->setItem(i, 4, new QTableWidgetItem(formatHz(rec.summary.avgHz)));
        m_historyTable->setItem(i, 5, new QTableWidgetItem(formatScore(rec.summary.stabilityScore)));
        m_historyTable->setItem(i, 6, new QTableWidgetItem(formatJitter(rec.summary.jitterValue)));
        m_historyTable->setItem(i, 7, new QTableWidgetItem(uiText("[Export]", "[导出]")));
        m_historyTable->item(i, 7)->setData(Qt::UserRole, rec.sessionId);
    }
}

void Gg::refreshCompare()
{
    const QVector<SessionRecord> allRecords = m_controller->history();
    QVector<SessionRecord> selected;
    for (const QString& id : m_compareSessionIds) {
        for (const SessionRecord& rec : allRecords) {
            if (rec.sessionId == id) {
                selected.append(rec);
                break;
            }
        }
    }

    m_compareSelectionLabel->setText(uiText("Selected %1", "已选择 %1").arg(selected.size()));

    m_compareTable->setRowCount(selected.size());
    for (int i = 0; i < selected.size(); ++i) {
        const SessionRecord& rec = selected.at(i);
        m_compareTable->setItem(i, 0, new QTableWidgetItem(rec.startTimeUtc.toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
        m_compareTable->setItem(i, 1, new QTableWidgetItem(testModeToDisplayString(rec.mode, m_uiLanguage)));
        m_compareTable->setItem(i, 2, new QTableWidgetItem(rec.summary.device.displayName));
        m_compareTable->setItem(i, 3, new QTableWidgetItem(formatHz(rec.summary.avgHz)));
        m_compareTable->setItem(i, 4, new QTableWidgetItem(formatHz(rec.summary.maxHz)));
        m_compareTable->setItem(i, 5, new QTableWidgetItem(formatHz(rec.summary.minHz)));
        m_compareTable->setItem(i, 6, new QTableWidgetItem(formatHz(rec.summary.stddevHz)));
        m_compareTable->setItem(i, 7, new QTableWidgetItem(formatScore(rec.summary.stabilityScore)));
        m_compareTable->setItem(i, 8, new QTableWidgetItem(formatJitter(rec.summary.jitterValue)));
    }
}

void Gg::handleRawInput(void* lParam)
{
    HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(lParam);

    UINT dwSize = 0;
    GetRawInputData(hRawInput, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
    if (dwSize == 0) return;

    QByteArray buffer(dwSize, 0);
    if (GetRawInputData(hRawInput, RID_INPUT, buffer.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) return;

    const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
    if (raw->header.dwType != RIM_TYPEMOUSE) return;

    const RAWMOUSE& mouse = raw->data.mouse;
    const USHORT flags = mouse.usButtonFlags;
    const bool hasMovement = mouse.lLastX != 0 || mouse.lLastY != 0 || (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0;
    if (!hasMovement && flags == 0) return;

    const qint64 tsUs = nowMicroseconds();

    POINT pt;
    GetCursorPos(&pt);

    MouseSample sample;
    sample.timestampUs = tsUs;
    sample.position = QPoint(pt.x, pt.y);
    sample.delta = QPoint(mouse.lLastX, mouse.lLastY);
    sample.wheelDelta = (flags & RI_MOUSE_WHEEL) ? static_cast<short>(raw->data.mouse.usButtonData) : 0;
    sample.buttonFlags = flags;
    sample.eventType = 0;

    if (flags & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN | RI_MOUSE_BUTTON_3_DOWN)) sample.eventType = 1;
    else if (flags & RI_MOUSE_WHEEL) sample.eventType = 2;

    const quintptr deviceKey = reinterpret_cast<quintptr>(raw->header.hDevice);
    if (!m_deviceCache.contains(deviceKey)) {
        m_deviceCache.insert(deviceKey, resolveDeviceInfo(raw->header.hDevice));
    }

    DeviceInfo device = m_deviceCache.value(deviceKey);
    device.connected = true;
    device.lastSeenUtc = QDateTime::currentDateTimeUtc();
    m_deviceCache.insert(deviceKey, device);

    m_controller->ingestSample(sample, device);
}

DeviceInfo Gg::resolveDeviceInfo(void* handle)
{
    DeviceInfo info;
    info.deviceId = QStringLiteral("mouse_%1").arg(reinterpret_cast<quintptr>(handle), 0, 16);
    info.connected = true;
    info.lastSeenUtc = QDateTime::currentDateTimeUtc();

    UINT numDevices = 0;
    if (GetRawInputDeviceList(nullptr, &numDevices, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) return info;

    QVector<RAWINPUTDEVICELIST> deviceList(numDevices);
    GetRawInputDeviceList(deviceList.data(), &numDevices, sizeof(RAWINPUTDEVICELIST));

    for (const RAWINPUTDEVICELIST& dev : deviceList) {
        if (dev.hDevice != handle) continue;

        UINT size = 0;
        GetRawInputDeviceInfo(dev.hDevice, RIDI_DEVICENAME, nullptr, &size);
        if (size > 0) {
            QVector<wchar_t> name(size);
            GetRawInputDeviceInfo(dev.hDevice, RIDI_DEVICENAME, name.data(), &size);

            QString deviceName = QString::fromWCharArray(name.data());
            info.rawDeviceName = deviceName;
            populateFriendlyMouseName(deviceName, info);
            if (info.displayName.isEmpty()) {
                info.displayName = QStringLiteral("Mouse");
            }

            if (deviceName.contains(QStringLiteral("#VID_"), Qt::CaseInsensitive)) {
                QString vid = deviceName;
                QString pid = deviceName;
                vid = vid.mid(vid.indexOf(QStringLiteral("#VID_"), 0, Qt::CaseInsensitive) + 5, 4);
                pid = pid.mid(pid.indexOf(QStringLiteral("#PID_"), 0, Qt::CaseInsensitive) + 5, 4);
                info.vendorId = QStringLiteral("0x%1").arg(vid);
                info.productId = QStringLiteral("0x%1").arg(pid);
            }

            if (deviceName.contains(QStringLiteral("HID"), Qt::CaseInsensitive)) {
                info.connectionType = QStringLiteral("USB");
            }
        }
        break;
    }

    if (info.displayName.isEmpty()) info.displayName = QStringLiteral("Mouse");
    return info;
}

void Gg::onStartTest()
{
    const int modeIndex = m_testModeCombo->currentData().toInt();
    const TestMode mode = static_cast<TestMode>(modeIndex);

    QString error;
    if (!m_controller->startTest(mode, &error)) {
        QMessageBox::warning(this, uiText("Error", "错误"), error);
        return;
    }
    refreshAll();
}

void Gg::onStopTest()
{
    QString error;
    if (!m_controller->stopTest(&error)) {
        QMessageBox::warning(this, uiText("Error", "错误"), error);
        return;
    }

    refreshAll();
    refreshHistory();

    const SessionRecord last = m_controller->lastSession();
    if (!last.sessionId.isEmpty()) {
        m_testResultLabel->setText(QStringLiteral("%1 %2: %3 | %4: %5 | %6: %7")
                                        .arg(uiText("Completed!", "已完成!"))
                                        .arg(uiText("Avg", "平均"))
                                        .arg(formatHz(last.summary.avgHz))
                                        .arg(uiText("Score", "评分"))
                                        .arg(formatScore(last.summary.stabilityScore))
                                        .arg(uiText("Time", "时间"))
                                        .arg(formatDuration(last.durationMs)));
    }
}

void Gg::onExportSelected()
{
    const QModelIndexList selected = m_historyTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::information(this, uiText("Export", "导出"), uiText("Select records first.", "请先选择记录。"));
        return;
    }

    const QString filter = QStringLiteral("CSV (*.csv);;JSON (*.json)");
    const QString fileName = QFileDialog::getSaveFileName(this, uiText("Export", "导出"), QString(), filter);
    if (fileName.isEmpty()) return;

    const bool asJson = fileName.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive);
    const int row = selected.first().row();
    const QString sessionId = m_historyTable->item(row, 7)->data(Qt::UserRole).toString();

    QString error;
    if (!m_controller->exportSession(sessionId, fileName, asJson, &error)) {
        QMessageBox::warning(this, uiText("Error", "错误"), error);
        return;
    }

    QMessageBox::information(this, uiText("Success", "成功"), uiText("Exported to:\n%1", "已导出到:\n%1").arg(fileName));
}

void Gg::onAddToCompare()
{
    const QModelIndexList selected = m_historyTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::information(this, uiText("Compare", "对比"), uiText("Select records first.", "请先选择记录。"));
        return;
    }

    for (const QModelIndex& idx : selected) {
        const int row = idx.row();
        const QString sessionId = m_historyTable->item(row, 7)->data(Qt::UserRole).toString();
        if (!m_compareSessionIds.contains(sessionId)) {
            m_compareSessionIds.append(sessionId);
        }
    }

    refreshCompare();
    m_navList->setCurrentRow(3);
}

void Gg::onSwitchToEnglish()
{
    setLanguage(UiLanguage::English);
}

void Gg::onSwitchToChinese()
{
    setLanguage(UiLanguage::Chinese);
}

bool Gg::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(result);
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_INPUT) {
            handleRawInput(reinterpret_cast<void*>(msg->lParam));
            return true;
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void Gg::changeEvent(QEvent* event)
{
    if (event && (event->type() == QEvent::WindowStateChange || event->type() == QEvent::ActivationChange)) {
        updateUiActiveState();
    }
    QMainWindow::changeEvent(event);
}

void Gg::showEvent(QShowEvent* event)
{
    static bool first = true;
    if (first) {
        first = false;
        auto* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Gg::refreshAll);
        timer->start(200);
        QTimer::singleShot(0, this, [this] {
            registerRawInput();
            updateUiActiveState();
            refreshHistory();
            refreshCompare();
            refreshAll();
        });
    }
    QMainWindow::showEvent(event);
}

qint64 Gg::nowMicroseconds()
{
    LARGE_INTEGER counter;
    LARGE_INTEGER freq;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&freq);
    return (counter.QuadPart * 1000000) / freq.QuadPart;
}
