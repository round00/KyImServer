//
// Created by gjk on 2020/2/10.
//

#ifndef KYIMSERVER_IMSERVER_H
#define KYIMSERVER_IMSERVER_H

#include "TcpServer.h"
#include <memory>

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
    //根据uid来获取Sessions
    std::vector<std::shared_ptr<CClientSession>>
                getClientSession(uint32_t uid);
    //根据uid和client来获取Session
    std::shared_ptr<CClientSession>
                getClientSession(uint32_t uid, uint8_t client);
    //踢用户下线
    void        kickUser(uint32_t uid, uint8_t client);

private:
    std::map<int, std::shared_ptr<CClientSession>>  m_sessions;
};

#endif //KYIMSERVER_IMSERVER_H
