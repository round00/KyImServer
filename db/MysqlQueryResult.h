//
// Created by gjk on 2019/12/28.
//

#ifndef KYIMSERVER_MYSQLQUERYRESULT_H
#define KYIMSERVER_MYSQLQUERYRESULT_H

#include <mysql.h>
#include <string>
#include <vector>

class CMysqlQueryResult{
public:
    explicit CMysqlQueryResult(MYSQL_RES* result = nullptr);
    ~CMysqlQueryResult();

    //使用完之后应该要调用Release
    void Release();

    bool hasNextRow();
    std::vector<std::string> getNextRow();

private:
    MYSQL_RES* m_pResult;

    int m_nFileds;
    int m_nRows;
    int m_nCurRow;
    bool m_bInited;
};

#endif //KYIMSERVER_MYSQLQUERYRESULT_H
