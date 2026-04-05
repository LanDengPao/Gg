#include "Gg.h"

#include "ui_Gg.h"

#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QChar>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
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

QString deviceConnectionText(const DeviceInfo& device)
{
    return device.connected
        ? QCoreApplication::translate("Gg", "Connected")
        : QCoreApplication::translate("Gg", "Disconnected");
}

QString chineseTranslationFilePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("translations/Gg_zh_CN.qm"));
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
    m_ui->navigationList->setFocusPolicy(Qt::NoFocus);
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
    // 首次启动时根据系统区域设置选择默认界面语言。
    if (QLocale::system().name().startsWith(QStringLiteral("zh"), Qt::CaseInsensitive)) {
        activateChineseTranslation();
    } else {
        activateEnglishTranslation();
    }
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

bool Gg::activateChineseTranslation()
{
    // 先移除旧翻译器，避免重复安装造成翻译结果混杂。
    qApp->removeTranslator(&m_translator);
    if (!m_translator.load(chineseTranslationFilePath())) {
        m_isChineseTranslationActive = false;
        retranslateUi();
        refreshAll();
        refreshHistory();
        refreshCompare();
        return false;
    }

    qApp->installTranslator(&m_translator);
    m_isChineseTranslationActive = true;
    retranslateUi();
    refreshAll();
    refreshHistory();
    refreshCompare();
    return true;
}

void Gg::activateEnglishTranslation()
{
    qApp->removeTranslator(&m_translator);
    m_isChineseTranslationActive = false;
    retranslateUi();
    refreshAll();
    refreshHistory();
    refreshCompare();
}

void Gg::retranslateUi()
{
    m_ui->retranslateUi(this);

    setWindowTitle(tr("Mouse Data"));

    setListItemText(m_ui->navigationList, 0, tr("Dashboard"));
    setListItemText(m_ui->navigationList, 1, tr("Test"));
    setListItemText(m_ui->navigationList, 2, tr("History"));
    setListItemText(m_ui->navigationList, 3, tr("Compare"));
    setListItemText(m_ui->navigationList, 4, tr("Device"));

    m_ui->menuLanguage->setTitle(tr("Language"));
    m_ui->actionEnglish->setText(QStringLiteral("English"));
    m_ui->actionEnglish->setChecked(!m_isChineseTranslationActive);
    m_ui->actionChinese->setText(QStringLiteral("中文"));
    m_ui->actionChinese->setChecked(m_isChineseTranslationActive);

    if (m_statusLabel) m_statusLabel->setText(tr("Idle"));
    if (m_modeLabel) m_modeLabel->setText(tr("Monitoring"));
    if (m_deviceLabel) m_deviceLabel->setText(tr("No Device"));
    if (m_workspaceLabel) {
        m_workspaceLabel->setText(tr("Workspace: %1").arg(m_controller->workspacePath()));
    }

    m_ui->lastSessionTitleLabel->setText(tr("Last Session"));
    m_ui->lastSessionLabel->setText(tr("No Records"));
    m_ui->lastSessionDetailsButton->setText(tr("Details"));
    m_ui->lastSessionCompareButton->setText(tr("Compare"));

    m_ui->currentDeviceTitleLabel->setText(tr("Current Device"));
    m_ui->deviceInfoLabel->setText(tr("No Mouse"));
    m_ui->deviceMoreButton->setText(tr("More"));

    m_ui->dashboardHzTitleLabel->setText(tr("Current Hz"));
    m_ui->dashboardAvgTitleLabel->setText(tr("1s Avg"));
    m_ui->dashboardStdTitleLabel->setText(tr("Std Dev"));
    m_ui->dashboardSpeedTitleLabel->setText(tr("Speed"));
    m_ui->dashboardJitterTitleLabel->setText(tr("Jitter"));
    m_ui->dashboardClicksTitleLabel->setText(tr("Clicks"));
    m_ui->dashboardHzUnitLabel->setText(QStringLiteral("Hz"));
    m_ui->dashboardAvgUnitLabel->setText(QStringLiteral("Hz"));
    m_ui->dashboardStdUnitLabel->setText(QStringLiteral("Hz"));
    m_ui->dashboardSpeedUnitLabel->setText(QStringLiteral("px/s"));
    m_ui->dashboardJitterUnitLabel->setText(QStringLiteral("px"));
    m_ui->dashboardClicksUnitLabel->setText(QString());
    m_ui->dashboardChartTitleLabel->setText(tr("Polling Chart"));
    m_ui->dashboardTrajectoryTitleLabel->setText(tr("Trajectory"));

    m_ui->testModeGroup->setTitle(tr("Test Mode"));
    m_ui->testModeCombo->setItemText(0, testModeToDisplayString(TestMode::PollingRate));
    m_ui->testModeCombo->setItemText(1, testModeToDisplayString(TestMode::TrajectoryStability));
    m_ui->controlGroup->setTitle(tr("Control"));
    m_ui->startButton->setText(tr("Start"));
    m_ui->stopButton->setText(tr("Stop"));
    m_ui->testStateLabel->setText(tr("Status: Idle"));
    m_ui->metricsGroup->setTitle(tr("Metrics"));
    m_ui->testResultLabel->setText(tr("Start a test to collect data."));
    m_ui->chartGroup->setTitle(tr("Chart"));

    m_ui->historyTitleLabel->setText(tr("History"));
    m_ui->historyExportButton->setText(tr("Export"));
    m_ui->historyCompareButton->setText(tr("Compare"));
    m_ui->historyRefreshButton->setText(tr("Refresh"));
    m_ui->historyTable->setHorizontalHeaderLabels({
        tr("Time"),
        tr("Mode"),
        tr("Device"),
        tr("Duration"),
        tr("Avg Hz"),
        tr("Stability"),
        tr("Jitter"),
        tr("Action")
    });

    m_ui->compareTitleLabel->setText(tr("Compare"));
    m_ui->compareSelectionLabel->setText(tr("Selected %1").arg(0));
    m_ui->compareAddButton->setText(tr("Add"));
    m_ui->compareClearButton->setText(tr("Clear"));
    m_ui->compareTable->setHorizontalHeaderLabels({
        tr("Time"),
        tr("Mode"),
        tr("Device"),
        tr("Avg Hz"),
        tr("Max Hz"),
        tr("Min Hz"),
        tr("Std Dev"),
        tr("Stability"),
        tr("Jitter")
    });

    m_ui->deviceTitleLabel->setText(tr("Device"));
    m_ui->deviceBasicGroup->setTitle(tr("Basic"));
    m_ui->deviceStatusGroup->setTitle(tr("Status"));
    m_ui->deviceNameFieldLabel->setText(tr("Name:"));
    m_ui->deviceVendorFieldLabel->setText(tr("Vendor:"));
    m_ui->deviceVidFieldLabel->setText(QStringLiteral("VID:"));
    m_ui->devicePidFieldLabel->setText(QStringLiteral("PID:"));
    m_ui->deviceConnectionFieldLabel->setText(tr("Connection:"));
    m_ui->deviceTypeFieldLabel->setText(tr("Type:"));
}

void Gg::refreshAll()
{
    if (!m_controller || !m_statusLabel || !m_modeLabel) {
        return;
    }

    const LiveSnapshot snap = m_controller->snapshot();
    const SessionRecord last = m_controller->lastSession();

    m_statusLabel->setText(sessionStatusToDisplayString(snap.runStatus));
    m_modeLabel->setText(testModeToDisplayString(snap.currentMode));
    updateStatusTone(snap.recording);

    m_deviceLabel->setText(snap.device.displayName.isEmpty() ? tr("No Device") : snap.device.displayName);
    m_backgroundLabel->setText((snap.recording && !snap.uiActive) ? tr("Recording in Background") : QString());

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
                                            .arg(testModeToDisplayString(last.mode))
                                            .arg(tr("Avg"))
                                            .arg(last.summary.avgHz, 0, 'f', 0)
                                            .arg(tr("Score"))
                                            .arg(formatScore(last.summary.stabilityScore)));
    } else {
        m_ui->lastSessionLabel->setText(tr("No Records"));
    }

    if (!snap.device.deviceId.isEmpty()) {
        m_ui->deviceInfoLabel->setText(QStringLiteral("%1\nVID: %2 | PID: %3")
                                           .arg(snap.device.displayName)
                                           .arg(snap.device.vendorId)
                                           .arg(snap.device.productId));
    } else {
        m_ui->deviceInfoLabel->setText(tr("No Mouse"));
    }

    const QString vendorText = !snap.device.manufacturer.isEmpty()
        ? snap.device.manufacturer
        : (snap.device.vendorId.isEmpty() ? QStringLiteral("-") : snap.device.vendorId);

    m_ui->deviceNameValueLabel->setText(snap.device.displayName.isEmpty() ? QStringLiteral("-") : snap.device.displayName);
    m_ui->deviceVendorValueLabel->setText(vendorText);
    m_ui->deviceVidValueLabel->setText(snap.device.vendorId.isEmpty() ? QStringLiteral("-") : snap.device.vendorId);
    m_ui->devicePidValueLabel->setText(snap.device.productId.isEmpty() ? QStringLiteral("-") : snap.device.productId);
    m_ui->deviceConnectedValueLabel->setText(deviceConnectionText(snap.device));
    m_ui->deviceTypeValueLabel->setText(snap.device.connectionType.isEmpty() ? QStringLiteral("-") : snap.device.connectionType);

    const bool isRecording = snap.recording;
    m_ui->startButton->setEnabled(!isRecording);
    m_ui->stopButton->setEnabled(isRecording);

    if (isRecording) {
        m_ui->testStateLabel->setText(tr("Status: Recording"));

        const qint64 elapsed = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - snap.recordingStartMs;
        m_ui->testMetricsLabel->setText(QStringLiteral("%1: %2 | %3: %4 | %5: %6")
                                            .arg(tr("Current"))
                                            .arg(formatHz(snap.currentHz))
                                            .arg(tr("Avg"))
                                            .arg(formatHz(snap.avgHz30s))
                                            .arg(tr("Time"))
                                            .arg(formatDuration(elapsed)));
        m_ui->testChart->setValues(snap.pollingHistory);
        m_ui->testTrajectory->setPoints(snap.trajectory);
        return;
    }

    m_ui->testStateLabel->setText(tr("Status: Idle"));
    m_ui->testMetricsLabel->clear();
    if (!last.sessionId.isEmpty()) {
        m_ui->testResultLabel->setText(QStringLiteral("%1 %2: %3 | %4: %5 | %6: %7")
                                           .arg(tr("Completed!"))
                                           .arg(tr("Avg"))
                                           .arg(formatHz(last.summary.avgHz))
                                           .arg(tr("Score"))
                                           .arg(formatScore(last.summary.stabilityScore))
                                           .arg(tr("Time"))
                                           .arg(formatDuration(last.durationMs)));
    } else {
        m_ui->testResultLabel->setText(tr("Start a test to collect data."));
    }
}

void Gg::refreshHistory()
{
    // 每次刷新都从仓储层重新加载，确保表格反映最新落盘结果。
    m_controller->reloadHistory();
    const QVector<SessionRecord> records = m_controller->history();

    m_ui->historyTable->clearContents();
    m_ui->historyTable->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const SessionRecord& rec = records.at(i);
        m_ui->historyTable->setItem(i, 0, new QTableWidgetItem(rec.startTimeUtc.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
        m_ui->historyTable->setItem(i, 1, new QTableWidgetItem(testModeToDisplayString(rec.mode)));
        m_ui->historyTable->setItem(i, 2, new QTableWidgetItem(rec.summary.device.displayName));
        m_ui->historyTable->setItem(i, 3, new QTableWidgetItem(formatDuration(rec.durationMs)));
        m_ui->historyTable->setItem(i, 4, new QTableWidgetItem(formatHz(rec.summary.avgHz)));
        m_ui->historyTable->setItem(i, 5, new QTableWidgetItem(formatScore(rec.summary.stabilityScore)));
        m_ui->historyTable->setItem(i, 6, new QTableWidgetItem(formatJitter(rec.summary.jitterValue)));
        auto* actionItem = new QTableWidgetItem(tr("[Export]"));
        actionItem->setData(Qt::UserRole, rec.sessionId);
        m_ui->historyTable->setItem(i, kHistoryActionColumn, actionItem);
    }
}

void Gg::refreshCompare()
{
    // 对比页基于历史记录做筛选，保持与历史页数据来源一致。
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

    m_ui->compareSelectionLabel->setText(tr("Selected %1").arg(selected.size()));
    m_ui->compareTable->clearContents();
    m_ui->compareTable->setRowCount(selected.size());
    for (int i = 0; i < selected.size(); ++i) {
        const SessionRecord& rec = selected.at(i);
        m_ui->compareTable->setItem(i, 0, new QTableWidgetItem(rec.startTimeUtc.toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
        m_ui->compareTable->setItem(i, 1, new QTableWidgetItem(testModeToDisplayString(rec.mode)));
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
        QMessageBox::warning(this, tr("Error"), error);
        return;
    }

    refreshAll();
}

void Gg::onStopTest()
{
    QString error;
    if (!m_controller->stopTest(&error)) {
        QMessageBox::warning(this, tr("Error"), error);
        return;
    }

    refreshAll();
    refreshHistory();
}

void Gg::onExportSelected()
{
    const QModelIndexList selected = m_ui->historyTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Select records first."));
        return;
    }

    const QString filter = QStringLiteral("CSV (*.csv);;JSON (*.json)");
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Export"), QString(), filter);
    if (fileName.isEmpty()) {
        return;
    }

    const bool asJson = fileName.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive);
    const int row = selected.first().row();
    const QString sessionId = m_ui->historyTable->item(row, kHistoryActionColumn)->data(Qt::UserRole).toString();

    QString error;
    if (!m_controller->exportSession(sessionId, fileName, asJson, &error)) {
        QMessageBox::warning(this, tr("Error"), error);
        return;
    }

    QMessageBox::information(this, tr("Success"), tr("Exported to:\n%1").arg(fileName));
}

void Gg::onAddToCompare()
{
    const QModelIndexList selected = m_ui->historyTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::information(this, tr("Compare"), tr("Select records first."));
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
    activateEnglishTranslation();
}

void Gg::onSwitchToChinese()
{
    if (!activateChineseTranslation()) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to load translation file: %1").arg(chineseTranslationFilePath()));
    }
}

void Gg::changeEvent(QEvent* event)
{
    if (event && event->type() == QEvent::LanguageChange) {
        retranslateUi();
        refreshAll();
        refreshHistory();
        refreshCompare();
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

        // 绑定 Raw Input 需要窗口句柄已经完成创建，因此延后到首帧后执行。
        QTimer::singleShot(0, this, [this] {
            QString error;
            if (!m_controller->bindInputWindow(winId(), &error)) {
                QMessageBox::warning(this, tr("Error"), error);
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
