#include "OnvifIpc.h"
#include "wsdd.h"
#include "soapH.h"
#include "soapStub.h"
#include "wsseapi.h"
#include "wsaapi.h"
#include "comm/CommFun.h"
#include <iostream>

#define SOAP_TO         "urn:schemas-xmlsoap-org:ws:2005:04:discovery"
#define SOAP_ACTION     "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe"

#define SOAP_MCAST_ADDR "soap.udp://239.255.255.250:3702"                        // onvif规定的组播地址

#define SOAP_ITEM       ""                                                       // 寻找的设备范围
#define SOAP_TYPES      "dn:NetworkVideoTransmitter"                             // 寻找的设备类型

#define SOAP_SOCK_TIMEOUT  (10)                                                  // socket超时时间（单秒秒）
#define MAX_DETECT_IPC     (200)

using namespace std;

COnvifIpc::COnvifIpc()
{
}

COnvifIpc::~COnvifIpc()
{
}

/************************************************************************
**函数：ONVIF_soap_new
**功能：创建soap环境变量
**参数：
        [in] timeout     - 时间设置
**返回：
        soap环境变量,非空表明成功，NULL表明失败
**备注：
************************************************************************/
struct soap* COnvifIpc::ONVIF_soap_new(int timeout)
{
    struct soap*soap=NULL;
    if((soap=soap_new())==NULL)
    {
        return NULL;
    }

    if(soap_set_namespaces(soap,namespaces)!=SOAP_OK)
    {
        return NULL;
    }
    soap->recv_timeout = timeout;
    soap->send_timeout = timeout;
    soap->connect_timeout = timeout;

#if defined(__linux__)||defined(__linux)
    soap->socket_flags =MSG_NOSIGNAL;
#endif

    soap_set_mode(soap,SOAP_C_UTFSTRING);
    return soap;
}

/************************************************************************
**函数：ONVIF_soap_malloc
**功能：soap环境变量分配空间
**参数：
        [in] soap      - soap环境变量
        [in] n         - 空间大小
**返回：
        非NULL表明成功，NULL表明失败
**备注：
************************************************************************/
void* COnvifIpc::ONVIF_soap_malloc(struct soap *soap, unsigned int n)
{
    if(0 >= n)
    {
        return nullptr;
    }

    void* handle = nullptr;
    handle = soap_malloc(soap, n);
    if(handle)
    {
        memset(handle, 0x00 ,n);
    }
    return handle;
}

/************************************************************************
**函数：ONVIF_init_header
**功能：初始化soap描述消息头
**参数：
        [in] soap - soap环境变量
**返回：无
**备注：
    1). 在本函数内部通过ONVIF_soap_malloc分配的内存，将在ONVIF_soap_delete中被释放
************************************************************************/
void COnvifIpc::ONVIF_init_header(struct soap *soap)
{
    struct SOAP_ENV__Header *header = NULL;
    header =(struct SOAP_ENV__Header *)ONVIF_soap_malloc(soap, sizeof(struct SOAP_ENV__Header));

    soap_default_SOAP_ENV__Header(soap, header);
    header->wsa__MessageID = (char*)soap_wsa_rand_uuid(soap);
    header->wsa__To        = (char*)ONVIF_soap_malloc(soap, strlen(SOAP_TO) + 1);
    header->wsa__Action    = (char*)ONVIF_soap_malloc(soap, strlen(SOAP_ACTION) + 1);
    strcpy(header->wsa__To, SOAP_TO);
    strcpy(header->wsa__Action, SOAP_ACTION);
    soap->header = header;
}

void COnvifIpc::soap_perror(struct soap *soap, const char *str)
{
    if (NULL == str)
    {
        std::cout <<"[soap] error: " << soap->error <<" " << *soap_faultcode(soap) << " "<< *soap_faultstring(soap) << std::endl;
    } 
    else
    {
        std::string faultcodeString(*soap_faultcode(soap));
        std::string faultstringString(*soap_faultstring(soap));
        std::cout << "[soap] " << str << "error: " << soap->error << " " << *soap_faultcode(soap) << " " << *soap_faultstring(soap) << std::endl;
    }
}

void COnvifIpc::ONVIF_soap_delete(struct soap *soap)
{
    soap_destroy(soap);                                                         // remove deserialized class instances (C++ only)
    soap_end(soap);                                                             // Clean up deserialized data (except class instances) and temporary data
    soap_done(soap);                                                            // Reset, close communications, and remove callbacks
    soap_free(soap);                                                            // Reset and deallocate the context created with soap_new or soap_copy
}

/************************************************************************
**函数：ONVIF_init_ProbeType
**功能：初始化探测设备的范围和类型
**参数：
        [in]  soap  - soap环境变量
        [out] probe - 填充要探测的设备范围和类型
**返回：

**备注：
    1). 在本函数内部通过ONVIF_soap_malloc分配的内存，将在ONVIF_soap_delete中被释放
************************************************************************/
void COnvifIpc::ONVIF_init_ProbeType(struct soap *soap, struct wsdd__ProbeType *probe)
{
    struct wsdd__ScopesType *scope = NULL;                                      // 用于描述查找哪类的Web服务
    scope = (struct wsdd__ScopesType *)ONVIF_soap_malloc(soap, sizeof(struct wsdd__ScopesType));
    soap_default_wsdd__ScopesType(soap, scope);                                 // 设置寻找设备的范围
    scope->__item = (char*)ONVIF_soap_malloc(soap, strlen(SOAP_ITEM) + 1);
    strcpy(scope->__item, SOAP_ITEM);

    memset(probe, 0x00, sizeof(wsdd__ProbeType));
    soap_default_wsdd__ProbeType(soap, probe);
    probe->Scopes = scope;
    probe->Types  = (char*)ONVIF_soap_malloc(soap, strlen(SOAP_TYPES) + 1);     // 设置寻找设备的类型
    strcpy(probe->Types, SOAP_TYPES);
}

int COnvifIpc::ONVIF_DetectDevice(char**DeviceXAddr,struct soap** soap)
{                 
    *soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT);                                      // soap环境变量
    if(!(*soap))
    {
        return 0;
    }
    ONVIF_init_header(*soap);                                                       // 设置消息头描述

    struct wsdd__ProbeType      req;                                                // 用于发送Probe消息                                             
    ONVIF_init_ProbeType(*soap, &req);                                              // 设置寻找的设备的范围和类型
    int result = soap_send___wsdd__Probe(*soap, SOAP_MCAST_ADDR, NULL, &req);       // 向组播地址广播Probe消息
    unsigned int count = 0;                                                         // 搜索到的设备个数                                                 
    while (SOAP_OK == result)                                                       // 开始循环接收设备发送过来的消息
    {
        struct __wsdd__ProbeMatches rep;                                            // 用于接收Probe应答
        memset(&rep, 0x00, sizeof(rep));
        result = soap_recv___wsdd__ProbeMatches(*soap, &rep);
        if (SOAP_OK == result)
        {
            if ((*soap)->error)
            {
                soap_perror(*soap, "ProbeMatches");
            }
            else
            {                                                                       // 成功接收到设备的应答消息
                if (NULL != rep.wsdd__ProbeMatches)
                {
                    count += rep.wsdd__ProbeMatches->__sizeProbeMatch;
                    struct wsdd__ProbeMatchType *probeMatch = NULL;
                    for(int i = 0; i < rep.wsdd__ProbeMatches->__sizeProbeMatch; i++)
                    {
                        probeMatch = rep.wsdd__ProbeMatches->ProbeMatch + i;
                        DeviceXAddr[count - 1] = probeMatch->XAddrs;
                    }
                }
            }
        }
        else if ((*soap)->error)
        {
            break;
        }
    }

    return count;
}

/************************************************************************
**函数：ONVIF_SetAuthInfo
**功能：设置认证信息
**参数：
        [in] soap     - soap环境变量
        [in] username - 用户名
        [in] password - 密码
**返回：
        0表明成功，非0表明失败
**备注：
************************************************************************/
int COnvifIpc::ONVIF_SetAuthInfo(struct soap *soap, const char *username, const char *password)
{
    return soap_wsse_add_UsernameTokenDigest(soap, NULL, username, password);
}

/************************************************************************
**函数：ONVIF_GetCapabilities
**功能：获取设备能力信息
**参数：
        [in] DeviceXAddr - 设备服务地址
        [out] capa       - 返回设备能力信息信息
**返回：
        0表明成功，非0表明失败
**备注：
    1). 其中最主要的参数之一是媒体服务地址
************************************************************************/
int COnvifIpc::ONVIF_GetCapabilities(const char *DeviceXAddr, const char*usename,const char* password,struct tagCapabilities *capa)
{
    if(!DeviceXAddr || !capa || !(usename && password))
    {
        return -1;
    }

    struct soap *soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT);
    if(!soap)
    {
       return -1;
    }

    int result = ONVIF_SetAuthInfo(soap, usename, password);
    if(0 != result)
    {
        ONVIF_soap_delete(soap);
        return -1;
    }

    struct _tds__GetCapabilities            req;
    struct _tds__GetCapabilitiesResponse    rep;
    result = soap_call___tds__GetCapabilities(soap, DeviceXAddr,NULL, &req, rep);
    if(result != SOAP_OK)
    {
        ONVIF_soap_delete(soap);
        return -1;
    }

    memset(capa, 0x00, sizeof(struct tagCapabilities));
    if (NULL == rep.Capabilities)
    {
        ONVIF_soap_delete(soap);
        return -1;
    }
    
    if (rep.Capabilities->Media)
    {
        if (rep.Capabilities->Media->XAddr)
        {
            strncpy(capa->MediaXAddr, rep.Capabilities->Media->XAddr, sizeof(capa->MediaXAddr) - 1);
        }
    }

    if (rep.Capabilities->Events)
    {
        if (rep.Capabilities->Events->XAddr)
        {
            strncpy(capa->EventXAddr, rep.Capabilities->Events->XAddr, sizeof(capa->EventXAddr) - 1);
        }
    }
    
    ONVIF_soap_delete(soap);
    return result;
}

/************************************************************************
**函数：ONVIF_GetProfiles
**功能：获取设备的音视频码流配置信息
**参数：
        [in] MediaXAddr - 媒体服务地址
        [out] profiles  - 返回的设备音视频码流配置信息列表，调用者有责任使用free释放该缓存
**返回：
        返回设备可支持的码流数量（通常是主/辅码流），即使profiles列表个数
**备注：
        1). 注意：一个码流（如主码流）可以包含视频和音频数据，也可以仅仅包含视频数据。
************************************************************************/
int COnvifIpc::ONVIF_GetProfiles(const char *MediaXAddr, const char*usename,const char*password,struct tagProfile **profiles)
{
    if(!usename || !password)
    {
        return -1;
    }

    struct soap *soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT);
    if(!soap)
    {
        return -1;
    }

    int result = ONVIF_SetAuthInfo(soap, usename, password);
    if(0 != result)
    {
        ONVIF_soap_delete(soap);
       return -1;
    }
    
    struct _trt__GetProfiles            req;
    struct _trt__GetProfilesResponse    rep;
    result = soap_call___trt__GetProfiles(soap, MediaXAddr, NULL, &req, rep);
    if(SOAP_OK != result)
    {
       ONVIF_soap_delete(soap);
       return -1;
    }

    if (0 < rep.__sizeProfiles)
    {    
        // 分配缓存
        (*profiles) = (struct tagProfile *)malloc(rep.__sizeProfiles * sizeof(struct tagProfile));
        if(!(*profiles))
        {
            return -1;
        }
        memset((*profiles), 0x00, rep.__sizeProfiles * sizeof(struct tagProfile));
    }

    for(int i = 0; i < rep.__sizeProfiles; i++)
    {   
        // 提取所有配置文件信息（我们所关心的）
        struct tt__Profile *ttProfile = rep.Profiles[i];
        struct tagProfile *plst = &(*profiles)[i];

        if (NULL != ttProfile->token)
        {   
            // 配置文件Token
            strncpy(plst->token, ttProfile->token, sizeof(plst->token) - 1);
        }

        if (NULL != ttProfile->VideoEncoderConfiguration)
        {   
            // 视频编码器配置信息
            if (NULL != ttProfile->VideoEncoderConfiguration->token)
            {          
                // 视频编码器Token
                strncpy(plst->venc.token, ttProfile->VideoEncoderConfiguration->token, sizeof(plst->venc.token) - 1);
            }
            if (NULL != ttProfile->VideoEncoderConfiguration->Resolution)
            {     
                // 视频编码器分辨率
                plst->venc.Width  = ttProfile->VideoEncoderConfiguration->Resolution->Width;
                plst->venc.Height = ttProfile->VideoEncoderConfiguration->Resolution->Height;
            }
        }
    }

    if (NULL != soap)
    {
        ONVIF_soap_delete(soap);
    }

    return rep.__sizeProfiles;
}

/************************************************************************
**函数：make_uri_withauth
**功能：构造带有认证信息的URI地址
**参数：
        [in]  src_uri       - 未带认证信息的URI地址
        [in]  username      - 用户名
        [in]  password      - 密码
        [out] dest_uri      - 返回的带认证信息的URI地址
        [in]  size_dest_uri - dest_uri缓存大小
**返回：
        0成功，非0失败
**备注：
    1). 例子：
    无认证信息的uri：rtsp://100.100.100.140:554/av0_0
    带认证信息的uri：rtsp://username:password@100.100.100.140:554/av0_0
************************************************************************/
int COnvifIpc::ONVIF_GetStreamUriWithAuth(char *src_uri, const char *username, const char *password, char *dest_uri, unsigned int size_dest_uri)
{
    int result = 0;
    unsigned int needBufSize = 0;

    memset(dest_uri, 0x00, size_dest_uri);
    needBufSize = strlen(src_uri) + strlen(username) + strlen(password) + 3;    // 检查缓存是否足够，包括‘:’和‘@’和字符串结束符
    if (size_dest_uri < needBufSize)
    {
        result = -1;
    }

    if (0 == strlen(username) && 0 == strlen(password))
    {                       
        // 生成新的uri地址
        strcpy(dest_uri, src_uri);
    } 
    else
    {
        char *p = strstr(src_uri, "//");
        if (NULL == p)
        {
            result = -1;
        }
        p += 2;

        memcpy(dest_uri, src_uri, p - src_uri);
        sprintf(dest_uri + strlen(dest_uri), "%s:%s@", username, password);
        strcat(dest_uri, p);
    }
    return result;
}

/************************************************************************
**函数：ONVIF_GetStreamUri
**功能：获取设备码流地址(RTSP)
**参数：
        [in]  MediaXAddr    - 媒体服务地址
        [in]  ProfileToken  - the media profile token
        [out] uri           - 返回的地址
        [in]  sizeuri       - 地址缓存大小
**返回：
        0表明成功，非0表明失败
**备注：
************************************************************************/
int COnvifIpc::ONVIF_GetStreamUri(const char *MediaXAddr,const char* usename,const char* password)
{
    //获取配置信息和soap
	struct tagProfile* profiles = NULL;
    struct soap* pstSoap = NULL;
    int result = GetSoapAndProfiles(MediaXAddr,usename,password,pstSoap,profiles);
    if(0 > result)
    {
        return -1;
    }

    //设置人证信息
    result = ONVIF_SetAuthInfo(pstSoap, usename, password);
    if(0 == result)
    {
        struct tt__StreamSetup              ttStreamSetup;
        struct tt__Transport                ttTransport;
        ttStreamSetup.Stream                = tt__StreamType__RTP_Unicast;
        ttStreamSetup.Transport             = &ttTransport;
        ttStreamSetup.Transport->Protocol   = tt__TransportProtocol__RTSP;
        ttStreamSetup.Transport->Tunnel     = NULL;

        struct _trt__GetStreamUri           req;
        struct _trt__GetStreamUriResponse   rep;
        req.StreamSetup                     = &ttStreamSetup;
        req.ProfileToken                    = profiles[0].token;;

        result = soap_call___trt__GetStreamUri(pstSoap, MediaXAddr, NULL, &req, rep);
        if(result == SOAP_OK)
        {
            memset(m_streamUri, 0x00, ONVIF_ADDRESS_SIZE); 
            strcpy(m_streamUri, rep.MediaUri->Uri);
        }
    }

    //资源释放
	free(profiles);
    ONVIF_soap_delete(pstSoap);

    return result;
}

/************************************************************************
**函数：ONVIF_GetSnapshotUri
**功能：获取设备图像抓拍地址(HTTP)
**参数：
        [in]  MediaXAddr    - 媒体服务地址
        [in]  ProfileToken  - the media profile token
**返回：
        0表明成功，非0表明失败
**备注：
    1). 并非所有的ProfileToken都支持图像抓拍地址。举例：XXX品牌的IPC有如下三个配置profile0/profile1/TestMediaProfile，其中TestMediaProfile返回的图像抓拍地址就是空指针。
************************************************************************/
int COnvifIpc::ONVIF_GetSnapshotUri(const char *MediaXAddr,const char* usename,const char* password)
{
	struct tagProfile* profiles = NULL;
    struct soap* pstSoap = NULL;

    int result = GetSoapAndProfiles(MediaXAddr,usename,password,pstSoap,profiles);
    if(0 > result)
    {
        return -1;
    }

    result = ONVIF_SetAuthInfo(pstSoap, usename, password);
    if(0 == result)
    {
        //获取抓拍地址
        struct _trt__GetSnapshotUriResponse rep;
        struct _trt__GetSnapshotUri         req;
        req.ProfileToken = profiles[0].token;
        result = soap_call___trt__GetSnapshotUri(pstSoap, MediaXAddr,NULL,&req,rep);
        if (SOAP_OK == result) 
        {
            memset(m_snapshotUri, 0x00, ONVIF_ADDRESS_SIZE);
            strcpy(m_snapshotUri, rep.MediaUri->Uri);
            m_s32SnapShotHeight = profiles[0].venc.Height;
            m_s32SnapShotWidth = profiles[0].venc.Width;
        } 
    }

    //资源释放
	free(profiles);
    ONVIF_soap_delete(pstSoap);

    return result;
}

//获取媒体地址
string COnvifIpc::GetMediaAddr(const std::string& strIp,const std::string& strUserName,const std::string& strPwd)
{
   //摄像头探测(组播)
    char** DeviceXAddr = new char* [MAX_DETECT_IPC]{0x00};
    struct soap * soap = NULL;
	int&& s32DeviceCount = ONVIF_DetectDevice(DeviceXAddr,&soap);

    //过滤ip地址
    string strMediaAddr("");
	for(int i = 0;i < s32DeviceCount;i++)
	{
		struct tagCapabilities capa;                                   
		ONVIF_GetCapabilities(DeviceXAddr[i],strUserName.c_str(),strPwd.c_str(),&capa);
		
		string strTemp(capa.MediaXAddr,ONVIF_ADDRESS_SIZE);
		if(string::npos != strTemp.find(strIp))
		{
			strMediaAddr = strTemp;
		}
	}

    for(int i = 0;i < MAX_DETECT_IPC;i++)
    {
        SafeDelete(DeviceXAddr[i]);
    }
    SafeDeleteArray(DeviceXAddr);

    if(NULL != soap)
    {
        soap_destroy(soap);                                                         // remove deserialized class instances (C++ only)
        soap_done(soap);
        soap_free(soap);
    }
    return strMediaAddr;
}

//获取配置信息和soap
auto ProfileReleaseFun = [](struct tagProfile*& pstProfiles)
{
    if(pstProfiles)
    {
        free(pstProfiles);
    }
    
    pstProfiles = NULL;
};

int COnvifIpc::GetSoapAndProfiles(const char *MediaXAddr,const char* username,const char* passwdord,struct soap*& pstSoap,struct tagProfile*& pstProfiles)
{
    int s32ProfilesCnt = ONVIF_GetProfiles(MediaXAddr,username,passwdord,&pstProfiles);
	if(0 >= s32ProfilesCnt)
	{
		return -1;
	}

    pstSoap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT);
    if(!pstSoap)
    {
        ProfileReleaseFun(pstProfiles);
        return -1;
    }
    return 0;
}

string COnvifIpc::GetStreamUri(const std::string& strIp,const std::string& strUserName,const std::string& strPwd,bool bWithAuth)
{
    //获取媒体地址
    string&& strMediaAddr = GetMediaAddr(strIp,strUserName,strPwd);
    if(strMediaAddr.empty())
    {
        return strMediaAddr;
    }

    //获取流地址
	ONVIF_GetStreamUri(strMediaAddr.c_str(),strUserName.c_str(),strPwd.c_str());
	
    //是否带认证返回
     stringstream strStream;
    if(bWithAuth)
    {
        ONVIF_GetStreamUriWithAuth(m_streamUri,strUserName.c_str(),strPwd.c_str(),m_streamUriAuth,ONVIF_ADDRESS_AUTH_SIZE);
        strStream << m_streamUriAuth;
    }
    else
    {
        strStream << m_streamUri;
    }
    
    return strStream.str();
}

string COnvifIpc::GetSnapshotUri(const std::string& strIp,const std::string& strUserName,const std::string& strPwd,int& s32Height,int& s32Width,bool bWithAuth)
{
    //获取媒体地址
    string&& strMediaAddr  = GetMediaAddr(strIp,strUserName,strPwd);
    if(strMediaAddr.empty())
    {
        return strMediaAddr;
    }

    //获取抓拍地址
    ONVIF_GetSnapshotUri(strMediaAddr.c_str(),strUserName.c_str(),strPwd.c_str());

    //是否带认证返回
     stringstream strStream;
    if(bWithAuth)
    {
        ONVIF_GetStreamUriWithAuth(m_snapshotUri,strUserName.c_str(),strPwd.c_str(),m_snapshotUriAuth,ONVIF_ADDRESS_AUTH_SIZE);
        strStream << m_snapshotUriAuth;
    }
    else
    {
        strStream << m_snapshotUri;
    }

    //返回抓拍图片长宽
    s32Height = m_s32SnapShotHeight;
    s32Width = m_s32SnapShotWidth;
    
    return strStream.str();
}

