#include <string>
#include <vector>
#include <map>
#include "IUsbCameraServiceModule.h"
#include "thread/Thread.h"
#include "comm/Singleton.h"
#include "CfgMng/CfgMng.h"

namespace MAIN_MNG
{
    class CUsbCameraTransferMng : public IF_USBCAMERA_SERVICE::IUsbCameraStreamCallback,
    public common_template::CSingleton<CUsbCameraTransferMng>
    {
        friend class common_template::CSingleton<CUsbCameraTransferMng>;
        using ThreadShrPtr = std::shared_ptr<common_cmmobj::CThread>;
        using PullStreamMap = std::map<std::string,ThreadShrPtr>;

    public:
        //启动摄像
        void StartCameras();

        //摄像机运行状态上报
        virtual bool ReportCameraRunState(const std::string& CameraCode, const int state);

    private:
        CUsbCameraTransferMng() = default;
        ~CUsbCameraTransferMng();

        void PullVideLoop(const std::string& strIp,const std::string& strCamCode);

    private:
    	ThreadShrPtr m_ptrPullVideoThread;
        PullStreamMap m_mapPullStream;
    };
    #define SCUsbCameraTransferMng (common_template::CSingleton<CUsbCameraTransferMng>::GetInstance())
}