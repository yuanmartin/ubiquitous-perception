#pragma once
#include <map>
#include "comm/Singleton.h"
#include "comm/CommDefine.h"
#include "CfgObj.h"
#include "network/NetTcpDataPkg.h"

namespace MAIN_MNG
{	
	class CCfgMng : public common_template::CSingleton<CCfgMng>
	{
		friend class common_template::CSingleton<CCfgMng>;
		using CfgObjMapType = std::map<std::string,MAIN_MNG::CfgObj>;
		using CfgObjMapTypeShrPtr = std::shared_ptr<CfgObjMapType>;
		using TupleType = std::tuple<std::string,Json,CfgObjMapTypeShrPtr>;

	public:
		bool LoadCfg();

		void GetRuntimeSysCfg(const std::string& strKey,std::vector<Json>& lstCfg);
		bool GetRuntimeSysCfg(const std::string& strKey,Json& cfg);
		void GetExFactorySysCfg(const std::string& strKey,std::vector<Json>& lstCfg);
		bool GetExFactorySysCfg(const std::string& strKey,Json& cfg);

	private:
		CCfgMng() = default;
		~CCfgMng() = default;

	private:
		void LoadExFactorySysCfgInFile(const std::string& strPath);

		void LoadRuntimeSysCfgInDb();

		void LoadRuntimeSysCfgTableInDb(const std::string& strTableName);

		void SaveJsonObj(const std::string& strKey,Json& elm,const CfgObjMapTypeShrPtr& cfgMapShrPtr);

		void ParseJsonNodeObj(TupleType&& jCfg);

		void ParseJson(const Json& jObj,const CfgObjMapTypeShrPtr& mapExFactoryCfgObjShrPtr);

		void RuntimeSysCfgUpdate(const std::string& strRootNode,Json& jSubNode);

		void CleanRuntimeSysCfgMap();

		void RecTcpProtocolMsg(const std::string& strTcpSvrPort, const std::string& strCliConn,const unsigned int& u32Cmd,const char*& pData, const int& s32DataLen);

	private:
		Json m_jExFactorySysCfg;
		CfgObjMapTypeShrPtr m_mapExFactorySysCfgShrPtr = CfgObjMapTypeShrPtr(new CfgObjMapType());
		
		Json m_jRuntimeSysCfg;
		CfgObjMapTypeShrPtr m_mapRuntimeSysCfgShrPtr = CfgObjMapTypeShrPtr(new CfgObjMapType());
	};

#define SCCfgMng (common_template::CSingleton<CCfgMng>::GetInstance())
}