#include "CommSafeFace.h"
#include "log4cxx/Loging.h"
#include "boost/BoostFun.h"
#include "endecode/Base64.h"
#include "comm/CommFun.h"
#include "CommData.h"
#include "img/ProcImg.h"
#include "json/JsonCpp.h"
#include "CommSafeMng.h"

using namespace std;
using namespace cv;
using namespace COMMSAFE_APP;
using namespace common_cmmobj;
using namespace common_template;

const int s32QueueSize = 5;

void CCommSafeFace::Init(const MsgBusShrPtr& ptrMegBus,const Json& jTaskCfg,const Json& jAlgCfg,const vector<Json>& vecDataSrc)
{
    //配置保存
    SetTaskCfg(jTaskCfg);

    //算法结果订阅
    if(!m_ptrMsgBus)
    {
        //总线绑定
        m_ptrMsgBus = ptrMegBus;

        //一个任务只需要订阅一次
        string&& strCaclResultTopic = AlgResultTopic(string(jTaskCfg[strAttriType]),string(jTaskCfg[strAttriIdx]));
        auto caclResultCB = [this](const string& strIp,const cv::Mat& img,const cv::Rect& rect,const int& faceid,const float& fScore)->void {ProResultMsg(strIp,img,rect,faceid,fScore);};
        ptrMegBus->Attach(move(caclResultCB),strCaclResultTopic);
    }

    //算法创建
    ////算法配置保存
    SetAlgCfg(jAlgCfg);

    ////数据源列表
    SetDataSrc(vecDataSrc);

    ////发送算法创建主题
    map<int,vector<float>> mapFaceFeature;
    ReadFaceFeatureLst(mapFaceFeature);
    string&& strTopic = AlgCreateTopic(string(jAlgCfg[strAttriType]),string(jAlgCfg[strAttriIdx]));
    ptrMegBus->SendReq<bool,const Json&,const Json&,const vector<Json>&,const map<int,vector<float>>&>(jTaskCfg,jAlgCfg,vecDataSrc,mapFaceFeature,strTopic);
}

using MemberIterType = rapidjson::Value::ConstMemberIterator;
void CCommSafeFace::ReadFaceFeatureLst(map<int,vector<float>>& mapFaceFeature)
{
	//数据库读人脸注册表
	rapidjson::Document doc;
	string&& strSql = string_format("select * from %s",strDBFaceRegisteTable);
	SCCommSafeMng.ReadDBTableData(doc,strSql);

    //解析注册表内容
	size_t len = doc.Size();
	for (size_t i = 0; i < len; i++)
	{
        int s32FaceId = 0;
        vector<float> vec;
		for (MemberIterType ite = doc[i].MemberBegin();ite != doc[i].MemberEnd();)
		{
			string strKey(doc[i].GetKey(ite++));
			auto& t = doc[i][strKey.c_str()];
			if(0 == strKey.compare(strAttriId))
			{
                s32FaceId = t.GetInt();
				continue;
			}

            if(0 == strKey.compare(strAttriFaceIDFeature))
            {
                //分割字符串
				stringstream streamFeature(t.GetString());
				string strItem;
				while (std::getline(streamFeature, strItem, ':')) 
				{
                    istringstream streamFloat(strItem);
                    float fValue;
                    while(streamFloat >> fValue)
                    {
                        vec.push_back(fValue);
                    }
                }
            }
        }
        mapFaceFeature.insert(pair<int,vector<float>>(s32FaceId,vec));
    }
}

void CCommSafeFace::ProResultMsg(const string& strIp,const cv::Mat& img,const cv::Rect& rect,const int& faceid,const float& fScore)
{
    Json jDataSrcInfo;
    if(!GetDataSrc(strIp,jDataSrcInfo))
    {
        return;
    }

    if(!m_ptrMsgBus)
    {
        LOG_ERROR("commsafe_app") << "Process alg resutl : message bus is null\n";
        return;
    }

    //人脸画框
    stringstream strStream;
    strStream << "score : " << fScore;

    cv::Mat srcImg;
    img.copyTo(srcImg);
    cv::rectangle(srcImg,rect.tl(), rect.br(), colorArray[2], 3);
    cv::putText(srcImg, strStream.str().c_str(), rect.br(), 1, 2, cv::Scalar(0, 255, 0, 255));
    
    //删除数据库
    string&& strDelSql = string_format("delete from %s where id <= (select max(id) - %d from %s)",strDBFaceReportTable.c_str(),s32QueueSize,strDBFaceReportTable.c_str());
    m_ptrMsgBus->SendReq<bool,const string&>(strDelSql,SqliteDelDBTopic(strAttriSqliteSvrValue));
    
    //保存数据库
    CJsonCpp jcp;
    jcp.StartArray();
    jcp.StartObject();
    jcp.WriteJson(strAttriId.c_str(),nullptr);

    Json jTaskCfg(GetTaskCfg());
    string&& strFlag = string(jTaskCfg[strAttriType]) + string(jTaskCfg[strAttriIdx]);
    jcp.WriteJson(strAttriTaskFlag.c_str(),strFlag.c_str());

    jcp.WriteJson(strAttriAlgIdx.c_str(),strFaceAlgTypeIdxValue.c_str());
    jcp.WriteJson(strAttriAlertType.c_str(),to_string(1).c_str());
    jcp.WriteJson(strAttriAlertDate.c_str(),GetSecondTime().c_str());

    string&& strDataSrc = string(jDataSrcInfo[strAttriType]) + string(jDataSrcInfo[strAttriIdx]) + string(":") + string(jDataSrcInfo[strAttriDesc]);
    jcp.WriteJson(strAttriDataSrc.c_str(),strDataSrc.c_str());

    jcp.WriteJson(strAttriFaceId.c_str(),to_string(faceid).c_str());

    int s32Rows = srcImg.rows / 2;
    int s32Cols = srcImg.cols / 2;
    cv::Mat resizeImg(s32Rows, s32Cols, srcImg.type());
    cv::resize(srcImg, resizeImg, cv::Size(s32Rows, s32Cols));
    jcp.WriteJson(strAttriFaceImg.c_str(),Base64EnImg(resizeImg).c_str());
    
    jcp.EndObject();
    jcp.EndArray();

    string&& strInsertSql = string_format("insert into %s(id,task_flag,alg_idx,alert_type,alert_date,data_src,face_id,face_img) VALUES(?,?,?,?,?,?,?,?);",strDBFaceReportTable.c_str());
    m_ptrMsgBus->SendReq<bool,const string&,const CJsonCpp&>(strInsertSql,jcp,SqliteSaveDBTopic(strAttriSqliteSvrValue));
}

void CCommSafeFace::GetFaceFeature(const string& strBase64FaceImgData,vector<string>& vecFeature)
{
    //base64解码 -> jpeg图像解码
    Mat faceImg;
    Base64DeImg(strBase64FaceImgData,faceImg);

    //特征提取
    Json jAlgCfg(GetAlgCfg());
    string&& strTopic = FaceFeatureExtTopic(string(jAlgCfg[strAttriType]),string(jAlgCfg[strAttriIdx]));
    m_ptrMsgBus->SendReq<void,const Mat&,vector<string>&>(faceImg,vecFeature,strTopic);
}

bool CCommSafeFace::FaceRegiste(const std::string& strIDCard,const string& strIDName,const string& strWhiteName,const vector<string>& vecBase64FaceImgData)
{
    if(vecBase64FaceImgData.empty() || !m_ptrMsgBus)
    {
        return false;
    }

    //人脸注册
    stringstream streamFeature;
    streamFeature.clear();

    stringstream streamFaceImg;
    streamFaceImg.clear();
    for(auto& FaceImgData : vecBase64FaceImgData)
    {
        if(FaceImgData.empty())
        {
            continue;
        }

        ////特征提取
        vector<string> vecFeature;
        GetFaceFeature(FaceImgData,vecFeature);
        if(vecFeature.empty())
        {
            continue;
        }

        streamFeature << vecFeature[0] << ":";
        streamFaceImg << FaceImgData << ":";
    }   
    
    if(0 >= streamFeature.str().length())
    {
        return false;
    }

    //json格式保存数据库
    CJsonCpp jcp;
    jcp.StartArray();
    jcp.StartObject();
    jcp.WriteJson(strAttriId.c_str(),nullptr);
    jcp.WriteJson(strAttriFaceIDCard.c_str(),strIDCard.c_str());
    jcp.WriteJson(strAttriFaceIDName.c_str(),strIDName.c_str());

    jcp.WriteJson(strAttriFaceIDFeature.c_str(),streamFeature.str().c_str());
    jcp.WriteJson(strAttriFaceIDImg.c_str(),streamFaceImg.str().c_str());
    jcp.WriteJson(strAttriWhiteName.c_str(),strWhiteName.c_str());
    jcp.EndObject();
    jcp.EndArray();
    
    string&& strInsertSql = string_format("insert into %s(id,ID_card,ID_name,ID_feature,ID_img,white_name) VALUES(?,?,?,?,?,?);",strDBFaceRegisteTable.c_str());
    string&& strSaveTopic = SqliteSaveDBTopic(strAttriSqliteSvrValue);
    m_ptrMsgBus->SendReq<bool,const string&,const CJsonCpp&>(strInsertSql,jcp,strSaveTopic);
    
    //在线更新算法的人脸库
    map<int,vector<float>> mapFaceFeature;
    ReadFaceFeatureLst(mapFaceFeature);
    Json jAlgCfg(GetAlgCfg());
    string&& strTopic = FaceFeatureUpdateTopic(string(jAlgCfg[strAttriType]),string(jAlgCfg[strAttriIdx]));
    m_ptrMsgBus->SendReq<void,const map<int,vector<float>>&>(mapFaceFeature,strTopic);
    return true;
}

void CCommSafeFace::FaceRegisteInfo(Json& jRspData)
{
	//数据库读人脸注册表
	rapidjson::Document doc;
	string&& strSql = string_format("select * from %s",strDBFaceRegisteTable);
	SCCommSafeMng.ReadDBTableData(doc,strSql);

	//解析注册表内容
	size_t len = doc.Size();
	for (size_t i = 0; i < len; i++)
	{
		Json jSubNode;
		for (MemberIterType ite = doc[i].MemberBegin();ite != doc[i].MemberEnd();)
		{
			//获取json值
			string strKey(doc[i].GetKey(ite++));
			if(strKey.empty())
			{
				break;
			}

			auto& t = doc[i][strKey.c_str()];
			if(0 == strKey.compare(strAttriId))
			{
				jSubNode[strAttriIdx] = to_string(t.GetInt64());
				continue;
			}

			string strValue(t.GetString(),t.GetStringLength());
			if(0 == strKey.compare(strAttriFaceIDImg))
			{
				//分割字符串
				stringstream stream(strValue);
				string strItem;
				while (std::getline(stream, strItem, ':')) 
				{
					Json jImgData;
					jImgData[strAttriFaceImgData] = strItem;
					jImgData[strAttriFaceImgIdx] = string("0");
					jSubNode[strAttriFaceIDImg].push_back(jImgData);
					break;
				}
				continue;
			}

			if(0 == strKey.compare(strAttriFaceIDFeature))
			{
				jSubNode[strKey] = string("");
				continue;
			}

			jSubNode[strKey] = strValue;
		}
		jRspData.push_back(jSubNode);
	}
}

void CCommSafeFace::ReportInfo(Json& jRspData)
{
	//数据库读告警索引
	rapidjson::Document doc;
	string&& strSql = string_format("select id from %s order by id asc",strDBFaceReportTable);
	SCCommSafeMng.ReadDBTableData(doc,strSql);

	//排序告警索引
	int&& len = doc.Size();
	vector<int> vecIdx;
	for(int i = 0;i < len;i++)
	{
		for (MemberIterType ite = doc[i].MemberBegin();ite != doc[i].MemberEnd();)
		{
			vecIdx.push_back(doc[i][doc[i].GetKey(ite++)].GetInt64());
		}
	}

	//回复协议
	int&& s32Min = (0 == len) ? 0 : vecIdx[0];
	int&& s32Max = (0 == len) ? -1 : vecIdx[len - 1];

	jRspData[strAttriStartIdx] =  to_string(s32Min);
	jRspData[strAttriEndIdx] = to_string(s32Max);
	jRspData[strAttriTotalSize] = to_string(len);
}

void CCommSafeFace::ReportResult(std::vector<Json>& vecResult,const int& s32StartIdx,const int& s32BatchSize)
{
    //数据库读取人脸告警信息
	rapidjson::Document doc;
	string&& strSql = string_format("select a.id,a.task_flag,a.alg_idx,a.alert_type,a.alert_date, \
                                    a.data_src, \
                                    a.face_id as idx,a.face_img as ID_img,b.ID_card,b.ID_name,b.white_name \
                                    from %s a inner join %s b on(a.face_id = b.id and a.id >= %d) order by a.id asc",
                                    strDBFaceReportTable.c_str(),
                                    strDBFaceRegisteTable.c_str(),
                                    s32StartIdx);
	SCCommSafeMng.ReadDBTableData(doc,strSql);

	//解析注册表内容
	int len = doc.Size();
    len = (len > s32BatchSize) ? s32BatchSize : len;
	for (size_t i = 0; i < len; i++)
	{
        Json jRecord;
        Json jFaceInfo;
        for (MemberIterType ite = doc[i].MemberBegin();ite != doc[i].MemberEnd();)
		{
			//获取json值
			string strKey(doc[i].GetKey(ite++));
			if(strKey.empty())
			{
				break;
			}

			auto& t = doc[i][strKey.c_str()];
			if(0 == strKey.compare(strAttriId))
			{
				jRecord[strAttriIdx] = to_string(t.GetInt64());
				continue;
			}

			string strValue(t.GetString(),t.GetStringLength());
            if(0 == strKey.compare(strAttriTaskFlag) ||
               0 == strKey.compare(strAttriAlgIdx) ||
               0 == strKey.compare(strAttriAlertType) ||
               0 == strKey.compare(strAttriAlertDate))
            {
                jRecord[strKey] = strValue;
                continue;
            }

            if(0 == strKey.compare(strAttriDataSrc))
            {
                Json jDataSrc;
                vector<string> vecStr;
                Strtok(strValue,":",vecStr);
                if(2 != vecStr.size())
                {
                    jRecord[strAttriDataSrc] = jDataSrc;
                    continue;
                }

                jDataSrc[strAttriTransferId] = vecStr[0];
                jDataSrc[strAttriDesc] = vecStr[1];
                jRecord[strAttriDataSrc] = jDataSrc;
                continue;
            }

            jFaceInfo[strKey] = strValue;
        }

        jRecord[strAttriFaceInfo] = jFaceInfo;
        vecResult.push_back(jRecord);
    }
}