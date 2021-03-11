#pragma once
#include <queue>
#include "comm/CommDefine.h"
#include "rknn/rknn.h"
#include "json/JsonCpp.h"
#include "mutex/LibMutex.h"
namespace COMMSAFE_APP
{
    class CCommSafeDetect
    {
        using MsgBusShrPtr = std::shared_ptr<common_template::MessageBus>;
        using DataSrcMap = std::map<std::string,Json>;
        using TaskLst = std::vector<std::string>;
        using CLockType = common_cmmobj::CLock;
        using CMutexType = common_cmmobj::CMutex;

    public:
        CCommSafeDetect() = default;
        ~CCommSafeDetect() = default;

        void Init(const MsgBusShrPtr& ptrMegBus,const Json& jTaskCfg,const Json& jAlgCfg,const std::vector<Json>& vecDataSrc);
        void ReportInfo(Json& jRspData);
        void ReportResult(std::vector<Json>& vecResult,const int& s32StartIdx,const int& s32BatchSize);

    private:
        void ProResultMsg(const std::string& strIp,const cv::Mat& img,const detection* dets,const int& total,const int& classes);

    private:
        CMutexType m_CfgMutex;
        Json m_jTaskCfg;
        TaskLst m_lstStrTask;
        DataSrcMap m_mapDataSrc;
        MsgBusShrPtr m_ptrMsgBus = nullptr;
    };
}