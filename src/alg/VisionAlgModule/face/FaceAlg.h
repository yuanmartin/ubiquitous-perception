#pragma once
#include "FaceDetection.h"
#include "FaceAlignment.h"
#include "FaceQuality.h"
#include "FacePos.h"
#include "FaceIdentification.h"
#include "mutex/LibMutex.h"

namespace Vision_FaceAlg
{
    #pragma pack(push, 1)  
    typedef struct stFaceIdentify
    {
        stFaceIdentify() = default;

        stFaceIdentify(const stFaceIdentify& obj)
        {
            s32FaceId = obj.s32FaceId;
            fScore = obj.fScore;
        }

        stFaceIdentify& operator=(const stFaceIdentify& obj)
        {
            if(this == &obj)
            {
                return *this;
            }

            s32FaceId = obj.s32FaceId;
            fScore = obj.fScore;
            return *this;
        }

        bool operator()(const stFaceIdentify& obj1,const stFaceIdentify& obj2)
        {
            return obj1.fScore > obj2.fScore;
        }

        int s32FaceId;
        float fScore;
    }TFaceIdentify;
    #pragma pack(pop)
    class CFaceAlg
    {
        using MsgBusShrPtr = std::shared_ptr<common_template::MessageBus>;
        using FaceDetectShrPtr = std::shared_ptr<Vision_FaceAlg::FaceDetection>;
        using FaceAlignShrPtr = std::shared_ptr<Vision_FaceAlg::FaceAlignment>;
        using FaceQualityShrPtr = std::shared_ptr<Vision_FaceAlg::FaceQuality>;
        using FaceIdentifyShrPtr = std::shared_ptr<Vision_FaceAlg::FaceIdentification>;
        using ResultReportTopicMapType = std::map<std::string,int>;
        using FaceFeatureMapType = std::map<int,std::vector<float>>;
        using CLockType = common_cmmobj::CLock;
        using DataSrcTopicMapType = std::map<std::string,std::string>;
        using CMutexType = common_cmmobj::CMutex;

    public:
        CFaceAlg() = default;
        ~CFaceAlg() = default;

        bool Init(const MsgBusShrPtr& ptrMsgBus,const Json& taskCfg,const Json& algCfg,const std::vector<Json>& lstDataSrcCfg,const std::map<int,std::vector<float>>& mapFaceFeature);
        void FaceFeatureExt(const cv::Mat& matFaceImg,std::vector<std::string>& vecFeature);

        inline void SetFaceFeature(const FaceFeatureMapType& mapFaceFeature)
        {
            CLockType lock(&m_mapFaceFeatureMutex);
            m_mapFaceFeature.clear();
            m_mapFaceFeature = mapFaceFeature;
        }

    private:
        template<class T>
        void AlgInit(T&& ptr,const Json& algJson)
        {
            std::forward<T>(ptr)->Init(algJson);
        }

        template<typename... Args>
        inline void AlgInitLst(const Json& algJson,Args&&... args)
        {
            std::initializer_list<int>{(AlgInit(std::forward<Args>(args),algJson), 0)...};
        }

        bool DataSrcInit(const std::vector<Json>& lstDataSrcCfg);

        bool PreprocMat(const cv::Mat& srcImg,cv::Mat& dstImg,int& s32Width,int& s32Height);

        void FaceFeatureExt(const cv::Mat& matFaceImg,std::vector<std::vector<float>>& vecFeature,std::vector<cv::Mat>& vecRoiMat,std::vector<Vision_FaceAlg::FaceRectInfo>& vecFaceRect);
        
        void ProcVideoStream(const std::string& strIp,const std::string& strCameCode,const cv::Mat& matImg);

        inline bool GetInitStatus()
        {
            return m_bAlgInit;
        }

        inline void SetInitStatus(bool bStatus = true)
        {
            m_bAlgInit = bStatus;
        }

        inline void SetFeatureDim()
        {
            m_s32FeatureDim = m_ptrFaceIdentify->GetFeatureDim();
        }

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
        MsgBusShrPtr m_ptrMsgBus = nullptr;
        std::atomic_bool m_bAlgInit = false;
        int m_s32FeatureDim = 0;
        
        FaceDetectShrPtr m_ptrFaceDetect = FaceDetectShrPtr(new FaceDetection());
        FaceAlignShrPtr m_ptrFaceAlign = FaceAlignShrPtr(new FaceAlignment());
        FaceQualityShrPtr m_ptrFaceQuality = FaceQualityShrPtr(new FaceQuality());
        FaceIdentifyShrPtr m_ptrFaceIdentify = FaceIdentifyShrPtr(new FaceIdentification());
        
        CMutexType m_mapFaceFeatureMutex;
        FaceFeatureMapType m_mapFaceFeature;

        CMutexType m_mapReportTopicMutex;
        ResultReportTopicMapType m_mapResultReportTopic;

        DataSrcTopicMapType m_mapDataSrcTopic;
    };
}