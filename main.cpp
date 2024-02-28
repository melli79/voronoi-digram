
#include <QApplication>

#include "VoronoiGraph.h"

int main(int nArgs, char** args) {
    QApplication app(nArgs, args);
    VoronoiGraph window;
    window.show();

    return app.exec();
}
