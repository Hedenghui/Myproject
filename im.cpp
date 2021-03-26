#include"im.hpp"
#include<inttypes.h>

void sql_test(){
	im::TableUser user;
	//user.Insert("zhangsan","111111");
	//user.Insert("lisi","111111");
	//user.UpdatePasswd("zhangsan","111111");
	//cout<<user.VerifyUser("lisi","111111")<<endl;
	//cout<<user.Exists("wangwu")<<endl;
	Json::Value val;
	user.SelectAll(&val);
	Json::StyledWriter writer;
	cout<<writer.write(val)<<endl;
}
int main(){
	//sql_test();
	im::IM im_serve;
    	im_serve.Init();
	im_serve.Run();
	return 0;
}

