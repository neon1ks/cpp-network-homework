#include "SslServer.h"

#include <QFile>
#include <QSslSocket>


SslServer::SslServer(QObject* parent)
    : QTcpServer(parent)
    , ssl_protocol_(QSsl::UnknownProtocol)
{
}


void SslServer::incomingConnection(qintptr socketDescriptor)
{
    // Create socket
    QSslSocket* ssl_socket = new QSslSocket(this);
    ssl_socket->setSocketDescriptor(socketDescriptor);
    ssl_socket->setLocalCertificate(ssl_local_certificate_);
    ssl_socket->setPrivateKey(ssl_private_key_);
    ssl_socket->setProtocol(ssl_protocol_);
    ssl_socket->startServerEncryption();

    // Add to the internal list of pending connections (see Qt doc:
    // http://qt-project.org/doc/qt-5/qtcpserver.html#addPendingConnection)
    this->addPendingConnection(ssl_socket);
}


const QSslCertificate& SslServer::get_ssl_local_certificate() const
{
    return ssl_local_certificate_;
}


const QSslKey& SslServer::get_ssl_private_key() const
{
    return ssl_private_key_;
}


QSsl::SslProtocol SslServer::get_ssl_protocol() const
{
    return ssl_protocol_;
}


void SslServer::set_ssl_local_certificate(const QSslCertificate& certificate)
{
    ssl_local_certificate_ = certificate;
}


bool SslServer::set_ssl_local_certificate(const QString& path, QSsl::EncodingFormat format)
{
    QFile certificate_file(path);

    if (!certificate_file.open(QIODevice::ReadOnly))
        return false;
    
    ssl_local_certificate_ = QSslCertificate(certificate_file.readAll(), format);
    return true;
}


void SslServer::set_ssl_private_key(const QSslKey& key)
{
    ssl_private_key_ = key;
}


bool SslServer::set_ssl_private_key(const QString&       file_name,
                                    QSsl::KeyAlgorithm   algorithm,
                                    QSsl::EncodingFormat format,
                                    const QByteArray&    pass_phrase)
{
    QFile key_file(file_name);

    if (!key_file.open(QIODevice::ReadOnly))
        return false;
    
    ssl_private_key_ = QSslKey(key_file.readAll(), algorithm, format, QSsl::PrivateKey, pass_phrase);
    return true;
}


void SslServer::set_ssl_protocol(QSsl::SslProtocol protocol)
{
    ssl_protocol_ = protocol;
}
