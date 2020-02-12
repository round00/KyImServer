//
// Created by gjk on 2020/2/10.
//

#include <event.h>
#include "EventLoop.h"

CEventLoop::CEventLoop()
{
    m_evbase = event_base_new();
}

CEventLoop::~CEventLoop() {
    if(m_evbase){
        event_base_free(m_evbase);
    }
}

void CEventLoop::loop() {
    if(!m_evbase)return;
    event_base_dispatch(m_evbase);

}
