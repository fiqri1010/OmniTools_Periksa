#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("CustomsInspectorSuite");
    app.setOrganizationName("CustomsPriok");

    MainWindow window;
    window.show();

    return app.exec();
}
