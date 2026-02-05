#include "EventPlannerPro.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    EventPlannerPro window;
    window.show();
    return app.exec();
}
