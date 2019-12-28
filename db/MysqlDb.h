#include <mysql.h>
#include <string>

#include "MysqlQueryResult.h"


class CMysqlDb
{
public:
	CMysqlDb(const std::string& host, 
		const std::string& username, 
		const std::string& password,
		const std::string& dbname);
	~CMysqlDb();
	bool init();  
	bool execute(const std::string& sql);
	CMysqlQueryResult query(const std::string& sql);

private:
	MYSQL*		m_dbConn;
	std::string m_strHost;
	std::string m_strUsername;
	std::string m_strPassword;
	std::string m_strDbName;
};
//