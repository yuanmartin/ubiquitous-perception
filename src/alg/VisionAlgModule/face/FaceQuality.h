#pragma once
#include <iostream>
#include <string>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "comm/CommDefine.h"

namespace Vision_FaceAlg
{
    class FaceQuality
    {
    public:
        FaceQuality();
        ~FaceQuality();

    public:

        void Init(const Json& cfgJson){}
        //----------------------------------------
        // 质量判定函数
        // input:  input_image 输入图像，
        //         landmarks 人脸关键点，68个点，排列顺序为x1,y1,x2,y2,...类型CV_32FC1
        //         face_rect 人脸检测框（框的位置和大小）
        //         camera_mode 摄像头模式，0为可见光，1为近红外
        // output: quality_score 图像的质量分值
        // return: 0 正常 -1 输入图像为空
        //----------------------------------------
        int face_quality_final(cv::Mat input_image, cv::Mat landmarks,cv::Rect face_rect, int camera_mode, float &quality_score);


        //----------------------------------------------------------
        // 图像裁剪函数
        // input: src_image，RGB图像
        //        absolute_points，Mat类型, CV_32FC1，关键点在原图上的坐标
        //        坐标顺序：x1,y1,x2,y2,...,x68,y68
        // output: face_region,最小外接矩形扩充之后resize(用于ssim)
        //         face_region2,最小外接矩形resize(用于bright)
        // return: 0正常，-1非正常
        //-----------------------------------------------------------
        int FaceCrop(cv::Mat &input_image,
        cv::Mat &absolute_points,
        cv::Mat *face_region,
        cv::Mat *face_region2);

        //-----------------------------------------------
        // 对图像进行运动模糊
        // input: src_img 输入图像
        // output: blur_img 输出运动模糊图像
        // return 0 正常 -1 异常
        //--------------------------------------------------
        int MotionReblur(cv::Mat src_img, cv::Mat &blur_img);

        //--------------------------------------------
        // 计算图像的相似度指标
        // input: img1_temp,待评价的原始图像 
        //        img2_temp,原始图像img1_temp运动模糊之后的图像
        // output: quality_score 图像的质量分值
        //----------------------------------------------
        void GetMSSIM(const cv::Mat& i1, const cv::Mat& i2, float &quality_score);

        //------------------------------------------------------------
        // 图像亮度异常检测
        // input: InputImg 输入图像（已裁剪人脸）
        // output: ave_gray 灰度均值，weight_diff 加权偏差
        //------------------------------------------

        void brightnessException(cv::Mat InputImg,
        float& ave_gray, float& weight_diff);
    };
}