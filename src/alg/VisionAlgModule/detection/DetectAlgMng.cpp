#include "DetectAlgMng.h"
#include "log4cxx/Loging.h"
#include "boost/BoostFun.h"
#include "SubTopic.h"
#include <vector>

using namespace std;
using namespace Vision_DetectAlg;
using namespace common_cmmobj;
using namespace common_template;

bool CDetectAlgMng::Init(common_template::Any&& anyObj)
{
	using TupleTypeInit = tuple<MsgBusShrPtr, Json, Json,Json>;
	return apply(bind(&CDetectAlgMng::InitParams, this, placeholders::_1, placeholders::_2, placeholders::_3,placeholders::_4), anyObj.AnyCast<TupleTypeInit>());
}

bool CDetectAlgMng::InitParams(const MsgBusShrPtr& ptrMsgBus, const Json& algCfg,const Json& transferCfg,const Json& dbCfg)
{
	// 消息总线绑定
	if(!ptrMsgBus)
	{
		return false;
	}
	m_ptrMsgBus = ptrMsgBus;
	
	//消息订阅(算法对象创建)
	string&& strTopic = AlgCreateTopic(string(algCfg[strAttriType]),string(algCfg[strAttriIdx]));
	auto func = [this](const Json& jTaskCfg,const Json& jAlgCfg,const vector<Json>& jLstDataSrc)->bool{return CreateAlgObj(jTaskCfg,jAlgCfg,jLstDataSrc);};
	m_ptrMsgBus->Attach(move(func),strTopic);

	return true;
}

bool CDetectAlgMng::CreateAlgObj(const Json& jTaskCfg,const Json& jAlgCfg,const vector<Json>& jLstDataSrc)
{
	for(auto DataSrc : jLstDataSrc)
	{
		//检测算法对象与数据源一一对应，一个算法对象加载和运算多个模型
		string&& strKey = string(DataSrc[strAttriType]) + string(DataSrc[strAttriIdx]);
		auto ite = m_mapDetectAlgShrPtr.find(strKey);
		if(ite == m_mapDetectAlgShrPtr.end())
		{
			m_mapDetectAlgShrPtr.insert(make_pair<string,DetectAlgShrPtr>(string(strKey),DetectAlgShrPtr(new CDetectAlg())));
		}

		m_mapDetectAlgShrPtr[strKey]->Init(m_ptrMsgBus,jTaskCfg,jAlgCfg,DataSrc);
	}
}