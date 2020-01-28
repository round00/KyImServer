#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "db/MysqlDb.h"
#include "ConfigFile.h"
#include "base/Logger.h"
#include "base/Thread.h"
#include "jsoncpp/json.h"
#include "md5.h"
#include "zlib/ZlibUtil.h"

using std::cout;
using std::endl;
int main()
{
    char buf[] = "Gongjinke Ni Zui shuai!";
    char combuf[128] = {0};
    size_t len = sizeof combuf;
    bool ret = Zlib::compressBuf(buf, strlen(buf), combuf, len);
    cout<<buf<<endl;
    cout<<ret<<endl;
    cout<<combuf<<endl;
    cout<<len<<endl;

    cout<<"======"<<endl;

    Json::Value json;
    json["name"] = Json::Value("gjk");
    json["age"] = Json::Value(21);
    Json::Value j2;
    j2["obj"] = json;
    auto t = j2["obj"].type();
    cout<<t<<endl;
    auto tt = j2["obj"];
    cout<<j2["obj"]["age"].type()<<endl;


    cout<<"======"<<endl;


    std::cout<<MD5("testmd5").toStr()<<std::endl;
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