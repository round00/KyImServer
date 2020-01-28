//
// Created by gjk on 2020/1/28.
//

#ifndef KYIMSERVER_SINGLETON_H
#define KYIMSERVER_SINGLETON_H

template <typename E>
class Singleton
{
public:
    Singleton() = delete;
    ~Singleton() = delete;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static E& getInstance(){
        static E instance;
        return instance;
    }
};

#endif //KYIMSERVER_SINGLETON_H
