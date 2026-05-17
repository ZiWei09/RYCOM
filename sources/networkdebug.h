#ifndef NETWORKDEBUG_H
#define NETWORKDEBUG_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUdpSocket>
#include <QHostAddress>

class NetworkDebug : public QObject
{
    Q_OBJECT

public:
    enum Protocol {
        TCP_Client,
        TCP_Server,
        UDP
    };

    enum State {
        Disconnected,
        Listening,
        Connected
    };

    explicit NetworkDebug(QObject *parent = nullptr);
    ~NetworkDebug();

    void start(Protocol protocol, const QString &host, quint16 port);
    void stop();
    void send(const QByteArray &data);

    State state() const { return m_state; }
    Protocol protocol() const { return m_protocol; }

signals:
    void dataReceived(const QByteArray &data);
    void stateChanged(NetworkDebug::State state);
    void errorOccurred(const QString &error);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpReadyRead();
    void onTcpError(QAbstractSocket::SocketError socketError);

    void onServerNewConnection();
    void onServerAcceptError(QAbstractSocket::SocketError socketError);

    void onUdpReadyRead();
    void onUdpError(QAbstractSocket::SocketError socketError);

private:
    void setState(State state);
    void cleanup();

    QTcpSocket   *m_tcpSocket   = nullptr;
    QTcpServer   *m_tcpServer   = nullptr;
    QTcpSocket   *m_serverClient = nullptr;  // TCP Server接受的客户端
    QUdpSocket   *m_udpSocket   = nullptr;

    State     m_state     = Disconnected;
    Protocol  m_protocol;
    QString   m_host;
    quint16   m_port      = 0;
};

#endif // NETWORKDEBUG_H
