#include "FaceAlgMng.h"
#include "log4cxx/Loging.h"
#include "boost/BoostFun.h"
#include "SubTopic.h"
#include <vector>

using namespace std;
using namespace cv;
using namespace Vision_FaceAlg;
using namespace common_cmmobj;
using namespace common_template;

CFaceAlgMng::CFaceAlgMng() :
m_ptrFaceAlg(nullptr),
m_ptrMsgBus(nullptr)
{

}

bool CFaceAlgMng::Init(common_template::Any&& anyObj)
{
    using TupleTypeInit = tuple<MsgBusShrPtr, Json, Json,Json>;
    return apply(bind(&CFaceAlgMng::InitParams, this, placeholders::_1, placeholders::_2, placeholders::_3,placeholders::_4), anyObj.AnyCast<TupleTypeInit>());
}

bool CFaceAlgMng::InitParams(const MsgBusShrPtr& ptrMsgBus, const Json& algCfg,const Json& transferCfg,const Json& dbCfg)
{
    // 消息总线绑定
	if(!ptrMsgBus)
	{
		return false;
	}
	m_ptrMsgBus = ptrMsgBus;

    //消息订阅(算法对象创建)
	auto CreateAlgObjFun = [this](const Json& jTaskCfg,const Json& jAlgCfg,const vector<Json>& jLstDataSrc,const map<int,vector<float>>& mapFaceFeature)->bool{return CreateAlgObj(jTaskCfg,jAlgCfg,jLstDataSrc,mapFaceFeature);};
	m_ptrMsgBus->Attach(move(CreateAlgObjFun),AlgCreateTopic(string(algCfg[strAttriType]),string(algCfg[strAttriIdx])));

	//消息订阅(人脸特征提取)
	auto FeatureExtFun = [this](const cv::Mat& matFaceImg,vector<string>& vecFaceFeature)->void{FaceFeatureExt(matFaceImg,vecFaceFeature);};
	m_ptrMsgBus->Attach(move(FeatureExtFun),FaceFeatureExtTopic(string(algCfg[strAttriType]),string(algCfg[strAttriIdx])));

	//消息订阅(人脸特征库更新)
	auto FeatureUpdateFun = [this](const map<int,vector<float>>& mapFaceFeature)->void{FaceFeatureUpdate(mapFaceFeature);};
	m_ptrMsgBus->Attach(move(FeatureUpdateFun),FaceFeatureUpdateTopic(string(algCfg[strAttriType]),string(algCfg[strAttriIdx])));
    return true;
}

bool CFaceAlgMng::CreateAlgObj(const Json& jTaskCfg,const Json& jAlgCfg,const vector<Json>& jLstDataSrc,const map<int,vector<float>>& mapFaceFeature)
{
	//算法对象与模型一一对应，多个数据源排队等待算法对象计算
	if(!m_ptrFaceAlg)
	{
		m_ptrFaceAlg = FaceAlgShrPtr(new CFaceAlg());
	}

    return m_ptrFaceAlg->Init(m_ptrMsgBus,jTaskCfg,jAlgCfg,jLstDataSrc,mapFaceFeature);;
}

void CFaceAlgMng::FaceFeatureExt(const Mat& matFaceImg,vector<string>& vecFaceFeature)
{
	if(m_ptrFaceAlg)
	{
		m_ptrFaceAlg->FaceFeatureExt(matFaceImg,vecFaceFeature);
	}
}

void CFaceAlgMng::FaceFeatureUpdate(const map<int,vector<float>>& mapFaceFeature)
{
	if(m_ptrFaceAlg)
	{
		m_ptrFaceAlg->SetFaceFeature(mapFaceFeature);
	}
}