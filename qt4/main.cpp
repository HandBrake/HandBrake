#include <QtGui>

#include "qhandbrakewizard.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QHandBrakeWizard *wizard = new QHandBrakeWizard();

    wizard->show();
    return app.exec();
}
