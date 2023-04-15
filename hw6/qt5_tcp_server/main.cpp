#include <QtWidgets>

#include "TcpServer.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TcpServer server(2323);
    server.show();
    return app.exec();
}
