#include "swftohtml5.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    SwfToHtml5 *wgt = new SwfToHtml5();

    wgt->show();

    return app.exec();
}

