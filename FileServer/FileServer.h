//
// Created by gjk on 2020/2/22.
//

#ifndef KYIMSERVER_FILESERVER_H
#define KYIMSERVER_FILESERVER_H


#include "TcpServer.h"
#include "Mutex.h"
#include <memory>

class CFileSession;

class CFileServer : public CTcpServer{
    CFileServer()= default;
    ~CFileServer()= default;
public:

    static CFileServer& getInstance(){
        static CFileServer server;
        return server;
    }

    bool        init(CEventLoop *loop, int listenport, int thread) override;

    void        onConnected(CTcpConnection *conn);
    void        onClosed(CTcpConnection *conn);

private:
    CMutex      m_mutex;
    std::map<int, std::shared_ptr<CFileSession>>
                m_sessions;
};


#endif //KYIMSERVER_FILESERVER_H
