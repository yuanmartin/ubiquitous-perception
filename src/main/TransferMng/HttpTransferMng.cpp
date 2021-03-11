#include "HttpTransferMng.h"
#include "MsgBusMng/MsgBusMng.h"
#include "log4cxx/Loging.h"
#include "SubTopic.h"
#include "comm/ReleaseThread.h"
#include "IHttpServiceModule.h"

using namespace std;
using namespace MAIN_MNG;
using namespace common_template;
using namespace common_cmmobj;

bool CHttpTransferMng::StartHttpCli()
{
    return true;
}

bool CHttpTransferMng::HttpGet(const std::string& strAuth,const std::string& strHost,const int& s32Port,const std::string& strPag,std::string& strRspData)
{
    return IF_HTTP_SERVICE::HttpGet(strAuth,strHost,s32Port,strPag,strRspData);
}