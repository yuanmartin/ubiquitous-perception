#pragma once
#include <string>
#include "mutex/Condition.h"
#include "mutex/LibMutex.h"
#include "thread/Thread.h"
#include "comm/NonCopyable.h"
#include "RtspCliMng.h"
#include "comm/CommFun.h"
#include "IIpCameraServiceModule.h"
namespace rtsp_net
{
    class CPullStream : common_template::NonCopyable
    {
        using CLockType = common_cmmobj::CLock;
        using CMutexType = common_cmmobj::CMutex;
        using CThreadType = common_cmmobj::CThread;

    public:
        CPullStream(const IF_VIDEO_SERVICE::TCameraBaseInfo& tCameraBaseInfo);
        ~CPullStream();

        //启动拉流
        bool Start();

        //拉流线程循环
        void PullStreamLoop();

        //更新摄像头信息
        void UpdateCameraBaseInfo(const IF_VIDEO_SERVICE::TCameraBaseInfo& tCameraBaseInfo);
    
    private:
        bool m_bRuning;

        std::shared_ptr<CRtspCliMng> m_RtspClientMngPtr;
        std::shared_ptr<CThreadType> m_PSThrUniPtr;
    };
} // namespace rtsp_net
