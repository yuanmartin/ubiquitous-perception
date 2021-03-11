#pragma once
#include <string>
#include <map>
//opencv的头文件
#include <opencv2/opencv.hpp>
//boost的头文件
#include <boost/mpl/int.hpp>
#include <mutex>
#include <functional>

namespace IF_USBCAMERA_SERVICE
{       
    #pragma pack(push, 1)  
    //视频流结构体
    typedef struct st_cvMat 
    {
        std::string strShotTime;
        cv::Mat srcMat;
    }cvtMat;
    #pragma pack(pop)

    /*Usb摄像机拉流*/
    class IUsbCameraStreamCallback
    {
    public:
        //上报摄像机运行状态
        virtual bool ReportCameraRunState(const std::string& CameraCode, const int32_t state) = 0;
    };

    class IUsbCameraStreamModule
    {
    public:
        virtual bool InitModule(IUsbCameraStreamCallback* pCallbackObj) = 0;
        virtual bool GetImage(const std::string& strCode,st_cvMat& cvMat) = 0;
    };
    extern "C" IUsbCameraStreamModule* GetUsbCameraModuleInstance();
}