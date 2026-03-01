#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QSize>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/assets/gpuopen.ico"));
    // apply the stylesheet
    QFile styleFile(":/stylesheets/main.qss");
    styleFile.open(QFile::ReadOnly);
    a.setStyleSheet(QString(styleFile.readAll()));
    // apply the font
    const int fontId = QFontDatabase::addApplicationFont(":/assets/fonts/TitilliumWeb-Regular.ttf");
    if (fontId != -1)
        a.setFont(QFont(QFontDatabase::applicationFontFamilies(fontId).at(0), 10));
    // show the window
    MainWindow w;
    w.resize(QSize(320, 160));
    w.show();
    return a.exec();
}
