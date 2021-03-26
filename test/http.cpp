#include<iostream>
#include"../mongoose.h"

void callback(struct mg_connection *c,int ev,void *ev_data,void *fn_data){
	while(ev){
		case MG_EV_HTTP_MSG:
			break;
		case MG_EV_WS_MSG:
			break;
		case MG_EV_CLOSE:
			break;
		default:
			break;
	}
}
int main(){
	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	struct mg_connection *lst_http;
	lst_http=mg_http_listen(&mgr,"0.0.0.0:9000",callback,NULL);
	if(lst_http==NULL){
		return -1
	}
	while(1){
		mg_mgr_poll(&mgr,1000);
	}
	return 0;
}
