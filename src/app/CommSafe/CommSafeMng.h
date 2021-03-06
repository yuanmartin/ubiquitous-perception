#include <vector>
#include "comm/CommDefine.h"
#include "rknn/rknn.h"
#include "CommSafeDetect.h"
#include "CommSafeFace.h"
namespace COMMSAFE_APP
{
	class CCommSafeMng : public common_template::CSingleton<CCommSafeMng>
	{
		friend class common_template::CSingleton<CCommSafeMng>;
        using MsgBusShrPtr = std::shared_ptr<common_template::MessageBus>;
        using TaskCfgType = tuple<Json,Json,std::vector<Json>>;
        using LstTaskCfgType = std::vector<TaskCfgType>;
        using DetectShrPtr = std::shared_ptr<COMMSAFE_APP::CCommSafeDetect>;
        using FaceShrPtr = std::shared_ptr<COMMSAFE_APP::CCommSafeFace>;
        using DetectShrPtrMap = std::map<std::string,DetectShrPtr>;
        using FaceShrPtrMap = std::map<std::string,FaceShrPtr>;

    public:
        bool Init(common_template::Any&& anyObj);
        void ReadDBTableData(rapidjson::Document& doc,const std::string& strSql);

    private:
        CCommSafeMng() = default;
        ~CCommSafeMng() = default;
        
        bool InitParams(const MsgBusShrPtr& ptrMegBus,const Json& southCfg,const LstTaskCfgType& taskLstCfg);
        void RecTcpProtocolMsg(const string& strTcpSvrPort, const string& strCliConn,const unsigned int& u32Cmd,const char*& pData, const int& s32DataLen);
        
    private:
        void DetectReportInfo(Json& jRspData);
        void DetectReportBatch(const std::string& strReq,Json& jRspData);

        void FaceRegiste(const std::string& strReq,Json& jRspData,bool bCfgTool = true);
        void FaceRegisteInfo(Json& jRspData);
        void FaceReportInfo(Json& jRspData);
        void FaceReportBatch(const std::string& strReq,Json& jRspData);
        
    private:
		MsgBusShrPtr m_ptrMsgBus = nullptr;

        DetectShrPtrMap m_mapDetectShrPtr;

        FaceShrPtrMap m_mapFaceShrPtr;
    };

    #define SCCommSafeMng (common_template::CSingleton<CCommSafeMng>::GetInstance())
}