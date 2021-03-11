#include "VideoQueue.h"
#include <iostream>
#include "comm/CommFun.h"
#include "log4cxx/Loging.h"
#include "IIpCameraServiceModule.h"

using namespace std;
using namespace rtsp_net;
using namespace IF_VIDEO_SERVICE;

CVSQueue::CVSQueue() :
m_s32Height(0),
m_s32Width(0)
{
	for (int i = 0; i < gTaskQueueNum; ++i)
	{
		m_VideoStreamQue[i].set_capacity(gTaskQueueCapacity);
	}
}

CVSQueue::~CVSQueue()
{
	Release();
}

//释放图片资源
void CVSQueue::Release()
{	
	for (int i = 0; i < gTaskQueueNum; ++i)
	{
		boost::circular_buffer<st_cvMat>::iterator it = m_VideoStreamQue[i].begin();
		for (; it != m_VideoStreamQue[i].end(); ++it)
		{
			if (!(*it).srcMat.empty())
			{
				(*it).srcMat.release();
			}
		}
	}
}

void CVSQueue::Push(const BYTE* src)
{	
	st_cvMat sMatObj;
	sMatObj.srcMat.create(m_s32Height, m_s32Width, CV_8UC3);
	memcpy(sMatObj.srcMat.data, src, m_s32Width * m_s32Height * 3);
	std::string strCurTime = GetMillionTime();
	sMatObj.strShotTime = strCurTime;

	for (int i = 0; i < gTaskQueueNum; i++)
	{
		if (!m_VideoStreamQue[i].full())
		{
			m_VideoStreamQue[i].push_back(sMatObj);
		}
	}
}

void CVSQueue::Push(const cv::Mat& cvMat)
{
	st_cvMat sMatObj;
	sMatObj.strShotTime = GetMillionTime();
	sMatObj.srcMat = std::move(cvMat);

	for (int i = 0; i < gTaskQueueNum; i++)
	{
		if (!m_VideoStreamQue[i].full())
		{
			m_VideoStreamQue[i].push_back(sMatObj);
		}
	}
}