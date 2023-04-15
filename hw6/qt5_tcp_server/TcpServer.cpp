#include <QMessageBox>
#include <QTcpServer>
#include <QTextEdit>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QLabel>

#include <QFile>
#include <QDir>

#include <QSslSocket>

#include "TcpServer.h"

class QTcpServer;


TcpServer::TcpServer(int nPort, QWidget* pwgt): QWidget(pwgt),
    next_block_size_(0)
{
    tcp_server_ptr_ = new SslServer(this);
    tcp_server_ptr_->set_ssl_local_certificate("sslserver.pem");
    tcp_server_ptr_->set_ssl_private_key("sslserver.key");
    tcp_server_ptr_->set_ssl_protocol(QSsl::TlsV1_2);
    
    if (!tcp_server_ptr_->listen(QHostAddress::Any, nPort))
    {
        QMessageBox::critical(0,
                              "Server Error",
                              "Unable to start the server:"
                                  + tcp_server_ptr_->errorString()
                              );
        tcp_server_ptr_->close();
        return;
    }
    connect(tcp_server_ptr_, SIGNAL(newConnection()),
            this, SLOT(slotNewConnection())
            );
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


void TcpServer::slot_disconnect()
{
    auto* pClientSocket = dynamic_cast<QSslSocket*>(sender());
    pClientSocket->disconnectFromHost();
    //pClientSocket->waitForDisconnected();
}


void TcpServer::slot_new_connection()
{
    QSslSocket* pClientSocket = dynamic_cast<QSslSocket*>(tcp_server_ptr_->nextPendingConnection());
    connect(pClientSocket, SIGNAL(disconnected()),
            this, SLOT(disconnect())
            );
    connect(pClientSocket, SIGNAL(readyRead()),
            this, SLOT(slotReadClient())
            );
    send_to_client(pClientSocket, "Server Response: Connected!");
}


void TcpServer::slot_read_client()
{
    QSslSocket* pClientSocket = (QSslSocket*) sender();
    QDataStream in(pClientSocket);
    in.setVersion(QDataStream::Qt_5_3);
    for (;;)
    {
        if (!next_block_size_)
        {
            if (pClientSocket->bytesAvailable() < sizeof(quint16))
            {
                break;
            }
            in >> next_block_size_;
        }
        if (pClientSocket->bytesAvailable() < next_block_size_)
        {
            break;
        }
        QString str;
        in >> str;

        const QString strMessage = "Client has sent - " + str;
        txt_ptr_->append(strMessage);
        next_block_size_ = 0;
        send_to_client(pClientSocket,
                     "Server Response: Received \"" +
                     str +
                     "\""
                     );

        // Command analyse
        QTextStream iss(&str,QIODevice::ReadOnly);
        QString command = "\0";
        QString filename = "\0";
        QString fiction = "\0";
        size_t fict = 0;
        size_t size_fict = 0;
        iss >> command >> filename >> fiction >> size_fict;

        if ("exit" == command)
        {
            txt_ptr_->append("Breaking");
            send_to_client(pClientSocket,command);
            this->close();


        }

        if ("get" == command && sizeof(filename) > 0)
        {
          if ("beans" == fiction)
           {
             fict = 1;
           }
           else if ("size" == fiction)
           {
             fict = 2;
           }
           
           send_to_client(pClientSocket,command);
           send_to_file(pClientSocket,filename,fict,size_fict);

        }
    }
}


void TcpServer::send_to_client(QSslSocket *pSocket, const QString &str)
{
    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << quint16(0)/* << QTime::currentTime()*/ << str;
    out.device()->seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));
    pSocket->write(arrBlock);
}


bool TcpServer::send_to_file(QSslSocket *pSocket, const QString &filename,
                          const size_t &fict, const size_t &size_fict)
{
    if (!filename.size())
    {
        return false;
    }
    QString path = QDir::current().path()+"/"+filename;
    QFile appFile(path);
    appFile.open(QFile::ReadOnly);
    if (!appFile.exists() && !appFile.isOpen())
    {
        return false;
    }

    size_t size_fict_=size_fict;
    size_t count = appFile.size();

    if (size_fict_ > count)
        size_fict_ = count;

    if (1 == fict) {
        appFile.seek(size_fict_);
        count=count-size_fict_;
    } else if (2 == fict) {
        count = size_fict_;
    }

    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << quint32(0) << appFile.fileName();

    QByteArray q = appFile.read(count);
    arrBlock.append(q);
    appFile.close();

    out.device()->seek(0);
    out << quint32(arrBlock.size() - sizeof(quint32));

    qint64 x=0;
    while (x < arrBlock.size())
    {
        qint64 y = pSocket->write(arrBlock);
        x += y;
    }
    return true;
}
