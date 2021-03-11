#include "DbMng.h"
#include "MsgBusMng/MsgBusMng.h"
#include "DeviceMng/DeviceMng.h"
#include "CfgMng/CfgMng.h"
#include "log4cxx/Loging.h"
#include "boost/BoostFun.h"
#include "SubTopic.h"
#include "comm/FuncInitLst.h"

using namespace std;
using namespace MAIN_MNG;
using namespace common_cmmobj;
using namespace common_template;

bool CDbMng::InitDb()
{
	return m_RdCliShrPtr->Init();
}

bool CDbMng::LocalDBTableInit()
{
	return m_SqliteSvrShrPtr->Init();
}

void CDbMng::LocalDBTableUpdate(bool bInit)
{
	m_SqliteSvrShrPtr->DBTableOpt(bInit);
}