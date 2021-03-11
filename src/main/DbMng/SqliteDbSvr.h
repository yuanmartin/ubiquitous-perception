#pragma once
#include "comm/CommDefine.h"
#include "json/JsonCpp.h"

namespace MAIN_MNG
{
    class SqliteDbSvr
    {
    public:
        SqliteDbSvr() = default;
        ~SqliteDbSvr() = default;

        bool Init();
		void DBTableOpt(bool bInit = true);

    private:
        void DbOrTransferTableOpt(std::tuple<std::string,bool>&& tpParam);
		void AlgCfgTableDbOpt(bool bInit = true);
		void TaskCfgTableDbOpt(bool bInit = true);
		void AppCfgTableDbOpt(bool bInit = true);
		void DetectReportTableOpt(const std::string& strTableName);
        void FaceRegisteTableOpt(const std::string& strTableName);
        void FaceReportTableOpt(const std::string& strTableName);
		bool SqliteExcecuteJson(const std::string& strSql,const common_cmmobj::CJsonCpp& jData);
		void SqliteQuery(const std::string& strSql,rapidjson::Document& jData);
		bool SqliteExcecute(const std::string& strSql);
    };    
}