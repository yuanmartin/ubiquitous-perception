#pragma once
#include <string>
#include "comm/CommFun.h"
#include "comm/NonCopyable.h"
#include "videostream/BasicUsageEnvironment.hh"
#include "videostream/H264VideoRTPSource.hh"
#include "IIpCameraServiceModule.h"
#include "time/TimeWatcher.h"
#include "ffmpeg/FFmpeg.h"
#include "time/TimeWatcher.h"
#include "RtspCli.h"
#include "VideoQueue.h"
#include "VideoModule.h"
#include "thread/Thread.h"
#include "onvif/OnvifIpc.h"
#include "VideoCommFunc.h"
#include "comm/ReleaseThread.h"
#include "mutex/LibMutex.h"
#include <atomic> 
namespace rtsp_net
{
    class CRtspCliMng : common_template::NonCopyable
    {
        using TCameraBaseInfoType = IF_VIDEO_SERVICE::TCameraBaseInfo;
        using VSQueShrPtr = std::shared_ptr<CVSQueue>;
        using VSQueWekPtr = std::weak_ptr<CVSQueue>;
        using CRtspCliShrPtr = std::shared_ptr<CRtspCli>;
        using CFFmpegShrPtr = std::shared_ptr<common_cmmobj::CFFmpeg>;
        using ThreadShrPtr = std::shared_ptr<common_cmmobj::CThread>;
        using TimeSecondType = common_template::TimerSeconds;
        using CMutexType = common_cmmobj::CMutex;
        using CLockType = common_cmmobj::CLock;
        
    public:
        CRtspCliMng(const TCameraBaseInfoType& camBaseInfo);
        ~CRtspCliMng();

        //打开摄像头
        void OpenCamera();

        //拉流遍历接口
        void PullStream();

        //更新摄像头信息
        void UpdateCamBaseInfo(const TCameraBaseInfoType& camBaseInfo);

    private:
        //设置拉流回调
        void SetPullStreamCB();
        
        //拉流回调函数
        void CallBackFunc(SPropRecord* pRecord, int nRecords, u_int8_t* fReceiveBuffer, unsigned frameSize);

        //获取抓拍图像
        std::string GetSnapShotPic();

        //抓拍循环
        void SnapShotLoop();

        //设置摄像头信息更新状态
        void SetUpdate(const TCameraBaseInfoType& camBaseInfo,bool bUpdate = true);
        void SetUpdate(bool bUpdate = true);

        //获取摄像头信息更新状态
        bool GetUpdate();

        //重置重连检测初始时间
        void SetConnTimeOut(bool bReconnFlag = false);

        //获取重连超时状态
        bool GetConnTimeOut();

        //获取首帧标识
        bool GetFirstPicFlag();

        //设置首帧标识
        void SetFirstPicFlag(bool bFlag = false);

        //重启摄像头
        void RestartCam();

        //关闭摄像头
        inline void CloseCamera()
        {
            if(m_RtspClientPtr)
            {
                m_RtspClientPtr.reset();
            }

            if(m_ptrSnapShotThread)
            {
                common_template::ReleaseThread(m_ptrSnapShotThread);
            }
        }

        //关闭调度器
        inline void CloseScheduler()
        {
            if(m_pEnv)
            {
                m_pEnv->reclaim();
                m_pEnv = NULL;
            }

            if(m_pScheduler)
            {
                delete m_pScheduler;
                m_pScheduler = NULL;
            }
        }

        //关闭解码器
        inline void CloseFmpeg()
        {
            if(m_FmpegPtr)
            {
                m_FmpegPtr.reset();
            }
        }

        //是否资源
        inline void ReleaseSource()
        {
            CloseScheduler();

            CloseCamera();

            CloseFmpeg();
        }

        //获取摄像头RTSP和抓拍地址
        inline bool GetRtspAndSnapUrl()
        {
            string& strUserName = m_CamBaseInfo.strUserName;
            string& strPwd = m_CamBaseInfo.strPassword;
            string& strIp = m_CamBaseInfo.strIP;

            COnvifIpc onvifIpc;

            //拉流地址
            m_strRtspUrl = onvifIpc.GetStreamUri(strIp,strUserName,strPwd);

            //抓拍地址
            int s32SnapHeight = 0;
            int s32SnapWidth = 0;    
            m_strSnapshotUrl = onvifIpc.GetSnapshotUri(strIp,strUserName,strPwd,s32SnapHeight,s32SnapWidth);
            if(m_strRtspUrl.empty() || m_strSnapshotUrl.empty())
            {
                return false;
            }
            int&& idx = m_strSnapshotUrl.find(strIp) + strIp.size();
            m_strSnapShotPag = m_strSnapshotUrl.substr(idx);
            m_strSnapShotAuth = strUserName + string(":") + strPwd;
            m_strSnapShotHost = strIp;
            return true;
        }

    private:
        //摄像头基本信息
        CMutexType m_CamBaseInfoMutex;
        TCameraBaseInfoType m_CamBaseInfo;

        //重启竞态量
        atomic_long m_TimeForReConn;
        atomic_bool m_bReconnFlag;
        atomic_bool m_bFirstPicFlag;

        //拉流或抓拍公用的队列
        VSQueWekPtr m_VSQueWekPtr;

        //在线检测
        TaskScheduler* m_pScheduler;
        UsageEnvironment* m_pEnv;

        //拉流相关
        std::string m_strRtspUrl;
        CFFmpegShrPtr m_FmpegPtr;
        CRtspCliShrPtr m_RtspClientPtr;
        
        //抓拍相关
        std::string m_strSnapshotUrl;
        std::string m_strSnapShotAuth;
        std::string m_strSnapShotHost;
        std::string m_strSnapShotPag;
        ThreadShrPtr m_ptrSnapShotThread;
    };
}