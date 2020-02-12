//
// Created by gjk on 2020/2/10.
//

#ifndef KYIMSERVER_CLIENTSESSION_H
#define KYIMSERVER_CLIENTSESSION_H

class CTcpConnection;
class CClientSession{
public:
    explicit CClientSession(CTcpConnection* conn):m_conn(conn){}

    void        onMessage(CTcpConnection* conn);
private:
    CTcpConnection*     m_conn;
};

#endif //KYIMSERVER_CLIENTSESSION_H
