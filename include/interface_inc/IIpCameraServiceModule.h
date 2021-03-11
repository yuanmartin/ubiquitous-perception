#pragma once
#include <string>
#include <map>
//opencv的头文件
#include <opencv2/opencv.hpp>
//boost的头文件
#include <boost/mpl/int.hpp>
#include <mutex>
#include <functional>

namespace IF_VIDEO_SERVICE
{       
    #pragma pack(push, 1)  
    typedef struct TCameraBaseInfo
    {
        using Func = std::function<void(const std::string&,const std::string&, const std::string&, std::string&)>;
        TCameraBaseInfo(Func Cb) : m_cb(Cb){}
        TCameraBaseInfo(){}

        bool bUpdate = false;
        int64_t s64Id;
        int32_t s32Port;
        int32_t s32GetStreamMode;
        int32_t s32State;
        int32_t s32CoordinateX;
        int32_t s32CoordinateY;
        int32_t s32Width;
        int32_t s32Height;
        int32_t s32IsReport;
        int32_t s32OffOrOn = 1;
        int32_t s32WorkMode = 0;                    //0 拉流；1 抓拍
        std::string strCameraCode;
        std::string strCameraName;
        std::string strDiscernSysCode;
        std::string strDoorCode;
        std::string strIP;
        std::string strMac;
        std::string strUserName;
        std::string strPassword;
        std::string strProtocol;
        std::string strCameraPixel;
        std::string strTaskSet;
        std::string strImageGroupSet;
        std::string strInstallAddr;
        std::string strStartTime;
        std::string strEndTime;
        std::string strCreateTime;
        std::string strRegion;
        std::string strCameraLiveId;
        std::string strRemark;
        Func m_cb;                                //回调函数
    }TCameraBaseInfo;

    //视频流结构体
    typedef struct st_cvMat 
    {
        std::string strShotTime;
        cv::Mat srcMat;
    }cvtMat;

    //行为类型
    enum class eActType
    {
        NonSelfDef = 1,
        SelfDef = 2
    };
    #pragma pack(pop)

    /*摄像机拉流*/
    using VecCamBaseInfo = std::vector<IF_VIDEO_SERVICE::TCameraBaseInfo>;
    using MapCamBaseInfo = std::map<std::string, IF_VIDEO_SERVICE::TCameraBaseInfo>;
    class IVideoStreamCallback
    {
    public:
        //上报摄像机运行状态
        virtual bool ReportCameraRunState(const std::string& CameraCode, const int32_t state) = 0;
    };

    class IVideoStreamModule
    {
    public:
        virtual bool InitModule(IVideoStreamCallback* pCallbackObj) = 0;
        virtual bool StartModule(const MapCamBaseInfo& mapCamera) = 0;

        //从行为队列获取图片
        virtual void GetActImage(const std::string& strCameraCode,st_cvMat& stcvMat) = 0;

        //从人脸队列获取图片
        virtual void GetCmmImage(const std::string& strCameraCode, st_cvMat& stcvMat) = 0;

        //同步添加摄像机数据
        virtual void SyncAddCameraInfo(const std::vector<TCameraBaseInfo>& vecCameraBaseInfo) = 0;

        //同步删除摄像机数据
        virtual void SyncDelCameraInfo(const std::vector<TCameraBaseInfo>& vecCameraBaseInfo) = 0;

        //同步修改摄像机数据
        virtual void SyncModifyCameraInfo(const std::vector<TCameraBaseInfo>& vecCameraBaseInfo) = 0;
    };
    extern "C" IVideoStreamModule* GetVSModuleInstance();
}