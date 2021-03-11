#pragma once
#include <string>

//common attribe and value
const std::string strAttriId("id");
const std::string strAttriNode("node");
const std::string strAttriType("type");
const std::string strAttriIp("ip");
const std::string strAttriPort("port");
const std::string strAttriParams("params");
const std::string strAttriEnable("enable");
const std::string strAttriIdx("idx");
const std::string strAttriPwd("pwd");
const std::string strAttriDesc("desc");
const std::string strAttriResult("result");
const std::string strAttriValueOk("ok");
const std::string strAttriValueFail("fail");

//Transfer 
////root node and sub node
const std::string strTransferNode("Transfer");
const std::string strTransferSouthTcpSvrNode("SouthTcpSvr");
const std::string strTransferSouthSerialNode("SouthSerial");
const std::string strTransferSouthVideoNode("SouthVideo");
const std::string strTransferNorthRpcNode("NorthRpcCli");

////attribute and value
const std::string strAttriTcpSrvValue("tcpsrv");

const std::string strAttriSerialValue("serial");

const std::string strAttriRtspCliValue("rtspcli");
const std::string strAttriUserName("username");
const std::string strAttriCode("code");

const std::string strAttriRpcCliValue("rpcli");
const std::string strAttriConn("conn");

const std::string strAttriHttpCliValue("httpcli");

const std::string strAttriTcpCliValue("tcpcli");
const std::string strAttriThreads("threads");

////rpc topic
const std::string strRpcService("RpcService");
const std::string strSendRpcData("SendRpcData");
static std::string RpcServiceTopic(const std::string& strIp, const std::string& strPort)
{
	return std::string(strAttriRpcCliValue + strIp + strPort + strRpcService);
}

static std::string RpcSendTopic(const std::string& strIp, const std::string& strPort)
{
	return std::string(strAttriRpcCliValue + strIp + strPort + strSendRpcData);
}

////tcpserver topic
const std::string strRevConn("tcpcli_conn");
const std::string strSendConn("tcpcli_close");
const std::string strRevMsg("tcpcli_revmsg");
const std::string strSendMsg("tcpcli_sendmsg");
static std::string TcpCliConnTopic(const std::string& strPort)
{
	return std::string(strPort + strRevConn);
}

static std::string TcpSrvCloseConnTopic(const std::string& strPort)
{
	return std::string(strPort + strSendConn);
}

static std::string TcpSrvRecMsgTopic(const std::string& strPort)
{
	return std::string(strPort + strRevMsg);
}

static std::string TcpSrvSendMsgTopic(const std::string& strPort)
{
	return std::string(strPort + strSendMsg);
}

////rtsp topic
const std::string strGetRtspStream("GetRtspStream");

static std::string RtspStreamTopic(const std::string& strIp,const std::string& strCode)
{
	return std::string(strIp + strCode + strGetRtspStream);
}

//Db
////root node and sub node
const std::string strDbNode("Db");
const std::string strDbRedisNode("RedisCli");
const std::string strDbSqliteNode("SqliteSvr");
const std::string strDbMySqlNode("MySqlCli");

////attribute and value
const std::string strAttriRedisCliValue("rediscli");
const std::string strAttriMySqlCliValue("mysqlcli");
const std::string strAttriSqliteSvrValue("sqlitesvr");
const std::string strAttriConnTimeOut("conn_timeout");
const std::string strAttriRwTimeOut("rw_timeout");
const std::string strAttriInitConn("init_conn");
const std::string strAttriMaxConn("max_conn");

////db topic
const std::string strSaveDB("SaveToDB");
const std::string strReadDB("GetFromDB");
const std::string strDelDB("DelFromDB");
static std::string SaveRedisDBTopic(const std::string& strDBIdx)
{
	return std::string(strDBIdx + strSaveDB);
}

static std::string ReadRedisDBTopic(const std::string& strDBIdx)
{
	return std::string(strDBIdx + strReadDB);
}

static std::string DelRedisDBTopic(const std::string& strDBIdx)
{
	return std::string(strDBIdx + strDelDB);
}

static std::string SqliteSaveDBTopic(const std::string& strDBIdx)
{
	return std::string(strDBIdx + strSaveDB);
}

static std::string SqliteReadDBTopic(const std::string& strDBIdx)
{
	return std::string(strDBIdx + strReadDB);
}

static std::string SqliteDelDBTopic(const std::string& strDBIdx)
{
	return std::string(strDBIdx + strDelDB);
}

//Alg
////root node and sub node
const std::string strAlgNode("Alg");
const std::string strAlgDetectNode("Detect");
const std::string strAlgActionNode("Action");
const std::string strAlgFaceNode("Face");

////attribute and value
const std::string strAttriName("name");
const std::string strAttriVersion("version");
const std::string strAttriModel("model");

////alg topic
const std::string strCreatAlgObjCmd("AlgObjCreate");
static std::string AlgCreateTopic(const std::string& strType,const std::string& strIdx)
{
	return std::string(strType + strIdx + strCreatAlgObjCmd);
}

const std::string strAlgResultCmd("AlgResultReport");
static std::string AlgResultTopic(const std::string& strType,const std::string& strIdx)
{
	return std::string(strType + strIdx + strAlgResultCmd);
}

const std::string strFaceFeatureExtCmd("FaceFeatureExt");
static std::string FaceFeatureExtTopic(const std::string& strType,const std::string& strIdx)
{
	return std::string(strType + strIdx + strFaceFeatureExtCmd);
}

const std::string strFaceFeatureDBUpdateCmd("FaceFeatureUpdate");
static std::string FaceFeatureUpdateTopic(const std::string& strType,const std::string& strIdx)
{
	return std::string(strType + strIdx + strFaceFeatureDBUpdateCmd);
}

//Task
////root node and sub node
const std::string strTaskNode("Task");
const std::string strTaskDetectNode("Detect");
const std::string strTaskActionNode("Action");

////attribut and value
const std::string strAttriLst("lst");

//App
////root node and sub node
const std::string strAppNode("App");
const std::string strAppCommSafeNode("CommSafe");

////attribute and value
const std::string strAttriAlgIdx("alg_idx");
const std::string strAttriAlgIdxDetectValue("0");
const std::string strAttriAlgIdxActionValue("1");
const std::string strAttriAlgIdxFaceValue("2");

const std::string strAttriDataSrc("data_src");
const std::string strAttriTransferId("transfer_id");
const std::string strAttriTaskLst("task_lst");
const std::string strAttriStartTime("start_time");
const std::string strAttriEndTime("end_time");

const std::string strAttriFaceIDCard("ID_card");
const std::string strAttriFaceIDName("ID_name");
const std::string strAttriFaceIDFeature("ID_feature");
const std::string strAttriFaceIDImg("ID_img");
const std::string strAttriFaceImgData("data");
const std::string strAttriFaceImgIdx("idx");
const std::string strAttriWhiteName("white_name");

//db table
const std::string strDBAlgTable("Alg");
const std::string strDBAppTable("App");
const std::string strDbTable("Db");
const std::string strDBTaskTable("Task");
const std::string strDBTransferTable("Transfer");
const std::string strDBDetectReportTable("DetectReportTable");
const std::string strDBFaceRegisteTable("FaceRegisteTable");
const std::string strDBFaceReportTable("FaceReportTable");

//report result
const std::string strAttriTaskFlag("task_flag");
const std::string strAttriStartIdx("start_idx");
const std::string strAttriEndIdx("end_idx");
const std::string strAttriTotalSize("total_size");
const std::string strAttriAlertType("alert_type");
const std::string strAttriAlertDate("alert_date");
const std::string strAttriRecord("record");
const std::string strAttriImg("img");
const std::string strAttriBatchSize("batch_size");
const std::string strAttriFaceId("face_id");
const std::string strAttriFaceImg("face_img");
const std::string strAttriFaceInfo("face_info");