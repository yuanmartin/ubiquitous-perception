#pragma once
#include "IIpCameraServiceModule.h"
#include "comm/Singleton.h"

namespace rtsp_net
{
    class CVSModule : public 
    IF_VIDEO_SERVICE::IVideoStreamModule,
    common_template::CSingleton<CVSModule>
    {
        friend class common_template::CSingleton<CVSModule>;

    public:
        //模块初始化
        virtual bool InitModule(IF_VIDEO_SERVICE::IVideoStreamCallback* pCallbackObj);

        //模块启动
        virtual bool StartModule(const IF_VIDEO_SERVICE::MapCamBaseInfo& mapCamera);

        //从行为队列获取图片
        virtual void GetActImage(const std::string& strCameraCode, IF_VIDEO_SERVICE::st_cvMat& stcvMat);

        //从人脸队列获取图片
        virtual void GetCmmImage(const std::string& strCameraCode, IF_VIDEO_SERVICE::st_cvMat& stcvMat);

        //同步添加摄像机数据
        virtual void SyncAddCameraInfo(const std::vector<IF_VIDEO_SERVICE::TCameraBaseInfo>& vecCameraBaseInfo);

        //同步删除摄像机数据
        virtual void SyncDelCameraInfo(const std::vector<IF_VIDEO_SERVICE::TCameraBaseInfo>& vecCameraBaseInfo);

        //同步修改摄像机数据
        virtual void SyncModifyCameraInfo(const std::vector<IF_VIDEO_SERVICE::TCameraBaseInfo>& vecCameraBaseInfo);

        //上报摄像机运行状态
        bool ReportCameraRunState(const std::string& CameraCode, const int32_t state);

    protected:
        CVSModule() = default;
        virtual ~CVSModule() = default;

    private:
        IF_VIDEO_SERVICE::IVideoStreamCallback* m_pBDRCallBack = nullptr;
    };

    #define SVSModule (common_template::CSingleton<CVSModule>::GetInstance())
}