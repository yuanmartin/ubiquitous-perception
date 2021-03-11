#pragma once
#include <string>
#include <map>
#include <vector>
#include "comm/CommDefine.h"
#include "rknn/rknn.h"
#include "json/JsonCpp.h"
#include "mutex/LibMutex.h"
#include "SubTopic.h"
namespace COMMSAFE_APP
{
    class CCommSafeFace
    {
        using MsgBusShrPtr = std::shared_ptr<common_template::MessageBus>;
        using DataSrcMap = std::map<std::string,Json>;
        using CLockType = common_cmmobj::CLock;
        using CMutexType = common_cmmobj::CMutex;

    public:
        CCommSafeFace() = default;
        ~CCommSafeFace() = default;

        void Init(const MsgBusShrPtr& ptrMegBus,const Json& jTaskCfg,const Json& jAlgCfg,const std::vector<Json>& vecDataSrc);
        bool FaceRegiste(const std::string& strIDCard,const std::string& strIDName,const std::string& strWhiteName,const std::vector<std::string>& vecBase64FaceImgData);
        void FaceRegisteInfo(Json& jRspData);
        void ReportInfo(Json& jRspData);
        void ReportResult(std::vector<Json>& vecResult,const int& s32StartIdx,const int& s32BatchSize);

    private:
        void GetFaceFeature(const string& strBase64FaceImgData,std::vector<std::string>& vecFeature);
        void ProResultMsg(const string& strIp,const cv::Mat& img,const cv::Rect& rect,const int& faceid,const float& fScore);
        void ReadFaceFeatureLst(std::map<int,std::vector<float>>& mapFaceFeature);

        inline void SetTaskCfg(const Json& jCfg)
        {
            CLockType lock(&m_TaskCfgMutex);
            m_jTaskCfg.clear();
            m_jTaskCfg = jCfg;
        }

        inline Json GetTaskCfg()
        {
            CLockType lock(&m_TaskCfgMutex);
            return Json(m_jTaskCfg);
        }

        inline void SetAlgCfg(const Json& jCfg)
        {
            CLockType lock(&m_AlgCfgMutex);
            m_jAlgCfg.clear();
            m_jAlgCfg = jCfg;
        }

        inline Json GetAlgCfg()
        {
            CLockType lock(&m_AlgCfgMutex);
            return Json(m_jAlgCfg);
        }

        inline void SetDataSrc(const std::vector<Json>& vecDataSrc)
        {
            CLockType lock(&m_DataSrcMutex);
            m_mapDataSrc.clear();
            for(const auto& DataSrc : vecDataSrc)
            {
                string&& strIpKey = DataSrc[strAttriIp];
                m_mapDataSrc.insert(pair<string,Json>(strIpKey,DataSrc));
            }
        }

        inline bool GetDataSrc(const std::string& strKey,Json& jDataSrcCfg)
        {
            CLockType lock(&m_DataSrcMutex);
            if(0 == m_mapDataSrc.count(strKey))
            {
                return false;
            }
            jDataSrcCfg = m_mapDataSrc[strKey];
            return true;
        }

    private:
        MsgBusShrPtr m_ptrMsgBus = nullptr;

        CMutexType m_TaskCfgMutex;
        Json m_jTaskCfg;

        CMutexType m_AlgCfgMutex;
        Json m_jAlgCfg;

        CMutexType m_DataSrcMutex;
        DataSrcMap m_mapDataSrc;
    };
}