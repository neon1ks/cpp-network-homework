
#include <QtWidgets>

#include "TcpClient.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TcpClient client("127.0.0.1", 2323);
    client.show();
    return app.exec();
}
