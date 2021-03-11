#pragma once
#include <vector>
#include <list>
#include <map>
#include <atomic>
#include "HttpClient.h"
#include "comm/Singleton.h"
#include "comm/Any.h"
#include "mutex/LibMutex.h"

namespace http_client
{
	class CHttpClientMng : public common_template::CSingleton<CHttpClientMng>
	{
		friend class common_template::CSingleton<CHttpClientMng>;
		using CHttpCliShrPtr = boost::shared_ptr<CHttpClient>;

	public:
		void Init(const char* pAppIp, int nApport){}
        bool HttpGet(const std::string& strAuth,const std::string& strHost,const int& s32Port,const std::string& strPag,std::string& strRspData);

	private:
		CHttpClientMng() = default;
		virtual ~CHttpClientMng() = default;

	private:
		std::string m_strAppServerIp;
		std::string m_strDiscernSysCode;

		common_cmmobj::CMutex m_Mutex;
        std::map<std::string,CHttpCliShrPtr> m_mapHttpCli;

		std::atomic_bool m_bInited = false;
	};

#define  SCHttpCliMng (common_template::CSingleton<CHttpClientMng>::GetInstance())
}
