//
// Created by gjk on 2020/2/22.
//

#include "FileManager.h"
#include "Logger.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

bool FileManager::init(const std::string &fileDir) {
    if(fileDir.empty()){
        LOGE("fileDir is empty");
        return false;
    }

    m_fileDir = fileDir;
    //以目录格式结尾
    if(m_fileDir.back() != '/')m_fileDir += '/';
    return loadAllFiles();
}

bool FileManager::loadAllFiles() {
    if(m_fileDir.empty())return false;
    //打开文件目录
    DIR* fileDir = ::opendir(m_fileDir.c_str());
    if(!fileDir){
        fprintf(stderr, "open file dir failed");
        //打开失败尝试创建
        if(::mkdir(m_fileDir.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) != 0){
            fprintf(stderr, "create file dir failed, err=%s", strerror(errno));
            return false;
        }

        return true;
    }

    //读取目录中的文件列表
    struct dirent* file;
    while((file = ::readdir(fileDir))){
        m_allFiles.insert(file->d_name);
    }

    ::closedir(fileDir);
    LOGI("FileManager::loadAllFiles success");
    return true;
}

bool FileManager::fileExists(const std::string &fmd5) {
    {
        CMutexLock lock(m_mutex);
        auto it = m_allFiles.find(fmd5);
        //在文件列表中找到了
        if(it != m_allFiles.end())
            return true;
    }

    //在文件列表中没找到，则去磁盘查看
    FILE* file = ::fopen((m_fileDir + fmd5).c_str(), "r");
    if(file){
        fclose(file);
        CMutexLock lock(m_mutex);
        m_allFiles.insert(fmd5);
        return true;
    }
    return false;
}

void FileManager::addFile(const std::string &fmd5) {
    CMutexLock lock(m_mutex);
    m_allFiles.insert(fmd5);
}