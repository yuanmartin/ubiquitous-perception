#pragma once
#include <stdio.h>
#include <stddef.h>
#include <vector>
#include "boost/circular_buffer.hpp"
#include "comm/NonCopyable.h"
#include "IIpCameraServiceModule.h"

namespace rtsp_net
{
    const int gTaskQueueNum = 3;      //任务队列数
    const int gTaskQueueCapacity = 2; //任务队列容量

    typedef long unsigned int size_t;
    typedef unsigned char BYTE;

    typedef enum
    {
        FaceQueue = 0,
        ActQueue = 1,
        SelfDefActQueue = 2
    }eQueueType;

    class CVSQueue: common_template::NonCopyable
    {
    public:
        CVSQueue();
        ~CVSQueue();

        //添加队列数据
        void Push(const cv::Mat& srcMat);
        void Push(const BYTE* src);

        //释放图片资源
        void Release();

        void Pop(eQueueType QueueType, IF_VIDEO_SERVICE::st_cvMat& srcMat)
        {
            if (m_VideoStreamQue[QueueType].empty())
            {
                return;
            }

            srcMat = m_VideoStreamQue[QueueType].front();
            m_VideoStreamQue[QueueType].pop_front();
        }

        //获取行为识别图片
        void GetActImage(IF_VIDEO_SERVICE::st_cvMat& srcMat)
        {
            Pop(ActQueue, srcMat);
        }

        //获取行为识别图片
        void GetSelfDefActImage(IF_VIDEO_SERVICE::st_cvMat& srcMat)
        {
            Pop(SelfDefActQueue, srcMat);
        }			

        //获取人脸识别图片
        void GetCmmImage(IF_VIDEO_SERVICE::st_cvMat& srcMat)
        {
            Pop(FaceQueue, srcMat);
        }

        int GetFaceQueSize()
        {
            return m_VideoStreamQue[FaceQueue].size();
        }

        int GetActQueSize()
        {
            return m_VideoStreamQue[ActQueue].size();
        }

        int SetFirstPicSize(int s32Height,int s32Width)
        {
            m_s32Height = s32Height;
            m_s32Width = s32Width;
        }

    private:
        int m_s32Height;
        int m_s32Width;
        boost::circular_buffer<IF_VIDEO_SERVICE::st_cvMat> m_VideoStreamQue[gTaskQueueNum];
    };
}