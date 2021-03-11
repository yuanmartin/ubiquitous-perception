#pragma once
#include "opencv2/highgui.hpp"
using namespace cv;

#define FILENAMELEN 1000

namespace usb_camera
{
    typedef enum enGetImgMode
    {
        GET_IMG_MODE_RECORD,
        GET_IMG_MODE_CAM
    }GET_IMG_MODE_E;

    typedef enum enGetImgWorkMode
    {
        GET_IMG_WORK_MODE_ORC,              //获取原图
        GET_IMG_WORK_MODE_RESIZE_YUV420,    //获取缩放后的YUV420图
        GET_IMG_WORK_MODE_RESIZE_GRAY       //获取缩放后灰度图
    }GET_IMG_WORK_MODE_E;

    class GetImage
    {
    public:
        GetImage();
        GetImage(GET_IMG_MODE_E mode, GET_IMG_WORK_MODE_E workMode, int width, int height, char* pVideoPath);
        ~GetImage();

        int Work(Mat** ppstImage);
    public:
        GET_IMG_MODE_E m_Mode;
        GET_IMG_WORK_MODE_E m_WorkMode;

        char m_VideoPath[FILENAMELEN];
        int m_Width;
        int m_Height;
        VideoCapture m_Capture;
        Mat m_OrcImg;
        Mat m_ScaleImg;
        Mat m_ScaleGrayImg;
    };
}