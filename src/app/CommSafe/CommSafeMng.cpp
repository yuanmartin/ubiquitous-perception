#include "CommSafeMng.h"
#include "log4cxx/Loging.h"
#include "boost/BoostFun.h"
#include "SubTopic.h"
#include "endecode/Base64.h"
#include "comm/CommFun.h"
#include "CommData.h"

using namespace std;
using namespace COMMSAFE_APP;
using namespace common_cmmobj;
using namespace common_template;

using MemberIterType = rapidjson::Value::ConstMemberIterator;

bool CCommSafeMng::Init(Any&& anyObj)
{
	using TupleType = tuple<MsgBusShrPtr,Json,LstTaskCfgType>;
	return apply(bind(&CCommSafeMng::InitParams, this, placeholders::_1, placeholders::_2, placeholders::_3), anyObj.AnyCast<TupleType>());
}

void CCommSafeMng::ReadDBTableData(rapidjson::Document& doc,const string& strSql)
{
	if(m_ptrMsgBus)
	{
		string&& strReadSqliteDbTopic = SqliteReadDBTopic(strAttriSqliteSvrValue);
		m_ptrMsgBus->SendReq<void,const string&, rapidjson::Document&>(move(strSql),doc,move(strReadSqliteDbTopic));
	}
}

using MsgBusShrPtr = std::shared_ptr<common_template::MessageBus>;
template <class A>
void AlgInit(map<string,shared_ptr<A>>& mapShrPtr,const string& strKey,const MsgBusShrPtr& ptrMsgBus,const Json& taskCfg,const Json& algCfg,const vector<Json>& vecDataSrc)
{
	if(0 == mapShrPtr.count(strKey))
	{
		mapShrPtr.insert(pair<string,shared_ptr<A>>(strKey,shared_ptr<A>(new A())));
	}
	mapShrPtr[strKey]->Init(ptrMsgBus,taskCfg,algCfg,vecDataSrc);
}

bool CCommSafeMng::InitParams(const MsgBusShrPtr& ptrMsgBus,const Json& southCfg,const LstTaskCfgType& lstTaskCfg)
{
	//消息总线绑定
	if(!m_ptrMsgBus)
	{
		m_ptrMsgBus = ptrMsgBus;
	
		//tcpserver消息订阅
		auto func = [this](const std::string& strTcpSvrPort, const std::string& strCliConn,const unsigned int& u32Cmd,const char*& pData, const int& s32DataLen)->void {RecTcpProtocolMsg(strTcpSvrPort, strCliConn,u32Cmd,pData,s32DataLen);};
		m_ptrMsgBus->Attach(move(func), TcpSrvRecMsgTopic(string(southCfg[strAttriPort])));
	}
	
	//算法创建和初始化
	for (auto& val : lstTaskCfg)
	{
		//解参数
		Json taskCfg,algCfg;
		vector<Json> vecDataSrc;
		tie(taskCfg,algCfg,vecDataSrc) = val;
	
		//初始化
		string&& strAlgIdx = string(algCfg[strAttriType]) + string(algCfg[strAttriIdx]);
		string&& strTaskIdx = string(taskCfg[strAttriType]) + string(taskCfg[strAttriIdx]);
		if(0 == AlgCfgMap[strDetectAlgTypeIdxValue].compare(strAlgIdx))
		{
			//目标检测
			AlgInit(m_mapDetectShrPtr,strTaskIdx,ptrMsgBus,taskCfg,algCfg,vecDataSrc);
		}
		else if(0 == AlgCfgMap[strActionAlgTypeIdxValue].compare(strAlgIdx))
		{
			//动作识别
		}
		else
		{
			//人脸检测
			AlgInit(m_mapFaceShrPtr,strTaskIdx,ptrMsgBus,taskCfg,algCfg,vecDataSrc);
		}
	}
	return true;
}

template<class A>
void ReportInfo(A& mapShrPtr,Json& jRspData)
{
	if(mapShrPtr.empty())
	{
		jRspData[strAttriStartIdx] =  to_string(0);
		jRspData[strAttriEndIdx] = to_string(-1);
		jRspData[strAttriTotalSize] = to_string(0);
		return;
	}
	mapShrPtr.begin()->second.get()->ReportInfo(jRspData);
}

template<class A>
void ReportBatch(A& mapShrPtr,const string& strReq,Json& jRspData)
{
	if(mapShrPtr.empty())
	{
		return;
	}

	//数据库查询
	Json&& jReqData = Json::parse(strReq);
	int&& s32ReqStartIdx = atoi(string(jReqData[strAttriStartIdx]).c_str());
	int&& s32ReqBatchSize = atoi(string(jReqData[strAttriBatchSize]).c_str());

	vector<Json> vecResult;
	mapShrPtr.begin()->second.get()->ReportResult(vecResult,s32ReqStartIdx,s32ReqBatchSize);
	int&& s32ResultCount = vecResult.size();
	if(0 == s32ResultCount)
	{
		return;
	}
	
	//构建回复
	string&& strRspStartIdx = string(vecResult[0][strAttriIdx]);
	string&& strRspEndIdx = string(vecResult[s32ResultCount - 1][strAttriIdx]);
	jRspData[strAttriStartIdx] = strRspStartIdx;
	jRspData[strAttriEndIdx] = strRspEndIdx;
	jRspData[strAttriBatchSize] = to_string(s32ResultCount);
	for(int i = 0;i < s32ResultCount;i++)
	{
		jRspData[strAttriRecord].push_back(vecResult[i]);
	}
}

//目标检测
void CCommSafeMng::DetectReportInfo(Json& jRspData)
{
	ReportInfo(m_mapDetectShrPtr,jRspData);
}

void CCommSafeMng::DetectReportBatch(const string& strReq,Json& jRspData)
{
	ReportBatch(m_mapDetectShrPtr,strReq,jRspData);
}

//人脸注册和识别
void CCommSafeMng::FaceRegiste(const string& strReq,Json& jRspData,bool bCfgTool)
{
	if(m_mapFaceShrPtr.empty())
	{
		jRspData[strAttriResult] = string(strAttriValueFail);
		return;
	}

	Json jReqData = Json::parse(strReq);

	string&& strIDCard = jReqData[strAttriFaceIDCard];
	string&& strIDName = jReqData[strAttriFaceIDName];
	string&& strWhiteName = jReqData[strAttriWhiteName];

	vector<string> vecBase64ImgData;
	for_each(jReqData[strAttriFaceIDImg].begin(),jReqData[strAttriFaceIDImg].end(),[&](auto& val){vecBase64ImgData.push_back(val[strAttriFaceImgData]);});

	//人脸特征提取
	bool bRet = m_mapFaceShrPtr.begin()->second.get()->FaceRegiste(strIDCard,strIDName,strWhiteName,vecBase64ImgData);
	
	jRspData[strAttriResult] = string(bRet ? strAttriValueOk : strAttriValueFail);
}

void CCommSafeMng::FaceRegisteInfo(Json& jRspData)
{
	if(m_mapFaceShrPtr.empty())
	{
		return;
	}
	m_mapFaceShrPtr.begin()->second.get()->FaceRegisteInfo(jRspData);
}

void CCommSafeMng::FaceReportInfo(Json& jRspData)
{
	ReportInfo(m_mapFaceShrPtr,jRspData);
}

void CCommSafeMng::FaceReportBatch(const string& strReq,Json& jRspData)
{
	ReportBatch(m_mapFaceShrPtr,strReq,jRspData);
}

void CCommSafeMng::RecTcpProtocolMsg(const string& strTcpSvrPort, const string& strCliConn,const unsigned int& u32Cmd,const char*& pData, const int& s32DataLen)
{
	if(!pData)
	{
		return;
	}

	string strReqJson(pData,s32DataLen);
	Json jRspData;

	switch (u32Cmd)
	{
		case 0x41:			//获取目标检测数量信息
		{
			DetectReportInfo(jRspData);
		}
		break;
		case 0x42:			//获取目标检测内容
		{
			DetectReportBatch(strReqJson,jRspData);
		}
		break;
		case 0x71:			//平台端人脸注册(结果直接返回给平台)
		{
			FaceRegiste(strReqJson,jRspData,false);
		}
		break;
		case 0x72:			//配置端人脸注册(保存本地数据库)
		{
			FaceRegiste(strReqJson,jRspData);
		}
		break;
		case 0x73:			//获取所有人脸注册信息列表
		{
			FaceRegisteInfo(jRspData);
		}
		break;
		case 0x74:			//获取人脸告警数量信息
		{
			FaceReportInfo(jRspData);
		}
		break;
		case 0x75:			//获取人脸告警内容
		{
			FaceReportBatch(strReqJson,jRspData);
		}
		break;
		default:
			return;
	}

	if(m_ptrMsgBus)
	{
		LOG_INFO("systerm") << string_format("req cmd(0x%02x) and json:%s\n",u32Cmd,strReqJson.c_str());
		string&& strRspData = jRspData.dump();
		LOG_INFO("systerm") << string_format("rsq cmd(0x%02x) and json:%s\n",u32Cmd,strRspData.c_str());
		m_ptrMsgBus->SendReq<bool,const int &,const string&, const int&,const string&,const int&>(atoi(strTcpSvrPort.c_str()),strCliConn,u32Cmd,jRspData.dump(),1,TcpSrvSendMsgTopic(strTcpSvrPort));
	}
}