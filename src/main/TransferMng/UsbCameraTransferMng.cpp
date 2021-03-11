#include "UsbCameraTransferMng.h"
#include "SubTopic.h"

using namespace MAIN_MNG;
using namespace IF_USBCAMERA_SERVICE;
using namespace std;

CUsbCameraTransferMng::~CUsbCameraTransferMng()
{

}

void CUsbCameraTransferMng::StartCameras()
{
    //获取摄像头拉流信息
    vector<nlohmann::json> vecSoutUsbCameraCfg;

	string&& strKey = strTransferNode + strTransferSouthSerialNode + strAttriSerialValue;
	SCCfgMng.GetRuntimeSysCfg(strKey,vecSoutUsbCameraCfg);
    if(vecSoutUsbCameraCfg.empty())
    {
        return;
    }

    //usb camera module init
    GetUsbCameraModuleInstance()->InitModule(this);

    //start pull visdeo threads
    for (auto elm : vecSoutUsbCameraCfg)
	{
        string&& strIp = elm[strAttriIp];
        string&& strCamCode = elm[strAttriParams][strAttriCode];
        if(strCamCode.empty() || strIp.empty())
        {
            continue;
        }

        if(0 < m_mapPullStream.count(strCamCode))
        {
            continue;
        }
        
        m_mapPullStream[strCamCode] = ThreadShrPtr(new common_cmmobj::CThread(0, 10, "PullVisdeoLoop",-1,
                                                                        std::bind(&CUsbCameraTransferMng::PullVideLoop,this,placeholders::_1,placeholders::_2),
                                                                        strIp,
                                                                        strCamCode));
    }
}

bool CUsbCameraTransferMng::ReportCameraRunState(const std::string& CameraCode, const int state)
{
    return true;
}

void CUsbCameraTransferMng::PullVideLoop(const std::string& strIp,const std::string& strCamCode)
{
    IF_USBCAMERA_SERVICE::st_cvMat cvMat;
    GetUsbCameraModuleInstance()->GetImage(strCamCode,cvMat);
    if(0 >= cvMat.srcMat.cols || 0 >= cvMat.srcMat.rows)
    {
        return;
    }
}


