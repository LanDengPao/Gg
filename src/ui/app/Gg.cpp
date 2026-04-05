#include "Gg.h"

#include "ui_Gg.h"

#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QChar>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QShowEvent>
#include <QSizePolicy>
#include <QString>
#include <QStyle>
#include <QTableWidgetItem>
#include <QTimer>

namespace {
constexpr int kRefreshIntervalMs = 200;
constexpr int kHistoryActionColumn = 7;

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

QString deviceConnectionText(const DeviceInfo& device, UiLanguage language)
{
    return device.connected
        ? (language == UiLanguage::Chinese ? QStringLiteral("已连接") : QStringLiteral("Connected"))
        : (language == UiLanguage::Chinese ? QStringLiteral("未连接") : QStringLiteral("Disconnected"));
}

void setListItemText(QListWidget* list, int row, const QString& text)
{
    if (!list || row < 0 || row >= list->count()) {
        return;
    }
    if (auto* item = list->item(row)) {
        item->setText(text);
    }
}

QLabel* createStatusLabel(const QString& objectName, QWidget* parent)
{
    auto* label = new QLabel(parent);
    label->setObjectName(objectName);
    return label;
}

QLabel* createStatusSeparator(QWidget* parent)
{
    auto* label = new QLabel(QStringLiteral("|"), parent);
    label->setObjectName(QStringLiteral("statusSeparatorLabel"));
    return label;
}
}  // namespace

Gg::Gg(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::Gg>())
{
    setAppUiLanguage(m_uiLanguage);
    m_ui->setupUi(this);

    m_controller = new AppController(this);
    connect(m_controller, &AppController::stateChanged, this, &Gg::refreshAll);
    connect(m_controller, &AppController::historyChanged, this, &Gg::refreshHistory);

    initializeUi();
}

Gg::~Gg() = default;

void Gg::initializeUi()
{
    registerStatusBar();
    applyAppStyleSheet();

    for (QWidget* card : {static_cast<QWidget*>(m_ui->lastSessionCard),
                          static_cast<QWidget*>(m_ui->currentDeviceCard),
                          static_cast<QWidget*>(m_ui->dashboardHzCard),
                          static_cast<QWidget*>(m_ui->dashboardAvgCard),
                          static_cast<QWidget*>(m_ui->dashboardStdCard),
                          static_cast<QWidget*>(m_ui->dashboardSpeedCard),
                          static_cast<QWidget*>(m_ui->dashboardJitterCard),
                          static_cast<QWidget*>(m_ui->dashboardClicksCard),
                          static_cast<QWidget*>(m_ui->dashboardChartCard),
                          static_cast<QWidget*>(m_ui->dashboardTrajectoryCard),
                          static_cast<QWidget*>(m_ui->deviceInfoCard)}) {
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    m_ui->dashboardTopRowLayout->setStretch(0, 1);
    m_ui->dashboardTopRowLayout->setStretch(1, 1);
    for (int i = 0; i < 6; ++i) {
        m_ui->dashboardStatsRowLayout->setStretch(i, 1);
    }
    m_ui->dashboardChartRowLayout->setStretch(0, 2);
    m_ui->dashboardChartRowLayout->setStretch(1, 1);

    m_ui->navigationList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->navigationList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->navigationList->setSpacing(4);
    if (m_ui->navigationList->count() == 0) {
        for (int i = 0; i < 5; ++i) {
            m_ui->navigationList->addItem(QString());
        }
    }
    connect(m_ui->navigationList, &QListWidget::currentRowChanged, this, [this](int index) {
        if (index >= 0) {
            m_ui->pageStack->setCurrentIndex(index);
        }
    });

    m_ui->testModeCombo->clear();
    m_ui->testModeCombo->addItem(QString(), QVariant::fromValue(static_cast<int>(TestMode::PollingRate)));
    m_ui->testModeCombo->addItem(QString(), QVariant::fromValue(static_cast<int>(TestMode::TrajectoryStability)));
    m_ui->stopButton->setEnabled(false);

    m_ui->historyTable->setColumnCount(8);
    m_ui->historyTable->horizontalHeader()->setStretchLastSection(true);
    m_ui->historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->historyTable->setSelectionMode(QAbstractItemView::MultiSelection);
    m_ui->historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->historyTable->setShowGrid(false);
    m_ui->historyTable->verticalHeader()->setVisible(false);

    m_ui->compareTable->setColumnCount(9);
    m_ui->compareTable->horizontalHeader()->setStretchLastSection(true);
    m_ui->compareTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->compareTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->compareTable->setShowGrid(false);
    m_ui->compareTable->verticalHeader()->setVisible(false);

    connect(m_ui->startButton, &QPushButton::clicked, this, &Gg::onStartTest);
    connect(m_ui->stopButton, &QPushButton::clicked, this, &Gg::onStopTest);
    connect(m_ui->lastSessionDetailsButton, &QPushButton::clicked, this, [this] { m_ui->navigationList->setCurrentRow(2); });
    connect(m_ui->lastSessionCompareButton, &QPushButton::clicked, this, [this] { m_ui->navigationList->setCurrentRow(3); });
    connect(m_ui->deviceMoreButton, &QPushButton::clicked, this, [this] { m_ui->navigationList->setCurrentRow(4); });
    connect(m_ui->historyExportButton, &QPushButton::clicked, this, &Gg::onExportSelected);
    connect(m_ui->historyCompareButton, &QPushButton::clicked, this, &Gg::onAddToCompare);
    connect(m_ui->historyRefreshButton, &QPushButton::clicked, this, &Gg::refreshHistory);
    connect(m_ui->compareAddButton, &QPushButton::clicked, this, [this] { m_ui->navigationList->setCurrentRow(2); });
    connect(m_ui->compareClearButton, &QPushButton::clicked, this, [this] {
        m_compareSessionIds.clear();
        refreshCompare();
    });
    connect(m_ui->actionEnglish, &QAction::triggered, this, &Gg::onSwitchToEnglish);
    connect(m_ui->actionChinese, &QAction::triggered, this, &Gg::onSwitchToChinese);

    m_ui->navigationList->setCurrentRow(0);
    retranslateUi();
}

void Gg::registerStatusBar()
{
    m_statusLabel = createStatusLabel(QStringLiteral("statusLabel"), this);
    m_modeLabel = createStatusLabel(QStringLiteral("modeLabel"), this);
    m_deviceLabel = createStatusLabel(QStringLiteral("deviceLabel"), this);
    m_backgroundLabel = createStatusLabel(QStringLiteral("backgroundLabel"), this);
    m_workspaceLabel = createStatusLabel(QStringLiteral("workspaceLabel"), this);

    statusBar()->addWidget(m_statusLabel);
    statusBar()->addWidget(createStatusSeparator(this));
    statusBar()->addWidget(m_modeLabel);
    statusBar()->addWidget(createStatusSeparator(this));
    statusBar()->addWidget(m_deviceLabel);
    statusBar()->addWidget(createStatusSeparator(this));
    statusBar()->addWidget(m_backgroundLabel);
    statusBar()->addPermanentWidget(m_workspaceLabel, 1);

    updateStatusTone(false);
}

void Gg::applyAppStyleSheet()
{
    QFile file(QStringLiteral(":/Gg/gg.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
}

void Gg::updateWidgetStyle(QWidget* widget) const
{
    if (!widget) {
        return;
    }

    // Dynamic QSS properties need a repolish to take effect immediately.
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void Gg::updateStatusTone(bool recording)
{
    if (!m_statusLabel) {
        return;
    }

    m_statusLabel->setProperty("tone", recording ? QStringLiteral("recording") : QStringLiteral("active"));
    updateWidgetStyle(m_statusLabel);
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
    m_ui->retranslateUi(this);

    setWindowTitle(uiText("Mouse Data", "鼠标数据"));

    setListItemText(m_ui->navigationList, 0, uiText("Dashboard", "首页"));
    setListItemText(m_ui->navigationList, 1, uiText("Test", "测试"));
    setListItemText(m_ui->navigationList, 2, uiText("History", "历史"));
    setListItemText(m_ui->navigationList, 3, uiText("Compare", "对比"));
    setListItemText(m_ui->navigationList, 4, uiText("Device", "设备"));

    m_ui->menuLanguage->setTitle(uiText("Language", "语言"));
    m_ui->actionEnglish->setText(QStringLiteral("English"));
    m_ui->actionEnglish->setChecked(m_uiLanguage == UiLanguage::English);
    m_ui->actionChinese->setText(QStringLiteral("中文"));
    m_ui->actionChinese->setChecked(m_uiLanguage == UiLanguage::Chinese);

    if (m_statusLabel) m_statusLabel->setText(uiText("Idle", "空闲"));
    if (m_modeLabel) m_modeLabel->setText(uiText("Monitoring", "实时监控"));
    if (m_deviceLabel) m_deviceLabel->setText(uiText("No Device", "无设备"));
    if (m_workspaceLabel) {
        m_workspaceLabel->setText(uiText("Workspace: %1", "工作区: %1").arg(m_controller->workspacePath()));
    }

    m_ui->lastSessionTitleLabel->setText(uiText("Last Session", "最近一次测试"));
    m_ui->lastSessionLabel->setText(uiText("No Records", "暂无记录"));
    m_ui->lastSessionDetailsButton->setText(uiText("Details", "详情"));
    m_ui->lastSessionCompareButton->setText(uiText("Compare", "对比"));

    m_ui->currentDeviceTitleLabel->setText(uiText("Current Device", "当前设备"));
    m_ui->deviceInfoLabel->setText(uiText("No Mouse", "未检测到鼠标"));
    m_ui->deviceMoreButton->setText(uiText("More", "更多"));

    m_ui->dashboardHzTitleLabel->setText(uiText("Current Hz", "当前 Hz"));
    m_ui->dashboardAvgTitleLabel->setText(uiText("1s Avg", "1 秒平均"));
    m_ui->dashboardStdTitleLabel->setText(uiText("Std Dev", "标准差"));
    m_ui->dashboardSpeedTitleLabel->setText(uiText("Speed", "速度"));
    m_ui->dashboardJitterTitleLabel->setText(uiText("Jitter", "抖动"));
    m_ui->dashboardClicksTitleLabel->setText(uiText("Clicks", "点击"));
    m_ui->dashboardHzUnitLabel->setText(QStringLiteral("Hz"));
    m_ui->dashboardAvgUnitLabel->setText(QStringLiteral("Hz"));
    m_ui->dashboardStdUnitLabel->setText(QStringLiteral("Hz"));
    m_ui->dashboardSpeedUnitLabel->setText(QStringLiteral("px/s"));
    m_ui->dashboardJitterUnitLabel->setText(QStringLiteral("px"));
    m_ui->dashboardClicksUnitLabel->setText(QString());
    m_ui->dashboardChartTitleLabel->setText(uiText("Polling Chart", "轮询曲线"));
    m_ui->dashboardTrajectoryTitleLabel->setText(uiText("Trajectory", "轨迹"));

    m_ui->testModeGroup->setTitle(uiText("Test Mode", "测试模式"));
    m_ui->testModeCombo->setItemText(0, uiText("Polling Rate", "轮询率测试"));
    m_ui->testModeCombo->setItemText(1, uiText("Trajectory Stability", "轨迹稳定性测试"));
    m_ui->controlGroup->setTitle(uiText("Control", "控制"));
    m_ui->startButton->setText(uiText("Start", "开始"));
    m_ui->stopButton->setText(uiText("Stop", "停止"));
    m_ui->testStateLabel->setText(uiText("Status: Idle", "状态: 空闲"));
    m_ui->metricsGroup->setTitle(uiText("Metrics", "指标"));
    m_ui->testResultLabel->setText(uiText("Start a test to collect data.", "开始测试后显示结果。"));
    m_ui->chartGroup->setTitle(uiText("Chart", "图表"));

    m_ui->historyTitleLabel->setText(uiText("History", "历史记录"));
    m_ui->historyExportButton->setText(uiText("Export", "导出"));
    m_ui->historyCompareButton->setText(uiText("Compare", "对比"));
    m_ui->historyRefreshButton->setText(uiText("Refresh", "刷新"));
    m_ui->historyTable->setHorizontalHeaderLabels({
        uiText("Time", "时间"),
        uiText("Mode", "模式"),
        uiText("Device", "设备"),
        uiText("Duration", "时长"),
        uiText("Avg Hz", "平均 Hz"),
        uiText("Stability", "稳定度"),
        uiText("Jitter", "抖动"),
        uiText("Action", "操作")
    });

    m_ui->compareTitleLabel->setText(uiText("Compare", "对比"));
    m_ui->compareSelectionLabel->setText(uiText("Selected 0", "已选择 0"));
    m_ui->compareAddButton->setText(uiText("Add", "添加"));
    m_ui->compareClearButton->setText(uiText("Clear", "清空"));
    m_ui->compareTable->setHorizontalHeaderLabels({
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

    m_ui->deviceTitleLabel->setText(uiText("Device", "设备"));
    m_ui->deviceBasicGroup->setTitle(uiText("Basic", "基础信息"));
    m_ui->deviceStatusGroup->setTitle(uiText("Status", "状态"));
    m_ui->deviceNameFieldLabel->setText(uiText("Name:", "名称:"));
    m_ui->deviceVendorFieldLabel->setText(uiText("Vendor:", "厂商:"));
    m_ui->deviceVidFieldLabel->setText(QStringLiteral("VID:"));
    m_ui->devicePidFieldLabel->setText(QStringLiteral("PID:"));
    m_ui->deviceConnectionFieldLabel->setText(uiText("Connection:", "连接:"));
    m_ui->deviceTypeFieldLabel->setText(uiText("Type:", "类型:"));
}

void Gg::refreshAll()
{
    if (!m_controller || !m_statusLabel || !m_modeLabel) {
        return;
    }

    const LiveSnapshot snap = m_controller->snapshot();
    const SessionRecord last = m_controller->lastSession();

    m_statusLabel->setText(sessionStatusToDisplayString(snap.runStatus, m_uiLanguage));
    m_modeLabel->setText(testModeToDisplayString(snap.currentMode, m_uiLanguage));
    updateStatusTone(snap.recording);

    m_deviceLabel->setText(snap.device.displayName.isEmpty() ? uiText("No Device", "无设备") : snap.device.displayName);
    m_backgroundLabel->setText((snap.recording && !snap.uiActive) ? uiText("Recording in Background", "后台录制中") : QString());

    m_ui->dashboardHzValueLabel->setText(formatHz(snap.currentHz));
    m_ui->dashboardAvgValueLabel->setText(formatHz(snap.avgHz1s));
    m_ui->dashboardStdValueLabel->setText(formatHz(snap.stddevHz));
    m_ui->dashboardSpeedValueLabel->setText(formatSpeed(snap.currentSpeed));
    m_ui->dashboardJitterValueLabel->setText(formatJitter(snap.jitterValue));
    m_ui->dashboardClicksValueLabel->setText(QString::number(snap.leftClickCount + snap.rightClickCount + snap.middleClickCount));

    m_ui->dashboardChart->setValues(snap.pollingHistory);
    m_ui->dashboardTrajectory->setPoints(snap.trajectory);

    if (!last.sessionId.isEmpty()) {
        m_ui->lastSessionLabel->setText(QStringLiteral("%1 | %2 | %3 %4 Hz | %5 %6")
                                            .arg(last.startTimeUtc.toString(QStringLiteral("yyyy-MM-dd HH:mm")))
                                            .arg(testModeToDisplayString(last.mode, m_uiLanguage))
                                            .arg(uiText("Avg", "平均"))
                                            .arg(last.summary.avgHz, 0, 'f', 0)
                                            .arg(uiText("Score", "评分"))
                                            .arg(formatScore(last.summary.stabilityScore)));
    } else {
        m_ui->lastSessionLabel->setText(uiText("No Records", "暂无记录"));
    }

    if (!snap.device.deviceId.isEmpty()) {
        m_ui->deviceInfoLabel->setText(QStringLiteral("%1\nVID: %2 | PID: %3")
                                           .arg(snap.device.displayName)
                                           .arg(snap.device.vendorId)
                                           .arg(snap.device.productId));
    } else {
        m_ui->deviceInfoLabel->setText(uiText("No Mouse", "未检测到鼠标"));
    }

    const QString vendorText = !snap.device.manufacturer.isEmpty()
        ? snap.device.manufacturer
        : (snap.device.vendorId.isEmpty() ? QStringLiteral("-") : snap.device.vendorId);

    m_ui->deviceNameValueLabel->setText(snap.device.displayName.isEmpty() ? QStringLiteral("-") : snap.device.displayName);
    m_ui->deviceVendorValueLabel->setText(vendorText);
    m_ui->deviceVidValueLabel->setText(snap.device.vendorId.isEmpty() ? QStringLiteral("-") : snap.device.vendorId);
    m_ui->devicePidValueLabel->setText(snap.device.productId.isEmpty() ? QStringLiteral("-") : snap.device.productId);
    m_ui->deviceConnectedValueLabel->setText(deviceConnectionText(snap.device, m_uiLanguage));
    m_ui->deviceTypeValueLabel->setText(snap.device.connectionType.isEmpty() ? QStringLiteral("-") : snap.device.connectionType);

    const bool isRecording = snap.recording;
    m_ui->startButton->setEnabled(!isRecording);
    m_ui->stopButton->setEnabled(isRecording);

    if (isRecording) {
        m_ui->testStateLabel->setText(uiText("Status: Recording", "状态: 录制中"));

        const qint64 elapsed = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - snap.recordingStartMs;
        m_ui->testMetricsLabel->setText(QStringLiteral("%1: %2 | %3: %4 | %5: %6")
                                            .arg(uiText("Current", "当前"))
                                            .arg(formatHz(snap.currentHz))
                                            .arg(uiText("Avg", "平均"))
                                            .arg(formatHz(snap.avgHz30s))
                                            .arg(uiText("Time", "时间"))
                                            .arg(formatDuration(elapsed)));
        m_ui->testChart->setValues(snap.pollingHistory);
        m_ui->testTrajectory->setPoints(snap.trajectory);
        return;
    }

    m_ui->testStateLabel->setText(uiText("Status: Idle", "状态: 空闲"));
    m_ui->testMetricsLabel->clear();
    if (!last.sessionId.isEmpty()) {
        m_ui->testResultLabel->setText(QStringLiteral("%1 %2: %3 | %4: %5 | %6: %7")
                                           .arg(uiText("Completed!", "已完成!"))
                                           .arg(uiText("Avg", "平均"))
                                           .arg(formatHz(last.summary.avgHz))
                                           .arg(uiText("Score", "评分"))
                                           .arg(formatScore(last.summary.stabilityScore))
                                           .arg(uiText("Time", "时间"))
                                           .arg(formatDuration(last.durationMs)));
    } else {
        m_ui->testResultLabel->setText(uiText("Start a test to collect data.", "开始测试后显示结果。"));
    }
}

void Gg::refreshHistory()
{
    m_controller->reloadHistory();
    const QVector<SessionRecord> records = m_controller->history();

    m_ui->historyTable->clearContents();
    m_ui->historyTable->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const SessionRecord& rec = records.at(i);
        m_ui->historyTable->setItem(i, 0, new QTableWidgetItem(rec.startTimeUtc.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
        m_ui->historyTable->setItem(i, 1, new QTableWidgetItem(testModeToDisplayString(rec.mode, m_uiLanguage)));
        m_ui->historyTable->setItem(i, 2, new QTableWidgetItem(rec.summary.device.displayName));
        m_ui->historyTable->setItem(i, 3, new QTableWidgetItem(formatDuration(rec.durationMs)));
        m_ui->historyTable->setItem(i, 4, new QTableWidgetItem(formatHz(rec.summary.avgHz)));
        m_ui->historyTable->setItem(i, 5, new QTableWidgetItem(formatScore(rec.summary.stabilityScore)));
        m_ui->historyTable->setItem(i, 6, new QTableWidgetItem(formatJitter(rec.summary.jitterValue)));
        auto* actionItem = new QTableWidgetItem(uiText("[Export]", "[导出]"));
        actionItem->setData(Qt::UserRole, rec.sessionId);
        m_ui->historyTable->setItem(i, kHistoryActionColumn, actionItem);
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

    m_ui->compareSelectionLabel->setText(uiText("Selected %1", "已选择 %1").arg(selected.size()));
    m_ui->compareTable->clearContents();
    m_ui->compareTable->setRowCount(selected.size());
    for (int i = 0; i < selected.size(); ++i) {
        const SessionRecord& rec = selected.at(i);
        m_ui->compareTable->setItem(i, 0, new QTableWidgetItem(rec.startTimeUtc.toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
        m_ui->compareTable->setItem(i, 1, new QTableWidgetItem(testModeToDisplayString(rec.mode, m_uiLanguage)));
        m_ui->compareTable->setItem(i, 2, new QTableWidgetItem(rec.summary.device.displayName));
        m_ui->compareTable->setItem(i, 3, new QTableWidgetItem(formatHz(rec.summary.avgHz)));
        m_ui->compareTable->setItem(i, 4, new QTableWidgetItem(formatHz(rec.summary.maxHz)));
        m_ui->compareTable->setItem(i, 5, new QTableWidgetItem(formatHz(rec.summary.minHz)));
        m_ui->compareTable->setItem(i, 6, new QTableWidgetItem(formatHz(rec.summary.stddevHz)));
        m_ui->compareTable->setItem(i, 7, new QTableWidgetItem(formatScore(rec.summary.stabilityScore)));
        m_ui->compareTable->setItem(i, 8, new QTableWidgetItem(formatJitter(rec.summary.jitterValue)));
    }
}

void Gg::onStartTest()
{
    const TestMode mode = static_cast<TestMode>(m_ui->testModeCombo->currentData().toInt());

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
}

void Gg::onExportSelected()
{
    const QModelIndexList selected = m_ui->historyTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::information(this, uiText("Export", "导出"), uiText("Select records first.", "请先选择记录。"));
        return;
    }

    const QString filter = QStringLiteral("CSV (*.csv);;JSON (*.json)");
    const QString fileName = QFileDialog::getSaveFileName(this, uiText("Export", "导出"), QString(), filter);
    if (fileName.isEmpty()) {
        return;
    }

    const bool asJson = fileName.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive);
    const int row = selected.first().row();
    const QString sessionId = m_ui->historyTable->item(row, kHistoryActionColumn)->data(Qt::UserRole).toString();

    QString error;
    if (!m_controller->exportSession(sessionId, fileName, asJson, &error)) {
        QMessageBox::warning(this, uiText("Error", "错误"), error);
        return;
    }

    QMessageBox::information(this, uiText("Success", "成功"), uiText("Exported to:\n%1", "已导出到:\n%1").arg(fileName));
}

void Gg::onAddToCompare()
{
    const QModelIndexList selected = m_ui->historyTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::information(this, uiText("Compare", "对比"), uiText("Select records first.", "请先选择记录。"));
        return;
    }

    for (const QModelIndex& idx : selected) {
        const QString sessionId = m_ui->historyTable->item(idx.row(), kHistoryActionColumn)->data(Qt::UserRole).toString();
        if (!m_compareSessionIds.contains(sessionId)) {
            m_compareSessionIds.append(sessionId);
        }
    }

    refreshCompare();
    m_ui->navigationList->setCurrentRow(3);
}

void Gg::onSwitchToEnglish()
{
    setLanguage(UiLanguage::English);
}

void Gg::onSwitchToChinese()
{
    setLanguage(UiLanguage::Chinese);
}

void Gg::changeEvent(QEvent* event)
{
    if (event && event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
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
        timer->start(kRefreshIntervalMs);

        QTimer::singleShot(0, this, [this] {
            QString error;
            if (!m_controller->bindInputWindow(winId(), &error)) {
                QMessageBox::warning(this, uiText("Error", "错误"), error);
            }

            updateUiActiveState();
            refreshHistory();
            refreshCompare();
            refreshAll();
        });
    }

    QMainWindow::showEvent(event);
}

void Gg::updateUiActiveState()
{
    m_controller->setUiActive(isActiveWindow() && !isMinimized());
}
