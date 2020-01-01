#include <stdio.h>
#include <unistd.h>
#include "db/MysqlDb.h"
#include "ConfigFile.h"
#include "base/Logger.h"
#include "base/Thread.h"


void* func(void*){
    for(int i = 0;i<10;++i) {
        printf("second %d\n", i);
        sleep(1);
    }
}

int main()
{
    CThread thread(func);
    if(!thread.start()){
        printf("thread start failed\n");
        return 0;
    }
    sleep(2);
    for(int i = 0;i<5; ++i){
        bool ret = thread.isRunning();
        printf("thread is alive=%d\n", ret);
        sleep(1);
    }
    thread.stop();
    puts("-------------------------");
    bool ret = thread.isRunning();
    printf("thread is alive=%d\n", ret);
    sleep(1);
    ret = thread.isRunning();
    printf("thread is alive=%d\n", ret);
    return 0;
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