#pragma once
#include <string>
#include <vector>

namespace IF_HTTP_SERVICE
{
    using Func = std::function<void()>;
    extern "C" bool HttpInit();
    extern "C" bool HttpGet(const std::string& strAuth,const std::string& strHost,const int& s32Port,const std::string& strPag,std::string& strRspData);
    extern "C" bool HttpPost();
}