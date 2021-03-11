#pragma once

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

namespace Vision_FaceAlg
{
    /**
     * @brief EyeDetection 人眼状态检测
     */
    class  EyeDetection 
    {
    public:
        /**
         * @brief EyeDetection 构造函数.
         * 默认设置eye_detection_为空指针.
         */
        EyeDetection() : eye_detection_(0)
        {
        }
        
        ~EyeDetection()
        {
        }

        /**
         * @brief 算法初始化函数,初始化算法的模型所在路径
         * @param [in] modelpath 输入模型的存放目录路径，如果模型存放在"./models"下，则传入值"./models"
         * @return 返回执行状态值
         * @retval 0 表示成功
         * @retval 其他值 表示失败.   
         */
        int Init(const char* modelpath);


        /**
         * @brief 资源释放函数，释放算法内部开辟的内存资源，在确定不再调用算法的情况下，可调用此函数释放资源.
         * @return 返回执行状态值.
         * @retval 0 表示成功
         * @retval 其他值 表示失败. 
         */
        int Release();

        int Process(const cv::Mat &img, float* landmark, cv::Mat *out);

        /** 
         *  @brief 获取人眼状态
         *  @param [in] img 输入原始图像
         *  @param [out] 
         *  @return 返回执行状态值.
         *  @retval 0 表示正常睁眼状态
         *  @retval 1 表示闭眼状态.
         */
        int Detection(const cv::Mat &gray, int * label);

    private:
        void* eye_detection_;
    };
}
