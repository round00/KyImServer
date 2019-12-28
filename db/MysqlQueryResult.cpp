//
// Created by gjk on 2019/12/28.
//

#include "MysqlQueryResult.h"

CMysqlQueryResult::CMysqlQueryResult(MYSQL_RES* result)
:m_pResult(result),m_nCurRow(0),m_bInited(false)
{
    if(m_pResult)
    {
        //获取行数和列数
        m_nRows = mysql_num_rows(m_pResult);
        m_nFileds = mysql_num_fields(m_pResult);
        m_bInited = true;
    }

}

CMysqlQueryResult::~CMysqlQueryResult()
{
    //FIXME:在析构函数里调用Release的话会段错误，目前还没查到为什么
}

void CMysqlQueryResult::Release()
{
    if(m_pResult)
    {
        mysql_free_result(m_pResult);
        m_pResult = nullptr;
    }
}

bool CMysqlQueryResult::hasNextRow() {
    if(!m_bInited)
    {
        return false;
    }
    return m_nCurRow<m_nRows;
}

std::vector<std::string> CMysqlQueryResult::getNextRow()
{
    std::vector<std::string> fields;
    //没有完成初始化，无法获取数据
    if(!m_bInited)return fields;

    MYSQL_ROW row = mysql_fetch_row(m_pResult);
    m_nCurRow++;

    for(int i = 0;i<m_nFileds; ++i)
    {
        //处理当前字段本身就是null的情况
        if(!row[i])
            fields.emplace_back("");
        else
            fields.emplace_back(row[i]);
    }
    return fields;
}