#pragma once

#include "AppController.h"

#include <QMainWindow>
#include <QStringList>

#include <memory>

class QLabel;
class QEvent;
class QShowEvent;
class QWidget;

namespace Ui {
class Gg;
}

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
    void initializeUi();
    void registerStatusBar();
    void applyAppStyleSheet();
    void updateWidgetStyle(QWidget* widget) const;
    void updateStatusTone(bool recording);
    void setLanguage(UiLanguage language);
    void retranslateUi();
    void updateUiActiveState();
    void refreshAll();
    void refreshHistory();
    void refreshCompare();

    std::unique_ptr<Ui::Gg> m_ui;
    AppController* m_controller = nullptr;
    QStringList m_compareSessionIds;
    UiLanguage m_uiLanguage = UiLanguage::English;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_modeLabel = nullptr;
    QLabel* m_deviceLabel = nullptr;
    QLabel* m_backgroundLabel = nullptr;
    QLabel* m_workspaceLabel = nullptr;
};
