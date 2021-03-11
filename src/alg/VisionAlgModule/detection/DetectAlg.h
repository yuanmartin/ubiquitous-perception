#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <atomic>
#include <queue>
#include <thread>
#include <mutex>
#include <chrono>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include <sys/time.h>
#include <sys/syscall.h>
#include "comm/CommDefine.h"
#include "rknn/rknn.h"
#include "mutex/LibMutex.h"

namespace Vision_DetectAlg
{
    class CDetectAlg
    {
        using MsgBusShrPtr = std::shared_ptr<common_template::MessageBus>;
        using AlgModelMapType = std::map<std::string,rknn_context>;
        using ResultReportTopicLstType = std::vector<std::string>;
        using ResultReportTopicMapType = std::map<std::string,int>;
        using DataSrcTopicMapType = std::map<std::string,std::string>;
        using CLockType = common_cmmobj::CLock;
        using CMutexType = common_cmmobj::CMutex;

    public:
        CDetectAlg() = default;
        ~CDetectAlg(){ Release();};

		bool Init(const MsgBusShrPtr& ptrMsgBus,const Json& taskCfg,const Json& algCfg,const Json& DataSrcCfg);

    private:
        bool AlgInit(const Json& algCfg);

        bool DataSrcInit(const Json& DataSrcCfg);

        void ProcMat(const std::string& strIp,const std::string& strCameCode,const cv::Mat& matImg);

        void Release();

        inline void SetReportTopic(const std::string& strTopicKey)
        {
            CLockType lock(&m_mapReportTopicMutex);
            auto ite = m_mapResultReportTopic.find(strTopicKey);
            if(ite == m_mapResultReportTopic.end())
            {
                m_mapResultReportTopic[strTopicKey] = 0;
            }
        }

    private:
        MsgBusShrPtr m_ptrMsgBus;

        CMutexType m_mapReportTopicMutex;
        ResultReportTopicMapType m_mapResultReportTopic;

        DataSrcTopicMapType m_mapDataSrcTopic;
        AlgModelMapType m_mapAlgModel;
    };
    
}