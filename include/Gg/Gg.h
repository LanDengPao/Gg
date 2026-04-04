#pragma once

#include "AppController.h"
#include "SparklineWidget.h"
#include "TrajectoryWidget.h"

#include <QComboBox>
#include <QHash>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>

class QAction;
class QFormLayout;
class QGroupBox;

class Gg : public QMainWindow
{
    Q_OBJECT

public:
    explicit Gg(QWidget* parent = nullptr);
    ~Gg() override;

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onStartTest();
    void onStopTest();
    void onExportSelected();
    void onAddToCompare();
    void onSwitchToEnglish();
    void onSwitchToChinese();

private:
    AppController* m_controller = nullptr;
    QHash<quintptr, DeviceInfo> m_deviceCache;
    QStringList m_compareSessionIds;
    bool m_rawInputRegistered = false;
    UiLanguage m_uiLanguage = UiLanguage::English;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_modeLabel = nullptr;
    QLabel* m_deviceLabel = nullptr;
    QLabel* m_backgroundLabel = nullptr;
    QLabel* m_workspaceLabel = nullptr;

    QListWidget* m_navList = nullptr;
    QStackedWidget* m_stack = nullptr;
    QMenu* m_languageMenu = nullptr;
    QAction* m_englishAction = nullptr;
    QAction* m_chineseAction = nullptr;

    QLabel* m_lastSessionTitleLabel = nullptr;
    QLabel* m_lastSessionLabel = nullptr;
    QPushButton* m_lastSessionDetailsButton = nullptr;
    QPushButton* m_lastSessionCompareButton = nullptr;
    QLabel* m_currentDeviceTitleLabel = nullptr;
    QPushButton* m_deviceMoreButton = nullptr;
    QLabel* m_dashboardHzTitleLabel = nullptr;
    QLabel* m_dashboardAvgTitleLabel = nullptr;
    QLabel* m_dashboardStdTitleLabel = nullptr;
    QLabel* m_dashboardSpeedTitleLabel = nullptr;
    QLabel* m_dashboardJitterTitleLabel = nullptr;
    QLabel* m_dashboardClicksTitleLabel = nullptr;
    QLabel* m_dashboardHzUnitLabel = nullptr;
    QLabel* m_dashboardAvgUnitLabel = nullptr;
    QLabel* m_dashboardStdUnitLabel = nullptr;
    QLabel* m_dashboardSpeedUnitLabel = nullptr;
    QLabel* m_dashboardJitterUnitLabel = nullptr;
    QLabel* m_dashboardClicksUnitLabel = nullptr;
    QLabel* m_dashboardChartTitleLabel = nullptr;
    QLabel* m_dashboardTrajectoryTitleLabel = nullptr;
    QLabel* m_dashboardHzLabel = nullptr;
    QLabel* m_dashboardAvgLabel = nullptr;
    QLabel* m_dashboardStdLabel = nullptr;
    QLabel* m_dashboardSpeedLabel = nullptr;
    QLabel* m_dashboardJitterLabel = nullptr;
    QLabel* m_dashboardClicksLabel = nullptr;
    SparklineWidget* m_dashboardChart = nullptr;
    TrajectoryWidget* m_dashboardTrajectory = nullptr;

    QGroupBox* m_testModeGroup = nullptr;
    QComboBox* m_testModeCombo = nullptr;
    QGroupBox* m_controlGroup = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QLabel* m_testStateLabel = nullptr;
    QLabel* m_testMetricsLabel = nullptr;
    QGroupBox* m_metricsGroup = nullptr;
    QLabel* m_testResultLabel = nullptr;
    QGroupBox* m_chartGroup = nullptr;
    SparklineWidget* m_testChart = nullptr;
    TrajectoryWidget* m_testTrajectory = nullptr;

    QLabel* m_historyTitleLabel = nullptr;
    QTableWidget* m_historyTable = nullptr;
    QPushButton* m_historyExportButton = nullptr;
    QPushButton* m_historyCompareButton = nullptr;
    QPushButton* m_historyRefreshButton = nullptr;
    QTableWidget* m_compareTable = nullptr;
    QLabel* m_compareTitleLabel = nullptr;
    QLabel* m_compareSelectionLabel = nullptr;
    QPushButton* m_compareAddButton = nullptr;
    QPushButton* m_compareClearButton = nullptr;

    QLabel* m_deviceTitleLabel = nullptr;
    QLabel* m_deviceInfoLabel = nullptr;
    QGroupBox* m_deviceBasicGroup = nullptr;
    QGroupBox* m_deviceStatusGroup = nullptr;
    QLabel* m_deviceNameValueLabel = nullptr;
    QLabel* m_deviceVendorValueLabel = nullptr;
    QLabel* m_deviceVidValueLabel = nullptr;
    QLabel* m_devicePidValueLabel = nullptr;
    QLabel* m_deviceConnectedValueLabel = nullptr;
    QLabel* m_deviceTypeValueLabel = nullptr;
    QFormLayout* m_deviceBasicLayout = nullptr;
    QFormLayout* m_deviceStatusLayout = nullptr;

    void buildUi();
    QWidget* buildDashboardPage();
    QWidget* buildTestPage();
    QWidget* buildHistoryPage();
    QWidget* buildComparePage();
    QWidget* buildDevicePage();

    void registerRawInput();
    void setLanguage(UiLanguage language);
    void retranslateUi();
    void updateUiActiveState();
    void refreshAll();
    void refreshHistory();
    void refreshCompare();
    void handleRawInput(void* lParam);
    DeviceInfo resolveDeviceInfo(void* handle);
    static qint64 nowMicroseconds();
};
