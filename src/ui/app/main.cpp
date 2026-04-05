#include "Gg.h"

#include <QtGui/QIcon>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/Gg/app_icon.png")));
    Gg window;
    window.show();
    return app.exec();
}
