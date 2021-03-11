#include "RtspCliMng.h"
#include "VideoQueueMng.h"
#include <thread>
#include <chrono>
#include "comm/ScopeGuard.h"
#include "VideoQueueMng.h"
#include "log4cxx/Loging.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgcodecs/legacy/constants_c.h"

using namespace std;
using namespace rtsp_net;
using namespace common_template;
using namespace common_cmmobj;
using namespace IF_VIDEO_SERVICE;

const int s32SnapShotTime = 5; 
const int s32ReconTimeOut = 5;
enum{STREAM_MODEL,SNAPSHOT_MODEL};

CRtspCliMng::CRtspCliMng(const TCameraBaseInfo& camBaseInfo):
	m_TimeForReConn(time(NULL)),
	m_FmpegPtr(NULL),
	m_RtspClientPtr(NULL),
	m_pScheduler(NULL),
	m_pEnv(NULL),
	m_ptrSnapShotThread(NULL),
	m_bReconnFlag(false),
	m_bFirstPicFlag(true)
{
	//设置摄像头信息
	SetCameraBaseInfo(camBaseInfo, m_CamBaseInfo);

	//获取队列对象
	SVSQueueManager.GetVSQueueObj(m_CamBaseInfo.strCameraCode, m_VSQueWekPtr);
}

CRtspCliMng::~CRtspCliMng()
{
	ReleaseSource();
}

//设置拉流回调
void CRtspCliMng::SetPullStreamCB()
{
	if(STREAM_MODEL == m_CamBaseInfo.s32WorkMode)
	{
		//创建解码器
		m_FmpegPtr = CFFmpegShrPtr(new CFFmpeg());
		m_FmpegPtr->OpenH264Avcodec();

		//创建拉流客户端
		m_RtspClientPtr = CRtspCliShrPtr(new CRtspCli());

		//创建调度器
		m_pScheduler = BasicTaskScheduler::createNew();
		m_pEnv = BasicUsageEnvironment::createNew(*m_pScheduler);

		//打开摄像头
		auto cb = std::bind(&CRtspCliMng::CallBackFunc, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
		m_RtspClientPtr->Open(*m_pEnv, m_strRtspUrl.c_str(), m_CamBaseInfo,cb);

		//设置首帧
		SetFirstPicFlag(true);
	}
	else
	{
		//创建抓拍线程
		m_ptrSnapShotThread = ThreadShrPtr(new common_cmmobj::CThread(s32SnapShotTime, 0, "SnapShotLoop",-1,std::bind(&CRtspCliMng::SnapShotLoop,this)));
	}
}

//打开摄像头
void CRtspCliMng::OpenCamera()
{	
	//获取流和抓拍地址
	if(!GetRtspAndSnapUrl())
	{
		return;
	}

	//回调设置
	SetPullStreamCB();
}

//获取重连超时状态
bool CRtspCliMng::GetConnTimeOut()
{
	bool bRet = IsTimeOut(m_TimeForReConn, s32ReconTimeOut);
	bool bTimeOut =  m_bReconnFlag && bRet;
	if(bTimeOut)
	{
		m_bReconnFlag = true;
	}
	return bTimeOut;
}

//重置重连检测初始时间
void CRtspCliMng::SetConnTimeOut(bool bReconnFlag)
{
	m_TimeForReConn = time(NULL);
	m_bReconnFlag = bReconnFlag;
}

//获取首帧标识
bool CRtspCliMng::GetFirstPicFlag()
{
	return m_bFirstPicFlag;
}

//设置首帧标识
void CRtspCliMng::SetFirstPicFlag(bool bFlag)
{
	m_bFirstPicFlag = bFlag;
}

//拉流回调线程
void CRtspCliMng::CallBackFunc(SPropRecord* pRecord, int nRecords, u_int8_t* fReceiveBuffer, unsigned frameSize)
{
	//配置更新
	if(GetUpdate())
	{
		return;
	}

	//如果没有sps和pps，解码会失败, nRecords < 2: 表示没有PPS。 
	if (nRecords < 2)
	{
		return;
	}

	//判断是否I Frame
	if (!(fReceiveBuffer[0] == 0x65 || fReceiveBuffer[0] == 0x25))
	{
		return;
	}

	//解码和放队列
	VSQueShrPtr vsQueShrPtr = m_VSQueWekPtr.lock();
	if (vsQueShrPtr)
	{
		//图片解码
		uint8_t* pPicBuffer = NULL;
		m_FmpegPtr->DecodePicture(pRecord, fReceiveBuffer, frameSize, &pPicBuffer);
		if (pPicBuffer)
		{
			//设置首帧大小
			if(GetFirstPicFlag())
			{
				vsQueShrPtr->SetFirstPicSize(m_FmpegPtr->GetHeight(),m_FmpegPtr->GetWidth());
				SetFirstPicFlag();
			}

			//图片添加到队列
			vsQueShrPtr->Push(pPicBuffer);
		}
	}

	//重置重连检测初始时间
	SetConnTimeOut();
}

//设置摄像头信息更新状态
void CRtspCliMng::SetUpdate(const TCameraBaseInfoType& camBaseInfo,bool bUpdate)
{
	CLockType lock(&m_CamBaseInfoMutex);
	if(bUpdate)
	{
		SetCameraBaseInfo(camBaseInfo, m_CamBaseInfo);
	}
	m_CamBaseInfo.bUpdate = bUpdate;
}

void CRtspCliMng::SetUpdate(bool bUpdate)
{
	CLockType lock(&m_CamBaseInfoMutex);
	m_CamBaseInfo.bUpdate = bUpdate;
}

//获取摄像头信息更新状态
bool CRtspCliMng::GetUpdate()
{
	CLockType lock(&m_CamBaseInfoMutex);
	return m_CamBaseInfo.bUpdate;
}

//重启摄像头
void CRtspCliMng::RestartCam()
{	
	//先关闭后打开
	ReleaseSource();

	OpenCamera();

	SetUpdate(false);
}

//在线状态检测线程
void CRtspCliMng::PullStream()
{
	//更新重启
	if(GetUpdate())
	{
		RestartCam();
		return;
	}

	//断线重启
	if(m_pScheduler)
	{
		//检测调度器
		((BasicTaskScheduler0 *)m_pScheduler)->SingleStep();

		//检测是否重连
		if (GetConnTimeOut())
        {
			SetUpdate(true);
            RestartCam();
        }
	}
}

//获取抓拍图像
string CRtspCliMng::GetSnapShotPic()
{
	CLockType lock(&m_CamBaseInfoMutex);
	string strMatData;
	m_CamBaseInfo.m_cb(m_strSnapShotAuth,m_strSnapShotHost,m_strSnapShotPag,strMatData);
	return strMatData;
}

//抓拍模式循环线程
void CRtspCliMng::SnapShotLoop()
{
	//get抓拍图像
	string&& strMatData = GetSnapShotPic();
	cv::_InputArray pic_arr(strMatData.c_str(), strMatData.size());
	cv::Mat srcMatObj = cv::imdecode(pic_arr,CV_LOAD_IMAGE_COLOR);

	//图片添加到队列
	VSQueShrPtr vsQueShrPtr = m_VSQueWekPtr.lock();
	if (vsQueShrPtr)
	{
		vsQueShrPtr->Push(srcMatObj);
	}

	//重置上报和重连检测初始时间
	SetConnTimeOut();
}

//更新摄像头信息线程
void CRtspCliMng::UpdateCamBaseInfo(const TCameraBaseInfo& camBaseInfo)
{
	SetUpdate(camBaseInfo);
}