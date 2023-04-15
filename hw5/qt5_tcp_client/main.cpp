#include <QtWidgets>

#include "TcpClient.h"


int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);

    TcpClient client("localhost", 54329);

    client.show();

    return QApplication::exec();
}
