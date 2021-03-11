#pragma once
#include <string>
#include "stdsoap2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ONVIF_ADDRESS_SIZE          (178)                                              // URI地址长度
#define ONVIF_ADDRESS_AUTH_SIZE     (228)                                              // 带认证的URI地址长度
#define ONVIF_TOKEN_SIZE     (65)

/* 设备能力信息 */
struct tagCapabilities 
{
    char MediaXAddr[ONVIF_ADDRESS_SIZE];                                        // 媒体服务地址
    char EventXAddr[ONVIF_ADDRESS_SIZE];                                        // 事件服务地址
                                                                                // 其他服务器地址就不列出了
};

/* 视频编码器配置信息 */
struct tagVideoEncoderConfiguration
{
    char token[ONVIF_TOKEN_SIZE];                                               // 唯一标识该视频编码器的令牌字符串
    int Width;                                                                  // 分辨率
    int Height;
};

/* 设备配置信息 */
struct tagProfile 
{
    char token[ONVIF_TOKEN_SIZE];                                               // 唯一标识设备配置文件的令牌字符串
    struct tagVideoEncoderConfiguration venc;                                   // 视频编码器配置信息
};

class COnvifIpc
{

public:
    COnvifIpc();
    ~COnvifIpc();

    std::string GetStreamUri(const std::string& strIp,const std::string& strUserName,const std::string& strPwd,bool bWithAuth = false);
    std::string GetSnapshotUri(const std::string& strIp,const std::string& strUserName,const std::string& strPwd,int& s32Height,int& s32Width,bool bWithAuth = true);

private:
    struct soap* ONVIF_soap_new(int timeout);

    void soap_perror(struct soap *soap, const char *str);

    void*  ONVIF_soap_malloc(struct soap *soap, unsigned int n);

    void ONVIF_init_header(struct soap *soap);

    void ONVIF_init_ProbeType(struct soap *soap, struct wsdd__ProbeType *probe);

    void ONVIF_soap_delete(struct soap *soap);

    int ONVIF_SetAuthInfo(struct soap *soap, const char *username, const char *password);

    int ONVIF_DetectDevice(char **DeviceXAddr,struct soap** soap);

    int ONVIF_GetCapabilities(const char *DeviceXAddr,const char*usename,const char*password, struct tagCapabilities *capa);
    
    int ONVIF_GetProfiles(const char *MediaXAddr, const char*usename,const char*password,struct tagProfile **profiles);

    int ONVIF_GetStreamUriWithAuth(char *src_uri, const char *username, const char *password, char *dest_uri, unsigned int size_dest_uri);

    int ONVIF_GetStreamUri(const char *MediaXAddr,const char*usename,const char*password);

    int ONVIF_GetSnapshotUri(const char *MediaXAddr,const char*usename,const char*password);

    std::string GetMediaAddr(const std::string& strIp,const std::string& strUserName,const std::string& strPwd);

    int GetSoapAndProfiles(const char *MediaXAddr,const char* username,const char* passwdord,struct soap*& soap,struct tagProfile*& profiles);

private:
	char m_streamUri[ONVIF_ADDRESS_SIZE] = {0};
    char m_snapshotUri[ONVIF_ADDRESS_SIZE] = {0};

    char m_streamUriAuth[ONVIF_ADDRESS_AUTH_SIZE] = {0};
    char m_snapshotUriAuth[ONVIF_ADDRESS_AUTH_SIZE] = {0};

    int m_s32SnapShotHeight = 0;
    int m_s32SnapShotWidth = 0;
};
#ifdef __cplusplus
}
#endif