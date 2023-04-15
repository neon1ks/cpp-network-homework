#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "TcpClient.h"

TcpClient::TcpClient(const QString& host, int port, QWidget* pwgt)
    : QWidget(pwgt)
    , next_block_size_(0)
    , block_size_(0)
{
    tcp_socket_ptr_ = new QTcpSocket(this);
    tcp_socket_ptr_->connectToHost(host, port);
    connect(tcp_socket_ptr_, &QAbstractSocket::connected, this, &TcpClient::slot_connected);
    connect(tcp_socket_ptr_, &QIODevice::readyRead, this, &TcpClient::slot_ready_read);
    connect(
        tcp_socket_ptr_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotError(QAbstractSocket::SocketError)));
    txt_info_ptr_  = new QTextEdit;
    txt_input_ptr_ = new QLineEdit;
    txt_info_ptr_->setReadOnly(true);
    auto* pcmd = new QPushButton("&Send");
    connect(pcmd, &QAbstractButton::clicked, this, &TcpClient::slot_send_to_server);
    connect(txt_input_ptr_, &QLineEdit::returnPressed, this, &TcpClient::slot_send_to_server);
    auto* pvbxLayout = new QVBoxLayout;
    pvbxLayout->addWidget(new QLabel("<H1>Client</H1"));
    pvbxLayout->addWidget(txt_info_ptr_);
    pvbxLayout->addWidget(txt_input_ptr_);
    pvbxLayout->addWidget(pcmd);
    setLayout(pvbxLayout);

    txt_info_ptr_->append("Running echo server...");
    txt_info_ptr_->append("Command:");
    txt_info_ptr_->append(" exit - to quit");
    txt_info_ptr_->append(" get <filename> <beans/size> <number>");
    txt_info_ptr_->append(" ");
}


bool TcpClient::read_file()
{
    QDataStream in(tcp_socket_ptr_);
    in.setVersion(QDataStream::Qt_5_3);

    if (block_size_ == 0)
    {
        if (tcp_socket_ptr_->bytesAvailable() < sizeof(quint32))
            return false;
        in >> block_size_;
    }
    if (tcp_socket_ptr_->bytesAvailable() < block_size_)
        return false;

    block_size_ = 0;
    QString file_name;
    in >> file_name;
    const QByteArray line = tcp_socket_ptr_->readAll();

    const QString filePath = QDir::current().path();

    file_name = file_name.section("/", -1);
    QFile target(filePath + "/" + file_name);

    if (!target.open(QIODevice::WriteOnly))
    {
        return false;
    }
    target.write(line);
    target.close();
    return true;
}


void TcpClient::slot_ready_read()
{
    QDataStream in(tcp_socket_ptr_);
    in.setVersion(QDataStream::Qt_5_3);
    for (;;)
    {
        if (0U == next_block_size_)
        {
            if (tcp_socket_ptr_->bytesAvailable() < sizeof(quint16))
            {
                break;
            }
            in >> next_block_size_;
        }

        if (tcp_socket_ptr_->bytesAvailable() < next_block_size_)
        {
            break;
        }
        QString str;
        in >> str;
        txt_info_ptr_->append(/*time.toString() + " " +*/ str);
        next_block_size_ = 0;

        if ("exit" == str)
        {
            txt_info_ptr_->append("Breaking");
            this->close();
        }
        if ("get" == str)
        {
            if (!read_file())
            {
                txt_info_ptr_->append("File can't write");
            }
            else
            {
                txt_info_ptr_->append("File write");
            }
        }
    }
}


void TcpClient::slot_error(QAbstractSocket::SocketError err)
{
    const QString error_message =
        "Error:" + (err == QAbstractSocket::HostNotFoundError        ? "The host was not found."
                    : err == QAbstractSocket::RemoteHostClosedError  ? "The remote host is closed."
                    : err == QAbstractSocket::ConnectionRefusedError ? "The connection was refused."
                                                                     : (tcp_socket_ptr_->errorString()));
    txt_info_ptr_->append(error_message);
}


void TcpClient::slot_send_to_server()
{
    QByteArray  block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << static_cast<quint16>(0) << txt_input_ptr_->text();
    out.device()->seek(0);
    out << static_cast<quint16>(block.size() - sizeof(quint16));
    tcp_socket_ptr_->write(block);
    txt_input_ptr_->setText("");
}


void TcpClient::slot_connected()
{
    txt_info_ptr_->append("Received the connected() signal");
}
