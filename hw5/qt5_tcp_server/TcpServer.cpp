#include <QLabel>
#include <QMessageBox>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QVBoxLayout>

#include <QDir>
#include <QFile>

#include "TcpServer.h"

class QTcpServer;


TcpServer::TcpServer(int nPort, QWidget* pwgt)
    : QWidget(pwgt)
    , next_block_size_(0)
{
    tcp_server_ptr_ = new QTcpServer(this);
    if (!tcp_server_ptr_->listen(QHostAddress::Any, nPort))
    {
        QMessageBox::critical(nullptr, "Server Error", "Unable to start the server:" + tcp_server_ptr_->errorString());
        tcp_server_ptr_->close();
        return;
    }
    connect(tcp_server_ptr_, &QTcpServer::newConnection, this, &TcpServer::slot_new_connection);
    txt_ptr_ = new QTextEdit;
    txt_ptr_->setReadOnly(true);

    auto* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Server</H1>"));
    pvbxLayout->addWidget(txt_ptr_);
    setLayout(pvbxLayout);
    
    txt_ptr_->append("Running echo server...");
    txt_ptr_->append("Command:");
    txt_ptr_->append(" exit - to quit");
    txt_ptr_->append(" get <filename> <beans/size> <number>");
}


void TcpServer::slot_new_connection()
{
    QTcpSocket* pClientSocket = tcp_server_ptr_->nextPendingConnection();
    connect(pClientSocket, &QAbstractSocket::disconnected, pClientSocket, &QObject::deleteLater);
    connect(pClientSocket, &QIODevice::readyRead, this, &TcpServer::slot_read_client);
    send_to_client(pClientSocket, "Server Response: Connected!");
}


void TcpServer::slot_read_client()
{
    auto* client_socket_ptr = dynamic_cast<QTcpSocket*>(sender());

    QDataStream in(client_socket_ptr);
    in.setVersion(QDataStream::Qt_5_3);
    for (;;)
    {
        if (0U == next_block_size_)
        {
            if (client_socket_ptr->bytesAvailable() < sizeof(quint16))
            {
                break;
            }
            in >> next_block_size_;
        }
        if (client_socket_ptr->bytesAvailable() < next_block_size_)
        {
            break;
        }
        QString str;
        in >> str;

        const QString message = "Client has sent - " + str;
        txt_ptr_->append(message);
        next_block_size_ = 0;
        send_to_client(client_socket_ptr, "Server Response: Received \"" + str + "\"");

        // Command analyse
        QTextStream iss(&str, QIODevice::ReadOnly);
        QString     command   = "\0";
        QString     filename  = "\0";
        QString     fiction   = "\0";
        size_t      fict      = 0;
        size_t      size_fict = 0;
        iss >> command >> filename >> fiction >> size_fict;

        if ("exit" == command)
        {
            txt_ptr_->append("Breaking");
            send_to_client(client_socket_ptr, command);
            this->close();
        }

        if ("get" == command && filename.size() > 0)
        {
            if ("beans" == fiction)
            {
                fict = 1;
            }
            else if ("size" == fiction)
            {
                fict = 2;
            }
            
            send_to_client(client_socket_ptr, command);
            send_to_file(client_socket_ptr, filename, fict, size_fict);
        }
    }
}


void TcpServer::send_to_client(QTcpSocket* socket_ptr, const QString& str)
{
    QByteArray  block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << static_cast<quint16>(0) << str;
    out.device()->seek(0);
    out << static_cast<quint16>(block.size() - sizeof(quint16));
    socket_ptr->write(block);
}


bool TcpServer::send_to_file(QTcpSocket* socket_ptr, const QString& filename, const size_t& fict, const size_t& size)
{
    if (0 == filename.size())
    {
        return false;
    }
    const QString path = QDir::current().path() + "/" + filename;

    QFile file(path);
    file.open(QFile::ReadOnly);
    if (!file.exists() && !file.isOpen())
    {
        return false;
    }

    size_t size_fict = size;
    size_t count      = file.size();

    if (size_fict > count)
        size_fict = count;

    if (1 == fict)
    {
        file.seek(size_fict);
        count = count - size_fict;
    }
    else if (2 == fict)
    {
        count = size_fict;
    }

    QByteArray  block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << static_cast<quint32>(0) << file.fileName();
    const QByteArray q = file.read(count);
    block.append(q);
    file.close();

    out.device()->seek(0);
    out << static_cast<quint32>(block.size() - sizeof(quint32));

    qint64 x = 0;
    while (x < block.size())
    {
        const qint64 y = socket_ptr->write(block);
        x += y;
    }
    return true;
}
