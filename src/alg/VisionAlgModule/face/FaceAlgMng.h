#pragma once
#include "FaceAlg.h"
#include "comm/CommDefine.h"

namespace Vision_FaceAlg
{
    class CFaceAlgMng : public common_template::CSingleton<CFaceAlgMng>
    {
        friend class common_template::CSingleton<CFaceAlgMng>;
        using MsgBusShrPtr = std::shared_ptr<common_template::MessageBus>;
        using FaceAlgShrPtr = std::shared_ptr<Vision_FaceAlg::CFaceAlg>;
        
    public:
        bool Init(common_template::Any&& anyObj);

    protected:
        CFaceAlgMng();
        ~CFaceAlgMng() = default;

    private:
        bool InitParams(const MsgBusShrPtr& ptrMegBus, const Json& algCfg,const Json& transferCfg,const Json& dbCfg);
		bool CreateAlgObj(const Json& jTaskCfg,const Json& jAlgCfg,const std::vector<Json>& jLstDataSrc,const std::map<int,std::vector<float>>& mapFaceFeature);
        void FaceFeatureExt(const cv::Mat& matFaceImg,std::vector<std::string>& vecFaceFeature);
        void FaceFeatureUpdate(const std::map<int,std::vector<float>>& mapFaceFeature);
	
    private:
		MsgBusShrPtr m_ptrMsgBus;
        FaceAlgShrPtr m_ptrFaceAlg;
	};
	#define SFaceAlgMng (common_template::CSingleton<CFaceAlgMng>::GetInstance())
}