//
// Created by gjk on 2020/2/10.
//

#ifndef KYIMSERVER_EVENTLOOP_H
#define KYIMSERVER_EVENTLOOP_H

#include "noncopyable.h"
struct event_base;

class CEventLoop:public noncopyable{
public:
    CEventLoop();
    ~CEventLoop();
    void loop();
    event_base*     getBase(){return m_evbase;}
private:
    event_base*     m_evbase;
};

#endif //KYIMSERVER_EVENTLOOP_H
