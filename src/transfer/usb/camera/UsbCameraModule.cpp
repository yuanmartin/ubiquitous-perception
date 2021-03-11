#include "UsbCameraModule.h"

using namespace IF_USBCAMERA_SERVICE;
using namespace usb_camera;

extern "C" IUsbCameraStreamModule* GetUsbCameraModuleInstance()
{
    return nullptr;
}

bool CUsbCameraModule::InitModule(IF_USBCAMERA_SERVICE::IUsbCameraStreamCallback* pCallbackObj)
{
    if(!m_pBDRCallBack)
    {
        m_pBDRCallBack = pCallbackObj;
    }
    return true;
}

bool CUsbCameraModule::GetImage(const std::string& strCode,st_cvMat& cvMat)
{
    return true;
}