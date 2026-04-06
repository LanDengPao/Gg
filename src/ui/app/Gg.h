#pragma once

#include "AppController.h"

#include <QMainWindow>
#include <QStringList>
#include <QTranslator>

#include <memory>

class QLabel;
class QComboBox;
class QSpinBox;
class QEvent;
class QShowEvent;
class QWidget;

namespace Ui {
class Gg;
}

// 管理主窗口，并保持各个仪表盘控件与控制器状态同步。
class Gg : public QMainWindow
{
    Q_OBJECT

public:
    explicit Gg(QWidget* parent = nullptr);
    ~Gg() override;

protected:
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
    // 完成界面的一次性初始化，包括控件配置、信号连接和默认语言选择。
    void initializeUi();
    // 创建应用统一使用的状态栏标签。
    void registerStatusBar();
    // 加载并应用内置的 QSS 主题。
    void applyAppStyleSheet();
    // 强制 Qt 重新计算控件的动态 QSS 属性。
    void updateWidgetStyle(QWidget* widget) const;
    // 根据是否正在录制更新状态栏的视觉状态。
    void updateStatusTone(bool recording);
    // 加载内置的简体中文翻译文件。
    bool activateChineseTranslation();
    // 移除自定义翻译器并回退到源码语言。
    void activateEnglishTranslation();
    // 在语言切换后刷新所有静态界面文案。
    void retranslateUi();
    // 上报主窗口是否激活，用于提示后台采集状态。
    void updateUiActiveState();
    // 根据最新实时快照刷新仪表盘和设备信息控件。
    void refreshAll();
    // 初始化轨迹图显示参数控件和联动逻辑。
    void initializeTrajectoryControls();
    // 根据当前着色模式启停相关参数控件。
    void updateTrajectoryControlState();
    // 将当前界面的轨迹显示参数应用到两个轨迹控件。
    void updateTrajectoryWidgets(const QVector<TimedPoint>& points);
    // 基于持久化的会话记录重建历史表格。
    void refreshHistory();
    // 基于当前选中的会话编号重建对比表格。
    void refreshCompare();

    std::unique_ptr<Ui::Gg> m_ui;
    AppController* m_controller = nullptr;
    QStringList m_compareSessionIds;
    bool m_isChineseTranslationActive = false;
    QTranslator m_translator;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_modeLabel = nullptr;
    QLabel* m_deviceLabel = nullptr;
    QLabel* m_backgroundLabel = nullptr;
    QLabel* m_workspaceLabel = nullptr;
};
