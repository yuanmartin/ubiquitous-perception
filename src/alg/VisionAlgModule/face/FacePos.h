#pragma once
#include <iostream>
#include <string>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

namespace Vision_FaceAlg
{
    /**
    \brief
    判断当前人脸属于哪个人脸姿态，共有：抬头，低头，眨眼，张闭嘴，左转头，右转头
    */
    class FacePoseBJ 
    {
    public:
        FacePoseBJ();
        ~FacePoseBJ();

        /**
         \brief   初始化
        \param   model_path  [in]  人脸检测和人脸关键点定位模型所在文件路径，如："E:\\model\\"
        */
        int Init(const std::string& model_path);

        /**
         \brief   修改姿态阈值
        */
        int Config(
            const float& link_eye_thresh,
            const float& yaw_left_head,
            const float& yaw_right_head,
            const float& picth_up_head,
            const float& pitch_down_head,
            const float& mouth_thresh);

        ///**
        //\brief   获取特征点
        //\param   img_color    [in]  原图，rgb
        //\param   landmarks    [out]  特征点
        //*/
        // int GetLandmarks(const cv::Mat &img_color, cv::Mat *landmarks);

        /**
         \brief   估计人脸的三个姿态值,yaw,picth,roll
        \param   img_color     [in]  图像，rgb原图
        \param   is_draw_pose  [in]  是否将当前人脸的姿态画到图像上进行显示
        \param   label         [out]  当前所要做的是哪种姿态,0:左转头；1:右转头，2:抬头，3:低头
        \return  -1 检测不到人脸，0正常
        */
        int Run(
            const cv::Mat& landmarks,
            float* thresh_face,
            float* thresh_face_in,
            double* pitch_result,
            double* yaw_result,
            int* label_ex,
            int* label_in);

        /**
         \brief   估计每一帧的张闭嘴动作
        \param   img_color     [in]  图像，rgb原图
        \param   is_draw_pose  [in]  是否将当前人脸的姿态画到图像上进行显示
        \param   distance      [out]  每一帧的距离值
        \return  -1 检测不到人脸，0正常
        */
        int Run_mouth_per_frame(const cv::Mat& landmarks, float* distance);

        /**
         \brief   估计多帧的张闭嘴动作
        \param   distances     [in]  array_length
        长度的数组，即选取array_length帧的计算 出每一帧的距离值组成的数据 \param
        array_length  [in]  array_length，一共选取的帧数 \param   label         [out]
        输出是否为张闭嘴动作，1为是张闭嘴动作，0代表不是 \param   mouth_thresh_predict
        [out] 一段帧数的张闭嘴预测的阈值 \return  -1 检测不到人脸，0正常
        */

        /*int Run_mouth_frames(const float* distances, const int &array_length,
                        float* mouth_thresh_predict, int* label);*/
        int Run_mouth_frames(
            const float& mouse_threshold,
            const float* distances,
            int* label);

        /**
         \brief   估计每一帧的眨眼
        \param   img_color  [in]  图像，rgb原图
        \param   label      [in]  是否有眨眼动作
        \return  -1 检测不到人脸，0正常
        */
        int Run_eye_max_threshold(
            const cv::Mat& landmarks,
            float* ear_open_threshold);

        /**
         \brief   估计每一帧的眨眼
        \param   img_color  [in]  图像，rgb原图
        \param   label      [in]  是否有眨眼动作
        \return  -1 检测不到人脸，0正常
        */
        int Run_eye(
            const float& ear_open_threshold,
            const cv::Mat& landmarks,
            float* ear,
            int* label);

        int Wearglass(
            cv::Mat srcImage,
            cv::Mat landmarks,
            int len_landmarks,
            int threshold_pixel,
            float threshold_percent,
            float* Percentage,
            int* state_sunglass);

        /**
         \brief   释放函数
        */
        int Release();

        private:
            void* pose_model_; //人脸姿态估计模型
            void* fd_model_; //人脸检测模型
            void* fa_model_; //人脸关键点定位模型
            float link_eye_thresh_; //眨眼的阈值，默认为5
            float yaw_left_head_; //判断左转头的阈值，默认为-10
            float yaw_right_head_; //判断右转头的阈值，默认为10
            float picth_up_head_; //判断抬头的阈值，默认为-8
            float pitch_down_head_; //判断低头的阈值，默认为10
            float mouth_thresh_;
    }; 

    class PoseEastimation
    {
    public:
        PoseEastimation() 
        {
            static int HeadPosePointIndexs[] = { 60, 64, 68, 72, 54, 76, 82 };
            estimateHeadPosePointIndexs = HeadPosePointIndexs;
            static float estimateHeadPose2dArray[] = {
                -0.208764, -0.140359, 0.458815, 0.106082, 0.00859783, -0.0866249, -0.443304, -0.00551231, -0.0697294,
                -0.157724, -0.173532, 0.16253, 0.0935172, -0.0280447, 0.016427, -0.162489, -0.0468956, -0.102772,
                0.126487, -0.164141, 0.184245, 0.101047, 0.0104349, -0.0243688, -0.183127, 0.0267416, 0.117526,
                0.201744, -0.051405, 0.498323, 0.0341851, -0.0126043, 0.0578142, -0.490372, 0.0244975, 0.0670094,
                0.0244522, -0.211899, -1.73645, 0.0873952, 0.00189387, 0.0850161, 1.72599, 0.00521321, 0.0315345,
                -0.122839, 0.405878, 0.28964, -0.23045, 0.0212364, -0.0533548, -0.290354, 0.0718529, -0.176586,
                0.136662, 0.335455, 0.142905, -0.191773, -0.00149495, 0.00509046, -0.156346, -0.0759126, 0.133053,
                -0.0393198, 0.307292, 0.185202, -0.446933, -0.0789959, 0.29604, -0.190589, -0.407886, 0.0269739,
                -0.00319206, 0.141906, 0.143748, -0.194121, -0.0809829, 0.0443648, -0.157001, -0.0928255, 0.0334674,
                -0.0155408, -0.145267, -0.146458, 0.205672, -0.111508, 0.0481617, 0.142516, -0.0820573, 0.0329081,
                -0.0520549, -0.329935, -0.231104, 0.451872, -0.140248, 0.294419, 0.223746, -0.381816, 0.0223632,
                0.176198, -0.00558382, 0.0509544, 0.0258391, 0.050704, -1.10825, -0.0198969, 1.1124, 0.189531,
                -0.0352285, 0.163014, 0.0842186, -0.24742, 0.199899, 0.228204, -0.0721214, -0.0561584, -0.157876,
                -0.0308544, -0.131422, -0.0865534, 0.205083, 0.161144, 0.197055, 0.0733392, -0.0916629, -0.147355,
                0.527424, -0.0592165, 0.0150818, 0.0603236, 0.640014, -0.0714241, -0.0199933, -0.261328, 0.891053 };
            estimateHeadPoseMat = cv::Mat(15, 9, CV_32FC1, estimateHeadPose2dArray);
            static float estimateHeadPose2dArray2[] = {
                0.139791, 27.4028, 7.02636,
                -2.48207, 9.59384, 6.03758,
                1.27402, 10.4795, 6.20801,
                1.17406, 29.1886, 1.67768,
                0.306761, -103.832, 5.66238,
                4.78663, 17.8726, -15.3623,
                -5.20016, 9.29488, -11.2495,
                -25.1704, 10.8649, -29.4877,
                -5.62572, 9.0871, -12.0982,
                -5.19707, -8.25251, 13.3965,
                -23.6643, -13.1348, 29.4322,
                67.239, 0.666896, 1.84304,
                -2.83223, 4.56333, -15.885,
                -4.74948, -3.79454, 12.7986,
                -16.1, 1.47175, 4.03941 };
            estimateHeadPoseMat2 = cv::Mat(15, 3, CV_32FC1, estimateHeadPose2dArray2);

            yaw = 0.0;
            pitch = 0.0;
            roll = 0.0;
        }

        double getYaw()
        {
            return yaw;
        }

        double getPitch()
        {
            return pitch;
        }

        double getRoll()
        {
            return roll;
        }

        //返回值为：1，表示pitch,yaw,roll无一个大于thresh
        //返回值为：0，表示pithc,yaw,roll有值大于Thresh
        //返回值为：-1，表示人脸关键点为空
        // img:输入人脸图像
        //current_shape:输入人脸关键点,float类型,坐标存储方式为：x1,x2,...,x68,y1,y2,...,y68
        //thresh:角度阈值
        int Predict(cv::Mat& img, const cv::Mat& current_shape, double thresh, cv::Mat *predict)
        {
            if (current_shape.empty())
                return -1;
            static const int samplePdim = 7;
            float miny = 10000000000.0f;
            float maxy = 0.0f;
            float sumx = 0.0f;
            float sumy = 0.0f;
            for (int i = 0; i<samplePdim; i++){
                sumx += current_shape.at<float>(estimateHeadPosePointIndexs[i]);
                float y = current_shape.at<float>(estimateHeadPosePointIndexs[i] + current_shape.cols / 2);
                sumy += y;
                if (miny > y)
                    miny = y;
                if (maxy < y)
                    maxy = y;
            }
            float dist = maxy - miny;
            sumx = sumx / samplePdim;
            sumy = sumy / samplePdim;
            static cv::Mat tmp(1, 2 * samplePdim + 1, CV_32FC1);
            for (int i = 0; i<samplePdim; i++){
                tmp.at<float>(i) = (current_shape.at<float>(estimateHeadPosePointIndexs[i]) - sumx) / dist;
                tmp.at<float>(i + samplePdim) = (current_shape.at<float>(estimateHeadPosePointIndexs[i] + current_shape.cols / 2) - sumy) / dist;
            }
            tmp.at<float>(2 * samplePdim) = 1.0f;

            //第二种求解yaw pitch roll
            (*predict) = tmp*estimateHeadPoseMat2;
            std::stringstream pitch2;
            pitch2 << (*predict).at<float>(0);
            pitch2 >> pitch;
		    ////cout << pitch << endl;
            //		std::string txt1 = "pitch: " + pitch2.str();
            //		cv::putText(img, txt1, cv::Point(340, 20), 0.5, 0.5, cv::Scalar(255, 255, 255));

            std::stringstream yaw2;
            yaw2 << (*predict).at<float>(1);
            yaw2 >> yaw;
            ////cout << yaw << endl;
            //		std::string txt2 = "yaw: " + yaw2.str();
            //		cv::putText(img, txt2, cv::Point(340, 40), 0.5, 0.5, cv::Scalar(255, 255, 255));

            std::stringstream roll2;
            roll2 << (*predict).at<float>(2);
            roll2 >> roll;
            ////cout << roll << endl;
            //		std::string txt3 = "Roll: " + roll2.str();
            //		cv::putText(img, txt3, cv::Point(340, 60), 0.5, 0.5, cv::Scalar(255, 255, 255));
            //将roll,picth,yaw与阈值相比较
            if ( abs(pitch) > thresh ||  abs(yaw) > thresh ||  abs(roll) > thresh)
                return 0;
            else
                return 1;
        }

    private:
        cv::Mat estimateHeadPoseMat;
        cv::Mat estimateHeadPoseMat2;
        int *estimateHeadPosePointIndexs;
        double yaw;
        double roll;
        double pitch;

    };  
}