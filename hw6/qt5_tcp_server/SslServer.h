#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <QSsl>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>
#include <QTcpServer>

class SslServer : public QTcpServer
{
    Q_OBJECT

public:
    SslServer(QObject* parent = nullptr);

    [[nodiscard]] const QSslCertificate& get_ssl_local_certificate() const;
    [[nodiscard]] const QSslKey&         get_ssl_private_key() const;
    [[nodiscard]] QSsl::SslProtocol      get_ssl_protocol() const;

    void set_ssl_local_certificate(const QSslCertificate& certificate);
    bool set_ssl_local_certificate(const QString& path, QSsl::EncodingFormat format = QSsl::Pem);

    void set_ssl_private_key(const QSslKey& key);
    bool set_ssl_private_key(const QString&       file_name,
                             QSsl::KeyAlgorithm   algorithm   = QSsl::Rsa,
                             QSsl::EncodingFormat format      = QSsl::Pem,
                             const QByteArray&    pass_phrase = QByteArray());

    void set_ssl_protocol(QSsl::SslProtocol protocol);

protected:
    void incomingConnection(qintptr socket_descriptor) override final;

private:
    QSslCertificate   ssl_local_certificate_;
    QSslKey           ssl_private_key_;
    QSsl::SslProtocol ssl_protocol_;
};

#endif // SSLSERVER_H
