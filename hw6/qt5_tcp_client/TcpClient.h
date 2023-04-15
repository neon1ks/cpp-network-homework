#ifndef MYCLIENT_H
#define MYCLIENT_H

#include <QWidget>
// #include <QTcpSocket>

#include <QSslSocket>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QLineEdit;
QT_END_NAMESPACE

class TcpClient : public QWidget
{
    Q_OBJECT

private:
    QSslSocket* tcp_socket_ptr_ = nullptr;
    QTextEdit*  txt_info_ptr_   = nullptr;
    QLineEdit*  txt_input_ptr_  = nullptr;
    quint16     next_block_size_;
    qint32      block_size_;

    bool read_file();

public:
    TcpClient(const QString& host, int port, QWidget* parent = nullptr);

private slots:
    void slot_ready_read();
    void slot_error(QAbstractSocket::SocketError);
    void slot_send_to_server();
    void slot_connected();
};

#endif
