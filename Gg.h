#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Gg.h"

class Gg : public QMainWindow
{
    Q_OBJECT

public:
    Gg(QWidget *parent = nullptr);
    ~Gg();

private:
    Ui::GgClass ui;
};

