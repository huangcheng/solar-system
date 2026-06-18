#include "globe/GlobeWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    GlobeWindow w;
    w.show();
    return app.exec();
}
