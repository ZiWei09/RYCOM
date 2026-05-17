#include "networkdebug.h"

NetworkDebug::NetworkDebug(QObject *parent)
    : QObject(parent)
{
}

NetworkDebug::~NetworkDebug()
{
    cleanup();
}

void NetworkDebug::start(Protocol protocol, const QString &host, quint16 port)
{
    if (m_state != Disconnected)
        stop();

    m_protocol = protocol;
    m_host = host;
    m_port = port;

    switch (protocol) {
    case TCP_Client: {
        m_tcpSocket = new QTcpSocket(this);
        connect(m_tcpSocket, &QTcpSocket::connected, this, &NetworkDebug::onTcpConnected);
        connect(m_tcpSocket, &QTcpSocket::disconnected, this, &NetworkDebug::onTcpDisconnected);
        connect(m_tcpSocket, &QTcpSocket::readyRead, this, &NetworkDebug::onTcpReadyRead);
        connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &NetworkDebug::onTcpError);

        m_tcpSocket->connectToHost(QHostAddress(host), port);
        break;
    }
    case TCP_Server: {
        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection, this, &NetworkDebug::onServerNewConnection);
        connect(m_tcpServer, &QTcpServer::acceptError, this, &NetworkDebug::onServerAcceptError);

        if (m_tcpServer->listen(QHostAddress::Any, port)) {
            setState(Listening);
        } else {
            emit errorOccurred("TCP服务端启动失败: " + m_tcpServer->errorString());
            cleanup();
        }
        break;
    }
    case UDP: {
        m_udpSocket = new QUdpSocket(this);
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkDebug::onUdpReadyRead);
        connect(m_udpSocket, &QUdpSocket::errorOccurred, this, &NetworkDebug::onUdpError);

        if (m_udpSocket->bind(QHostAddress::Any, port, QUdpSocket::ShareAddress)) {
            setState(Connected);
        } else {
            emit errorOccurred("UDP绑定失败: " + m_udpSocket->errorString());
            cleanup();
        }
        break;
    }
    }
}

void NetworkDebug::stop()
{
    cleanup();
    setState(Disconnected);
}

void NetworkDebug::send(const QByteArray &data)
{
    if (m_state == Disconnected)
        return;

    switch (m_protocol) {
    case TCP_Client:
        if (m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState)
            m_tcpSocket->write(data);
        break;
    case TCP_Server:
        if (m_serverClient && m_serverClient->state() == QAbstractSocket::ConnectedState)
            m_serverClient->write(data);
        break;
    case UDP:
        if (m_udpSocket)
            m_udpSocket->writeDatagram(data, QHostAddress(m_host), m_port);
        break;
    }
}

void NetworkDebug::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void NetworkDebug::cleanup()
{
    if (m_serverClient) {
        m_serverClient->disconnectFromHost();
        m_serverClient->deleteLater();
        m_serverClient = nullptr;
    }
    if (m_tcpServer) {
        m_tcpServer->close();
        m_tcpServer->deleteLater();
        m_tcpServer = nullptr;
    }
    if (m_tcpSocket) {
        m_tcpSocket->disconnectFromHost();
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
    }
    if (m_udpSocket) {
        m_udpSocket->close();
        m_udpSocket->deleteLater();
        m_udpSocket = nullptr;
    }
}

// TCP Client slots
void NetworkDebug::onTcpConnected()
{
    setState(Connected);
}

void NetworkDebug::onTcpDisconnected()
{
    setState(Disconnected);
    cleanup();
}

void NetworkDebug::onTcpReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket)
        emit dataReceived(socket->readAll());
}

void NetworkDebug::onTcpError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket)
        emit errorOccurred("TCP错误: " + socket->errorString());
    cleanup();
    setState(Disconnected);
}

// TCP Server slots
void NetworkDebug::onServerNewConnection()
{
    if (m_serverClient) {
        // 仅接受一个客户端，拒绝后续连接
        QTcpSocket *rejected = m_tcpServer->nextPendingConnection();
        rejected->disconnectFromHost();
        rejected->deleteLater();
        return;
    }

    m_serverClient = m_tcpServer->nextPendingConnection();
    connect(m_serverClient, &QTcpSocket::readyRead, this, &NetworkDebug::onTcpReadyRead);
    connect(m_serverClient, &QTcpSocket::disconnected, this, [this]() {
        m_serverClient->deleteLater();
        m_serverClient = nullptr;
        setState(Listening);
    });
    connect(m_serverClient, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred("客户端连接错误: " + m_serverClient->errorString());
        m_serverClient->deleteLater();
        m_serverClient = nullptr;
        setState(Listening);
    });
    m_host = m_serverClient->peerAddress().toString();
    setState(Connected);
}

void NetworkDebug::onServerAcceptError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit errorOccurred("服务端接受连接错误: " + m_tcpServer->errorString());
}

// UDP slots
void NetworkDebug::onUdpReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        // 记录最后发送者地址以便回复
        m_host = sender.toString();
        m_port = senderPort;
        emit dataReceived(datagram);
    }
}

void NetworkDebug::onUdpError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit errorOccurred("UDP错误: " + m_udpSocket->errorString());
}
