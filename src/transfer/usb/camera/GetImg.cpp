#include "GetImg.h"
#include "opencv2/video.hpp"
#include "opencv2/imgproc/types_c.h"

using namespace usb_camera;
using namespace std;

GetImage::GetImage()
{
    m_Mode = GET_IMG_MODE_CAM;
    m_WorkMode = GET_IMG_WORK_MODE_ORC;
    memset(&m_VideoPath[0], 0, sizeof(m_VideoPath[0])*FILENAMELEN);
    m_Width = 1280;
    m_Height = 720;
    m_Capture.open(0);
    m_ScaleImg.create(m_Width, m_Height, CV_8UC3);
    m_ScaleGrayImg.create(m_Width, m_Height, CV_8UC1);
}

GetImage::GetImage(GET_IMG_MODE_E mode, GET_IMG_WORK_MODE_E workMode, int width, int height, char* pVideoPath)
{
    m_Mode = mode;
    m_WorkMode = workMode;
    m_Width = width;
    m_Height = height;

    if (m_Mode == GET_IMG_MODE_RECORD)
    {
        memcpy(&m_VideoPath[0], pVideoPath, strlen(pVideoPath));
        m_VideoPath[strlen(pVideoPath)] = '\0';
        m_Capture.open(m_VideoPath);
    }
    else if (m_Mode == GET_IMG_MODE_CAM)
    {
        m_Capture.open(0);
    }

    m_ScaleImg.create(m_Width, m_Height, CV_8UC3);
    m_ScaleGrayImg.create(m_Width, m_Height, CV_8UC1);
}

GetImage::~GetImage()
{
    m_Capture.release();
}

int GetImage::Work(Mat** ppstImage)
{
    m_Capture >> m_OrcImg;
    if (m_OrcImg.empty())
    {
        (*ppstImage) = NULL;
        return 0;
    }

    //图像缩放
    if (m_WorkMode == GET_IMG_WORK_MODE_ORC)
    {
        *ppstImage = &m_OrcImg;
        return 0;
    }
    else if (m_WorkMode == GET_IMG_WORK_MODE_RESIZE_YUV420)
    {
        resize(m_OrcImg, m_ScaleImg, Size(m_Width, m_Height), (0, 0), (0, 0), INTER_AREA);
        *ppstImage = &m_ScaleImg;
        return 0;
    }
    else if (m_WorkMode == GET_IMG_WORK_MODE_RESIZE_GRAY)
    {
        resize(m_OrcImg, m_ScaleImg, Size(m_Width, m_Height), (0, 0), (0, 0), INTER_AREA);
        cvtColor(m_ScaleImg, m_ScaleGrayImg, CV_BGR2GRAY);
        *ppstImage = &m_ScaleGrayImg;
        return 0;
    }

    return 0;
}