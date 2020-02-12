//
// Created by gjk on 2020/2/10.
//

#ifndef KYIMSERVER_IMSERVER_H
#define KYIMSERVER_IMSERVER_H

#include "TcpServer.h"

class CClientSession;

class CImServer : public CTcpServer{
    CImServer()= default;
    ~CImServer()= default;
public:

    static CImServer& getInstance(){
        static CImServer server;
        return server;
    }

    bool        init(CEventLoop *loop, int listenport, int thread) override;

    void        onConnected(CTcpConnection *conn);
    void        onClosed(CTcpConnection *conn);

private:
    std::map<int, std::shared_ptr<CClientSession>>  m_sessions;
};

#endif //KYIMSERVER_IMSERVER_H
