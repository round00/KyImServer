#include "MysqlDb.h"
#include <stdio.h>

CMysqlDb::CMysqlDb(const std::string& host, 
	const std::string& username,
	const std::string& password, 
	const std::string& dbname)
	:m_dbConn(nullptr), 
	m_strHost(host), m_strUsername(username), 
	m_strPassword(password), m_strDbName(dbname)
{
}

CMysqlDb::~CMysqlDb()
{
	if (m_dbConn) {
		mysql_close(m_dbConn);
	}
}

bool CMysqlDb::init() 
{
	m_dbConn = mysql_init(nullptr);
	if (!m_dbConn) 
	{
		printf("mysql init failed, err=%u(%s)\n", mysql_errno(m_dbConn), mysql_error(m_dbConn));
		return false;
	}

	if (!mysql_real_connect(m_dbConn, 
		m_strHost.c_str(), m_strUsername.c_str(),
		m_strPassword.c_str(), m_strDbName.c_str(),
		0, nullptr, 0)) 
	{
		printf("mysql connect failed, err=%u(%s)\n", mysql_errno(m_dbConn), mysql_error(m_dbConn));
		return false;
	}

	return true;
}

bool CMysqlDb::execute(const std::string& sql)
{
    if(mysql_query(m_dbConn, sql.c_str()))
    {
        printf("mysql_query failed, err=%u(%s)\n", mysql_errno(m_dbConn), mysql_error(m_dbConn));
        return false;
    }
    return true;
}

CMysqlQueryResult CMysqlDb::query(const std::string& sql)
{
    if(!execute(sql))
    {
        return CMysqlQueryResult();
    }

    MYSQL_RES* result = mysql_store_result(m_dbConn);
    CMysqlQueryResult queryResult(result);
    return queryResult;
}