//
// Created by gjk on 2020/2/22.
//

#ifndef KYIMSERVER_FILEMANAGER_H
#define KYIMSERVER_FILEMANAGER_H

#include <string>
#include <set>
#include "noncopyable.h"
#include "Mutex.h"

class FileManager : public noncopyable {
public:
    static FileManager &getInstance() {
        static FileManager instance;
        return instance;
    }

    bool            init(const std::string& fileDir);
    bool            fileExists(const std::string& fmd5);
    void            addFile(const std::string& fmd5);

    std::string     getFileDir(){return m_fileDir;}
private:
    bool            loadAllFiles();
    //放置文件的目录
    std::string     m_fileDir;
    //所有文件的集合，用文件的MD5值标识文件
    CMutex          m_mutex;
    std::set<std::string>
                    m_allFiles;
};

#endif //KYIMSERVER_FILEMANAGER_H
