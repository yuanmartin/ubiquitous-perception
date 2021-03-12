#include "CommSafeDetect.h"
#include "log4cxx/Loging.h"
#include "boost/BoostFun.h"
#include "SubTopic.h"
#include "comm/CommFun.h"
#include "CommData.h"
#include "img/ProcImg.h"
#include "CommSafeMng.h"

using namespace std;
using namespace COMMSAFE_APP;
using namespace common_cmmobj;
using namespace common_template;

const int s32QueueSize = 20;

bool IsOverlap(box father, box child)
{
    bool bRet = false;
    float x1 = father.x - father.w / 2;
    float x2 = father.x + father.w / 2;
    float y1 = father.y - father.h / 2;
    float y2 = father.y + father.h / 2;
    float y3 =  child.y +  child.h / 2;

    if(y3 > y1 && y3 < y2 && child.x > x1 && child.x < x2)
    {
        return true;
    }
    return bRet;
}

bool DetectInference(cv::Mat& img,const detection* dets,const int& total,const int& classes,const std::string& strTask,vector<box>& vecBox)
{
    bool bHaveTarget = false;
	for(int i = 0;i < total;i++)
	{
        int s32TopCls = dets[i].sort_class;
        float fTopScore = dets[i].prob[s32TopCls];
        LOG_INFO("commsafe_app") << string_format("ImgDetect TopCls : %d and TopScore : %f\n",s32TopCls,fTopScore);
        
        if(0 != strTask.compare(to_string(s32TopCls)) || 0.0 >= fTopScore)
        {
            continue;
        }

        //画框
        box b = dets[i].bbox;
        int x1 = (b.x - b.w / 2.) * img.cols;
        int x2 = (b.x + b.w / 2.) * img.cols;
        int y1 = (b.y - b.h / 2.) * img.cols - (img.cols - img.rows) / 2;
        int y2 = (b.y + b.h / 2.) * img.cols - (img.cols - img.rows) / 2;

        stringstream strStream;
        strStream << s32TopCls << " : " << fTopScore;

        cv::rectangle(img, cv::Point(x1, y1), cv::Point(x2, y2), colorArray[s32TopCls % 10], 3);
        cv::putText(img, strStream.str().c_str(), cv::Point(x1, y1 - 12), 1, 2, cv::Scalar(0, 255, 0, 255));

        bHaveTarget = true;

       vecBox.push_back(b);
	}

    LOG_INFO("commsafe_app") << "ImgDetect Complete ......\n";

	return bHaveTarget;
}

void CCommSafeDetect::Init(const MsgBusShrPtr& ptrMsgBus,const Json& jTaskCfg,const Json& jAlgCfg,const vector<Json>& vecDataSrc)
{
    CLockType lock(&m_CfgMutex);

    //整个任务的配置
    m_jTaskCfg.clear();
    m_jTaskCfg = jTaskCfg;

    //任务列表
    m_lstStrTask.clear();
    Strtok(string(jTaskCfg[strAttriTaskLst]),"|",m_lstStrTask);

    //消息总线
    if(!m_ptrMsgBus)
    {
        m_ptrMsgBus = ptrMsgBus;
    
        //算法结果订阅(只需一次)
        string&& strCaclResultTopic = AlgResultTopic(string(jTaskCfg[strAttriType]),string(jTaskCfg[strAttriIdx]));
        auto caclResultCB = [this](const string& strIp,const cv::Mat& img,const detection* dets,const int& total,const int& classes)->void {ProResultMsg(strIp,img,dets,total,classes);};
        ptrMsgBus->Attach(move(caclResultCB),strCaclResultTopic);
    }

    //算法创建
    m_mapDataSrc.clear();
    for(const auto& DataSrc : vecDataSrc)
    {
        string&& strIpKey = DataSrc[strAttriIp];
        m_mapDataSrc.insert(pair<string,Json>(strIpKey,DataSrc));
    }
    string&& strTopic = AlgCreateTopic(string(jAlgCfg[strAttriType]),string(jAlgCfg[strAttriIdx]));
    ptrMsgBus->SendReq<bool,const Json&,const Json&,const vector<Json>&>(jTaskCfg,jAlgCfg,vecDataSrc,strTopic);
}

void CCommSafeDetect::ProResultMsg(const string& strIp,const cv::Mat& img,const detection* dets,const int& total,const int& classes)
{
    CLockType lock(&m_CfgMutex);

    //过滤非配数据源识别结果
    if(0 == m_mapDataSrc.count(strIp))
    {
        return;
    }

    vector<box> vecPersonBox;
    vector<box> vecHelmetBox;
    bool bPersonTask = false;
    bool bHelmetTask = false;

    //算法后处理
    for (const auto& task : m_lstStrTask)
    {
        if(0 == task.compare(to_string(0)))
        {
            bPersonTask = true;
        }

        if(0 == task.compare(to_string(23)))
        {
            bHelmetTask = true;
        }

        //结果推断
        cv::Mat srcImg;
        img.copyTo(srcImg);
        vector<box> vecBox;
        if(!DetectInference(srcImg,dets,total,classes,task,vecBox))
        {
            continue;
        }

        //删除数据库
        string&& strDelSql = string_format("delete from %s where id <= (select max(id) - %d from %s)",strDBDetectReportTable.c_str(),s32QueueSize,strDBDetectReportTable.c_str());
        m_ptrMsgBus->SendReq<bool,const string&>(strDelSql,SqliteDelDBTopic(strAttriSqliteSvrValue));

        //保存数据库
        CJsonCpp jcp;
        jcp.StartArray();
        jcp.StartObject();

        jcp.WriteJson(strAttriId.c_str(),nullptr);

        string&& strFlag = string(m_jTaskCfg[strAttriType]) + string(m_jTaskCfg[strAttriIdx]);
        jcp.WriteJson(strAttriTaskFlag.c_str(),strFlag.c_str());

        jcp.WriteJson(strAttriAlgIdx.c_str(),strDetectAlgTypeIdxValue.c_str());
        jcp.WriteJson(strAttriAlertType.c_str(),task.c_str());
        jcp.WriteJson(strAttriAlertDate.c_str(),GetSecondTime().c_str());

        auto&& ite = m_mapDataSrc[strIp];
        string&& strDataSrc = string(ite[strAttriType]) + string(ite[strAttriIdx]) + string(":") + string(ite[strAttriDesc]);
        jcp.WriteJson(strAttriDataSrc.c_str(),strDataSrc.c_str());

        int s32Rows = srcImg.rows / 2;
        int s32Cols = srcImg.cols / 2;
        cv::Mat resizeImg(s32Rows, s32Cols, srcImg.type());
        cv::resize(srcImg, resizeImg, cv::Size(s32Rows, s32Cols));
        jcp.WriteJson(strAttriImg.c_str(),Base64EnImg(resizeImg).c_str());

        jcp.EndObject();
        jcp.EndArray();

        string&& strInsertSql = string_format("insert into %s(id,task_flag,alg_idx,alert_type,alert_date,data_src,img) VALUES(?,?,?,?,?,?,?);",strDBDetectReportTable.c_str());
        m_ptrMsgBus->SendReq<bool,const string&,const CJsonCpp&>(strInsertSql,jcp,SqliteSaveDBTopic(strAttriSqliteSvrValue));

        //人框
        if(0 == task.compare(to_string(0)))
        {
            vecPersonBox.assign(vecBox.begin(),vecBox.end());
            LOG_INFO("commsafe_app") << string_format("detect persons : %d\n",vecPersonBox.size());
        }

        //头盔
        if(0 == task.compare(to_string(23)))
        {
            vecHelmetBox.assign(vecBox.begin(),vecBox.end());
            LOG_INFO("commsafe_app") << string_format("detect helmets : %d\n",vecHelmetBox.size());
        }
    }

    //告警语音播放
    if(!(bPersonTask && bHelmetTask))
    {
        return;
    }    

    bool bAlarm = true;
    int s32PersonBoxs = vecPersonBox.size();
    int s32HelmetBoxs = vecHelmetBox.size();
    if(0 == s32PersonBoxs)
    {
        return;
    }

    LOG_INFO("commsafe_app") << string_format("PersonBoxs : %d and HelmetBoxs : %d\n",s32PersonBoxs,s32HelmetBoxs);
    if(s32PersonBoxs <= s32HelmetBoxs)
    {
        for(auto& personBox : vecPersonBox)
        {
            bool bRet = false;
            for(auto& helmetBox : vecHelmetBox)
            {
                if(IsOverlap(personBox,helmetBox))
                {
                    bRet = true;
                    break;
                }
            }

            if(!bRet)
            {
                bAlarm = true;
                break;
            }
            else
            {
                bAlarm = false;
            }
        }
    }

    if(bAlarm)
    {
        system("aplay -Dplughw:0,0 -c 2 -r 44100 -f S16_LE ./resources/nohelmet.wav");
    }
}

using MemberIterType = rapidjson::Value::ConstMemberIterator;
void CCommSafeDetect::ReportInfo(Json& jRspData)
{
	//数据库读告警索引
	rapidjson::Document doc;
	string&& strSql = string_format("select id from %s order by id asc",strDBDetectReportTable);
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

void CCommSafeDetect::ReportResult(std::vector<Json>& vecResult,const int& s32StartIdx,const int& s32BatchSize)
{
    //数据库读取人脸告警信息
	rapidjson::Document doc;
	string&& strSql = string_format("select * from %s where id >= %d order by id asc",
                                    strDBDetectReportTable.c_str(),
                                    s32StartIdx);
	SCCommSafeMng.ReadDBTableData(doc,strSql);

	//解析注册表内容
	int len = doc.Size();
    len = (len > s32BatchSize) ? s32BatchSize : len;
	for (size_t i = 0; i < len; i++)
	{
        Json jRecord;
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
                jRecord[strKey] = jDataSrc;
                continue;
            }

            jRecord[strKey] = strValue;
        }
        vecResult.push_back(jRecord);
    }
}