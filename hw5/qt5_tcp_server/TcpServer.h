#ifndef MYSERVER_H
#define MYSERVER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTextEdit;
class QTcpSocket;
QT_END_NAMESPACE

class TcpServer : public QWidget
{
    Q_OBJECT

private:
    QTcpServer* tcp_server_ptr_ = nullptr;
    QTextEdit*  txt_ptr_       = nullptr;
    quint16     next_block_size_;
    QTimer*     timer_ = nullptr;

private:
    static void send_to_client(QTcpSocket* socket_ptr, const QString& str);
    static bool send_to_file(QTcpSocket* socket_ptr, const QString& filename, const size_t& fict, const size_t& size_fict);

public:
    TcpServer(int port, QWidget* pwgt = nullptr);

public slots:
    void slot_new_connection();
    void slot_read_client();
};

#endif
