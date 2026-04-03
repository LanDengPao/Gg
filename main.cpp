#include "Gg.h"
#include <QtWidgets/QApplication>
#include "LambdaThread.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Gg window;
    window.show();

    // do the wait on another thread
    LambdaThread* thread = new LambdaThread([]() {
       
        
    });
    thread->setName(QString("get mouse interval"));
    thread->selfDelete(true);
    thread->start();
    return app.exec();
}
