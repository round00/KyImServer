#include <stdio.h>
#include <unistd.h>
#include "db/MysqlDb.h"
#include "ConfigFile.h"
#include "base/Logger.h"
#include "base/TestThread.h"




int main()
{

	//dbserver = 0.0.0.0
	//dbuser = root
	//dbpassword = nibuzhidao1
	//dbname = flamingo
	CConfigFile config("../etc/chatserver.conf");
//    printf("%s\n", config.getValue("listenport").c_str());
	std::string dbhost = config.getValue("dbserver");
	std::string user = config.getValue("dbuser");
	std::string pass = config.getValue("dbpassword");
	std::string dbname = config.getValue("dbname");
//	std::string dbname = "test";

	CMysqlDb db(dbhost, user, pass, dbname);
	if(!db.init())
    {
	    printf("database init failed.\n");
	    return 0;
    }

//	int ret = db.execute("CREATE TABLE writers(name VARCHAR(25))");
//	ret = db.execute("INSERT INTO writers VALUES('Jinke Gong')");
//	ret = db.execute("INSERT INTO writers VALUES('Tongtong Yao')");

    CMysqlQueryResult queryResult = db.query("SELECT * FROM t_user");
    while(queryResult.hasNextRow()){
        std::vector<std::string> row = queryResult.getNextRow();
        for(auto s:row){
            printf("%s ", s.c_str());
        }
        printf("\n");
    }
    queryResult.Release();

	return 0;
}