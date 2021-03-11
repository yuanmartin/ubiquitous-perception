#pragma once
#include "IUsbCameraServiceModule.h"
#include "comm/Singleton.h"

using namespace IF_USBCAMERA_SERVICE;

namespace usb_camera
{
    class CUsbCameraModule : public IF_USBCAMERA_SERVICE::IUsbCameraStreamModule,common_template::CSingleton<CUsbCameraModule>
    {
        friend class common_template::CSingleton<CUsbCameraModule>;

    public:
        //模块初始化
        virtual bool InitModule(IF_USBCAMERA_SERVICE::IUsbCameraStreamCallback* pCallbackObj);
        virtual bool GetImage(const std::string& strCode,st_cvMat& cvMat);

    protected:
        CUsbCameraModule() = default;
        virtual ~CUsbCameraModule() = default;

    private:
        IF_USBCAMERA_SERVICE::IUsbCameraStreamCallback* m_pBDRCallBack = nullptr;
    };

    #define SCUsbCameraModule (common_template::CSingleton<CUsbCameraModule>::GetInstance())
}