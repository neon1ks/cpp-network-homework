#ifndef MYSERVER_H
#define MYSERVER_H

#include <QWidget>

#include "SslServer.h"

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTextEdit;
//class QTcpSocket;
class QSslSocket;
QT_END_NAMESPACE

class TcpServer : public QWidget
{
    Q_OBJECT

private:
    SslServer* tcp_server_ptr_ = nullptr;
    QTextEdit* txt_ptr_        = nullptr;
    quint16    next_block_size_;
    QTimer*    timer_ = nullptr;

private:
    void send_to_client(QSslSocket* socket_ptr, const QString& str);
    bool send_to_file(QSslSocket* socket_ptr, const QString& filename,const size_t& fict,
                    const size_t& size_fict);

public:
    TcpServer(int port, QWidget* parent = nullptr);

public slots:
    void slot_new_connection();
    void slot_read_client();
    void slot_disconnect();

};

#endif
