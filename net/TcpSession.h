//
// Created by gjk on 2020/2/22.
//

#ifndef KYIMSERVER_TCPSESSION_H
#define KYIMSERVER_TCPSESSION_H

#include "TcpServer.h"
class CTcpConnection;
class CTcpSession{
public:
    explicit CTcpSession(CTcpConnection* conn):
    m_conn(conn){}

    void        send(const std::string& packet);

protected:
    CTcpConnection*     m_conn;

};
#endif //KYIMSERVER_TCPSESSION_H
