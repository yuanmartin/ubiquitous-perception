
#include "HttpClientMng.h"
#include "comm/ReleaseThread.h"
#include "log4cxx/Loging.h"
#include "IHttpServiceModule.h"
#include <algorithm>

using namespace std;
using namespace http_client;

extern "C" bool IF_HTTP_SERVICE::HttpInit()
{
	return true;
}

extern "C" bool IF_HTTP_SERVICE::HttpGet(const string& strAuth,const string& strHost,const int& s32Port,const string& strPag,string& strRspData)
{
    return SCHttpCliMng.HttpGet(strAuth,strHost,s32Port,strPag,strRspData);
}

extern "C" bool IF_HTTP_SERVICE::HttpPost()
{
	return true;
}

bool CHttpClientMng::HttpGet(const string& strAuth,const string& strHost,const int& s32Port,const string& strPag,string& strRspData)
{
    CHttpClient httpCli(strAuth,strHost,s32Port);
    int&& s32Ret = httpCli.Get(strPag,"",strRspData);

    return (0 > s32Ret) ? false : true;
}