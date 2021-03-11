#include <string>
#include <vector>
#include <map>
#include <functional>
#include "IHttpServiceModule.h"
#include "comm/Singleton.h"
#include "CfgMng/CfgMng.h"
#include "thread/Thread.h"
#include "comm/CommDefine.h"

namespace MAIN_MNG
{
    class CHttpTransferMng : public common_template::CSingleton<CHttpTransferMng>
    {
        friend class common_template::CSingleton<CHttpTransferMng>;
        using ThreadShrPtr = std::shared_ptr<common_cmmobj::CThread>;

    public:
        bool StartHttpCli();
        bool HttpGet(const std::string& strAuth,const std::string& strHost,const int& s32Port,const std::string& strPag,std::string& strRspData);

    private:
        CHttpTransferMng() = default;
        virtual ~CHttpTransferMng() = default;

    private:
        //心跳线程
    	ThreadShrPtr m_ptrOnlineThread;

		//http
		std::string m_strRpcServiceTopic;
		std::string m_strRpcSendDataTopic;
    };
    #define SCHttpTransferMng (common_template::CSingleton<CHttpTransferMng>::GetInstance())
}