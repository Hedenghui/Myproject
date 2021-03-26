#include<cstdio>
#include<iostream>
#include<mysql/mysql.h>
#include<jsoncpp/json/json.h>
#include<mutex>
#include"mongoose.h"
#include<list>
#include<sstream>
using namespace std;
#define MYSQL_HOST "127.0.0.1"
#define MYSQL_USER "root"
#define MYSQL_PASS "123456"
#define MYSQL_DB "im_system"
#define ONLINE "online"
#define OFFLINE "offline"
namespace im{
	class TableUser
	{
		public:
			TableUser(){
				//完成数据库操作的初始化
				//初始化Mysql句柄
				_mysql=mysql_init(NULL);
				if(_mysql==NULL){
					printf("init mysql instance failed!\n");
					exit(-1);
				}
				//连接mysql服务器
				if(mysql_real_connect(_mysql,MYSQL_HOST,MYSQL_USER,MYSQL_PASS,MYSQL_DB,0,NULL,0)==NULL){
					printf("connect mysql server failed!\n");
					//如果连接失败关闭数据库释放mysql操作句柄
					mysql_close(_mysql);
					exit(-1);
				}
				//设置字符集,成功返回0,失败返回非0
				if(mysql_set_character_set(_mysql,"utf8")!=0){
					printf("set client character failed:%s\n",mysql_error(_mysql));
					mysql_close(_mysql);
					exit(-1);
				}

			}
			~TableUser(){
				//完成数据库句柄的销毁
				if(_mysql){
					mysql_close(_mysql);
				}
			}
			bool Insert(const string &name,const string &passwd){
#define INSERT_USER "insert tb_user value(null,'%s',MD5('%s'),'%s');"
				char tmp_sql[4096]={0};
				sprintf(tmp_sql,INSERT_USER,name.c_str(),passwd.c_str(),OFFLINE);
				return QuerySql(tmp_sql);//mysql执行语句	
			}
			bool Delete(const string &name){
#define DELETE_USER "delete from tb_user where name='%s';"
				char tmp_sql[4096]={0};
				sprintf(tmp_sql,DELETE_USER,name.c_str());
				return QuerySql(tmp_sql);
			}
			bool UpdateStatus(const string &name,const string &status){
#define UPDATE_USER_STATU "update tb_user set status='%s' where name='%s';"
				char tmp_sql[4096]={0};
				sprintf(tmp_sql,UPDATE_USER_STATU,status.c_str(),name.c_str());
				return QuerySql(tmp_sql);
			}
			bool UpdatePasswd(const string &name,const string &passwd){
#define UPDATE_USER_PASS "update tb_user set passwd=MD5('%s') where name='%s';"
				char tmp_sql[4096]={0};
				sprintf(tmp_sql,UPDATE_USER_PASS,passwd.c_str(),name.c_str());
				return QuerySql(tmp_sql);
			}
			//获取单人信息
			bool SelectOne(const string &name,Json::Value *user){
#define SELECT_USER_ONE "select id,passwd,status from tb_user where name='%s';"
				char tmp_sql[4096]={0};
				sprintf(tmp_sql,SELECT_USER_ONE,name.c_str());
				_mutex.lock();//加锁,获取结果集不是原子操作
				if(QuerySql(tmp_sql)==false){
					_mutex.unlock();
					return false;
				}
				MYSQL_RES *res=mysql_store_result(_mysql);
				_mutex.unlock();
				if(res==NULL){
					printf("select one user store result failed:%s\n",mysql_error(_mysql));
					return false;
				}
				int num_row=mysql_num_rows(res);//获取结果集中结果的条数
				if(num_row!=1){
					printf("one user result count error!\n");
					mysql_free_result(res);//释放结果集空间
					return false;
				}
				for(int i=0;i<num_row;i++){
					MYSQL_ROW row=mysql_fetch_row(res);//遍历结果集,每次获取一行数据,不会重复,内部有读取位置维护
					(*user)["id"]=stoi(row[0]);
					(*user)["name"]=name.c_str();
					(*user)["passwd"]=row[1];
					(*user)["status"]=row[2];
				}
				mysql_free_result(res);
				return true;
			}
			//获取所有人信息
			bool SelectAll(Json::Value *users){
#define SELECT_ALL_USER "select id,name,passwd,status from tb_user;"
				_mutex.lock();
				if(QuerySql(SELECT_ALL_USER)==false){
					_mutex.unlock();
					return false;
				}
				MYSQL_RES *res=mysql_store_result(_mysql);
				_mutex.unlock();
				if(res==NULL){
					printf("select all user store result failed:%s\n",mysql_error(_mysql));
					return false;
				}
				int num_row=mysql_num_rows(res);
				for(int i=0;i<num_row;i++){
					MYSQL_ROW row=mysql_fetch_row(res);
					Json::Value user;
					user["id"]=stoi(row[0]);
					user["name"]=row[1];
					user["passwd"]=row[2];
					user["status"]=row[3];
					users->append(user);
				}
				mysql_free_result(res);
				return true;
			}
			//验证账号密码
			bool VerifyUser(const string &name,const string &passwd){
#define VERIFY_USER "select * from tb_user where name='%s' and passwd=MD5('%s');"
				char tmp_sql[4096]={0};
				sprintf(tmp_sql,VERIFY_USER,name.c_str(),passwd.c_str());
				_mutex.lock();
				if(QuerySql(tmp_sql)==false){
					_mutex.unlock();
					return false;
				}
				MYSQL_RES *res=mysql_store_result(_mysql);
				_mutex.unlock();
				if(res==NULL){
					printf("verify user store result failed:%s\n",mysql_error(_mysql));
					return false;
				}
				int num_row=mysql_num_rows(res);
				if(num_row!=1){
					printf("verify user failed!\n");
					mysql_free_result(res);
					return false;
				}
				mysql_free_result(res);
				return true;
			}
			//验证账号状态
			bool VerifyUser_status(const string &name,const string &status){
#define VERIFY_USER_STATUS "select * from tb_user where name='%s' and status='%s';"
				char tmp_sql[4096]={0};
				sprintf(tmp_sql,VERIFY_USER_STATUS,name.c_str(),status.c_str());
				_mutex.lock();
				if(QuerySql(tmp_sql)==false){
					_mutex.unlock();
					return false;
				}
				MYSQL_RES *res=mysql_store_result(_mysql);
				_mutex.unlock();
				if(res==NULL){
					printf("verify user store result failed:%s\n",mysql_error(_mysql));
					return false;
				}
				int num_row=mysql_num_rows(res);
				if(num_row!=1){
					mysql_free_result(res);
					return false;
				}
				mysql_free_result(res);
				return true;
			}
			//判断用户是否存在
			bool Exists(const string &name){
#define EXISTS_USER "select * from tb_user where name='%s';"
				char tmp_sql[4096]={0};
				sprintf(tmp_sql,EXISTS_USER,name.c_str());
				_mutex.unlock();
				if(QuerySql(tmp_sql)==false){
					_mutex.unlock();
					return false;
				}
				MYSQL_RES *res=mysql_store_result(_mysql);
				_mutex.unlock();
				if(res==NULL){
					printf("exists user store result failed:%s\n",mysql_error(_mysql));
					return false;	
				}
				int num_row=mysql_num_rows(res);
				if(num_row!=1){
					printf("have no user!\n");
					mysql_free_result(res);
					return false;
				}
				mysql_free_result(res);
				return true;	
			}

		private:
			//执行sql语句
			bool QuerySql(const string &sql){
				if(mysql_query(_mysql,sql.c_str())!=0){
					printf("query sql:[%s] failed:%s\n",sql.c_str(),mysql_error(_mysql));
					return false;
				}
				return true;
			}
		private:
			MYSQL *_mysql;	
			mutex _mutex;
	};

	struct session{
		string name;
		uint64_t session_id;
		double login_time;
		double last_atime;
		struct mg_connection *conn;
	};

	class IM
	{
		public:
			~IM(){
				mg_mgr_free(&_mgr);
			}
			static bool Init(const string &port="9000"){
				_tb_user=new TableUser();
				//初始化句柄
				mg_mgr_init(&_mgr);
				string addr="0.0.0.0:";
				addr+=port;
				//创建http监听连接 
				_lst_http=mg_http_listen(&_mgr,addr.c_str(),callback,&_mgr);//callback回调函数
				if(_lst_http==NULL){
					cout<<"http listen failed!\n";
					return false;
				}
				return true;
			}
			static bool Run(){
				while(1) mg_mgr_poll(&_mgr,100000);//开始轮询监听(有数据到来进行处理)
				return true;
			}
		private:
			//分割字符串
			static int Split(const string &str,const string &sep,vector<string> *list){
				//string::substr(size_type _pos=0,size_type _n=npos);//从pos位置开始截取n长度数
				//string::find(const char *_s,size_type _pos);//从pos位置开始照s分割符，返回所在位置
				int count=0;
				size_t pos=0,idx=0;
				while(1){
					pos=str.find(sep,idx);
					if(pos==string::npos){
						break;
					}
					list->push_back(str.substr(idx,pos-idx));
					idx=pos+sep.size();
					count++;
				}
				if(idx<str.size()){
					list->push_back(str.substr(idx));
					count++;
				}
				return count;
			}
			//SESSION_ID=1615173399566992; NAME=wangwu; path=/
			static bool GetCookie(const string &cookie,const string &key,string *val){
				vector<string> list;
				int count=Split(cookie,";",&list);
				for(auto s:list){
					vector<string> arry_cookie;
					Split(s,"=",&arry_cookie);
					if(arry_cookie[0]==key){
						*val=arry_cookie[1];
						return true;
					}
				}
				return false;
			}
			//创建会话
			static void CreateSession(struct session *s,struct mg_connection *c,const string &name){
				s->name=name;
				s->session_id=(uint64_t)(mg_time()*1000000);//mg_time是mongoose中的时间函数，避免了冲突
				s->login_time=mg_time();
				s->last_atime=mg_time();
				s->conn=c;
				return;
			}
			//删除会话
			static void DeleteSession(struct mg_connection *c){
				auto it=_list.begin();
				for(;it!=_list.end();it++){
					if(it->conn==c){
						cout<<"delete session: "<<it->name<<endl;
						_list.erase(it);
						return;
					}
				}
				return;
			}
			//获取session信息
			static struct session *GetSessionByConn(struct mg_connection *c){
				auto it=_list.begin();
				for(;it!=_list.end();it++){
					if(it->conn==c){
						return &(*it);
					}
				}
				return NULL;
			}
			//获取session
			static struct session *GetSessionByName(const string &name){
				auto it=_list.begin();
				for(;it!=_list.end();it++){
					if(it->name==name){
						return &(*it);
					}
				}
				return NULL;
			}

			//注册模块
			static bool reg(struct mg_connection *c,struct mg_http_message *hm){
				//从正文中获取到提交的用户信息---json字符串
				//解析得到用户名和密码
				//判断用户名是否被占用
				//将用户插入到数据库
				int status=200;
				string header="Content-Type:application/json\r\n";
				string body;
				body.assign(hm->body.ptr,hm->body.len);//mg_str中含有ptr,len

				Json::Value user;
				Json::Reader reader;//反序列化,从json字符串解析得到Json::Value 对象
				bool ret=reader.parse(body,user);
				if(ret==false){
					status=400;
					mg_http_reply(c,status,header.c_str(),"{\"reason\":\"请求格式错误\"}");
					return false;
				}
				//判断用户名是否被占用
				ret=_tb_user->Exists(user["name"].asString());//asString()反序列化用到的打印接口
				if(ret==true){
					status=400;
					mg_http_reply(c,status,header.c_str(),"{\"reason\":\"用户名被占用\"}");
					return false;
				}
				//插入到数据库
				ret=_tb_user->Insert(user["name"].asString(),user["passwd"].asString());
				if(ret==false){
					status=500;
					mg_http_reply(c,status,header.c_str(),"{\"reason\":\"数据库访问错误\"}");
					return false;
				}

				mg_http_reply(c,status,header.c_str(),"{\"reason\":\"注册成功\"}");
				return true;
			}
			//登录模块
			static bool login(struct mg_connection *c,struct mg_http_message* hm){
				//从正文中获取到提交的用户信息---json字符串
				//解析得到用户名和密码

				int rsp_status=200;
				string rsp_body="{\"reason\":\"登录成功\"}";
				string rsp_header="Content-Type:application/json\r\n";//必须以\r\n结尾
				string req_body;
				req_body.assign(hm->body.ptr,hm->body.len);
				Json::Value user;
				Json::Reader reader;
				bool ret=reader.parse(req_body,user);
				if(ret==false){
					rsp_status=400;
					rsp_body="{\"reason\":\"请求格式错误\"}";
					mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
				}
				//验证账号密码是否正确
				ret=_tb_user->VerifyUser(user["name"].asString(),user["passwd"].asString());
				if(ret==false){
					rsp_status=403;
					rsp_body="{\"reason\":\"用户名或密码错误\"}";
					mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
					return false;
				}
				//判断一下状态是否是在线

				ret=_tb_user->VerifyUser_status(user["name"].asString(),ONLINE);
				if(ret==true){
					rsp_status=403;
					rsp_body="{\"reason\":\"该用户已登录\"}";
					mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
					return false;
				}

				//更改登录状态

				ret=_tb_user->UpdateStatus(user["name"].asString(),ONLINE);
				if(ret==false){
					rsp_status=500;
					rsp_body="{\"reason\":\"修改用户状态错误\"}";
					mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
					return false;
				}
				struct session s;
				CreateSession(&s,c,user["name"].asString());//穿建session
				_list.push_back(s);//加入_list链表
				stringstream cookie;
				//cookie<<"Set-Cookie:NAME="<<s.name<<"; path=/; max-age=30000\r\n";
				cookie<<"Set-Cookie:NAME="<<s.name<<"; path=/\r\n";
				//cookie<<"Set-Cookie:SESSION_ID="<<s.session_id<<"; path=/; max-age=30000\r\n";
				cookie<<"Set-Cookie:SESSION_ID="<<s.session_id<<"; path=/\r\n";

				rsp_header+=cookie.str();
				mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
				return true;
			}

			static void Broadcast(const string &msg){
				//句柄结构体内部是链表
				//广播就遍历链表即可
				struct mg_connection* c;
				for(c=_mgr.conns;c!=NULL;c=c->next){
					if(c->is_websocket){
						mg_ws_send(c,msg.c_str(),msg.size(),WEBSOCKET_OP_TEXT);
					}
				}
				return;
			}
			static void callback(struct mg_connection *c,int ev,void *ev_data,void *fn_data){
				struct mg_http_message* hm=(struct mg_http_message*)ev_data;
				struct mg_ws_message *wm=(struct mg_ws_message*)ev_data;
				switch(ev){
					case MG_EV_HTTP_MSG:
						if(mg_http_match_uri(hm,"/reg")){
							//注册的提交表单数据请求
							reg(c,hm);
						}
						else if(mg_http_match_uri(hm,"/login")){
							//登录的提交表单数据请求
							login(c,hm);
						}
						else if(mg_http_match_uri(hm,"/websocket")){
							//websocket的握手请求
							struct mg_str* cookie_str=mg_http_get_header(hm,"Cookie");//获取http头部字段
							if(cookie_str==NULL){
								//处于未登录用户
								string body=R"({"reason":"未登录"})";
								string header="Content-Type:application/json\r\n";
								mg_http_reply(c,403,header.c_str(),body.c_str());

								return;
							}
							string tmp;
							tmp.assign(cookie_str->ptr,cookie_str->len);

							string name;
							GetCookie(tmp,"NAME",&name);
							cout<<name<<endl;
							string msg=name+" 加入聊天室...大家欢迎...";
							cout<<msg<<endl;
							Broadcast(msg);
							struct session s;
							CreateSession(&s,c,name);//穿建session
							_list.push_back(s);//加入_list链表
					
							mg_ws_upgrade(c,hm,NULL);//收到指定的websocket请求之后可以通过这个接口升级到websocket协议
						}
						//else if(mg_http_match_uri(hm,"/index.html")){
						//	struct mg_str* cookie_str=mg_http_get_header(hm,"Cookie");//获取http头部字段
						//	if(cookie_str==NULL){
						//		//处于未登录用户
						//		string body=R"({"reason":"未登录"})";
						//		string header="Content-Type:application/json\r\n";
						//		header+="Location:/login.html\r\n";
						//		mg_http_reply(c,302,header.c_str(),body.c_str());
						
						//		return;
						//	}
								
						//}
						else{
							//静态页面请求
							//除了登录页面,过来的时候都应该检测一下cookie,判断是否登录成功的
							//如果没有检测到session,则应该跳转到登录页面
							if(mg_http_match_uri(hm,"/index.html")){
							struct mg_str* cookie_str=mg_http_get_header(hm,"Cookie");//获取http头部字段
							if(cookie_str==NULL){
								//处于未登录用户
								string body=R"({"reason":"未登录"})";
								string header="Content-Type:application/json\r\n";
								header+="Location:/login.html\r\n";
								mg_http_reply(c,302,header.c_str(),body.c_str());
						
								return;
							}
							
						      }

							struct mg_http_serve_opts opts={.root_dir="./web_root"};
							mg_http_serve_dir(c,hm,&opts);	
						}
						break;
					case MG_EV_WS_MSG:
						{
							string msg;
							msg.assign(wm->data.ptr,wm->data.len);//截取到获得信息	
							Broadcast(msg);//进行广播
						}
						break;
					case MG_EV_CLOSE:
						{
							cout<<c<<endl;
							cout<<"..............................."<<endl;
							for(auto e:_list){
								cout<<e.conn<<endl;
							}
							struct session *ss=GetSessionByConn(c);
							if(ss!=NULL){
								cout<<"进来了"<<endl;
								string msg=ss->name+" 退出聊天室...";
								Broadcast(msg);
								cout<<msg<<endl;
								_tb_user->UpdateStatus(ss->name,OFFLINE);
								DeleteSession(c);
							}	
						}
						break;
					default:
						break;
				}
				return;
			}
		private:
			static TableUser *_tb_user;
			struct _addr;//监听地址信息
			static struct mg_mgr _mgr;//操作句柄 使用mongoose库搭建http服务器
			static struct mg_connection *_lst_http;//监听连接返回值
			static list<struct session> _list;//存储会话信息列表
	};
	TableUser* IM::_tb_user=NULL;
	struct mg_mgr IM::_mgr;
	struct mg_connection* IM::_lst_http=NULL;
	list<struct session> IM::_list;
}

