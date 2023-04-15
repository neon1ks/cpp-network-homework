#include <QtWidgets>

#include "TcpServer.h"

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);

    TcpServer server(54329);

    server.show();

    return QApplication::exec();
}
