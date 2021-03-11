#include "FacePos.h"
#include <opencv2/imgproc/types_c.h>

using namespace std;
using namespace cv;
using namespace Vision_FaceAlg;

FacePoseBJ::FacePoseBJ() 
{
    pose_model_ = NULL;
    fd_model_ = NULL;
    fa_model_ = NULL;
}

FacePoseBJ::~FacePoseBJ() 
{
    pose_model_ = NULL;
    fd_model_ = NULL;
    fa_model_ = NULL;
}

int FacePoseBJ::Init(const string& model_path) 
{
    pose_model_ = new PoseEastimation();

    link_eye_thresh_ = 0.07;
    yaw_left_head_ = -13.418;
    yaw_right_head_ = 13.0494;
    picth_up_head_ = -3.17948;
    pitch_down_head_ = 12.4126;
    mouth_thresh_ = 0.02;

    return 0;
}

int FacePoseBJ::Config(
    const float& link_eye_thresh,
    const float& yaw_left_head,
    const float& yaw_right_head,
    const float& picth_up_head,
    const float& pitch_down_head,
    const float& mouth_thresh) 
{
    link_eye_thresh_ = link_eye_thresh;
    yaw_left_head_ = yaw_left_head;
    yaw_right_head_ = yaw_right_head;
    picth_up_head_ = picth_up_head;
    pitch_down_head_ = pitch_down_head;
    mouth_thresh_ = mouth_thresh;
    return 0;
}

int FacePoseBJ::Run(
    const cv::Mat& shape,
    float* thresh_face,
    float* thresh_face_in,
    double* pitch_result,
    double* yaw_result,
    int* label_ex,
    int* label_in) 
{
    if (shape.empty()) 
    {
        std::cout << "shape is empty" << std::endl;
        return -1;
    }

    cv::Mat landmarks = cv::Mat::zeros(1, 196, CV_32FC1);
    for (int i = 0; i < 98; i++) 
    {
        landmarks.at<float>(i) = shape.at<float>(2 * i);
        landmarks.at<float>(i + 98) = shape.at<float>(2 * i + 1);
    }

    cv::Mat frame_state;
    double thresh = thresh_face[6]; //定义角度阈值
    //	float thresh_face[6] = { 12.4126, -3.17948, 13.0494, -13.418, 15.962,
    //-13.8352 };
    cv::Mat predict(1, 3, CV_32FC1);
    ((PoseEastimation*)pose_model_)->Predict(frame_state, landmarks, thresh, &predict);

    //判断姿态
    if (predict.at<float>(0, 0) < thresh_face[0] &&
        predict.at<float>(0, 0) > thresh_face[1] &&
        predict.at<float>(0, 1) < thresh_face[2] &&
        predict.at<float>(0, 1) > thresh_face[3]) 
    {
        *label_ex = 0; //正脸
    } 
    else 
    {
        *label_ex = 1; //侧脸
    }
    if (predict.at<float>(0, 0) < thresh_face_in[0] &&
        predict.at<float>(0, 0) > thresh_face_in[1] &&
        predict.at<float>(0, 1) < thresh_face_in[2] &&
        predict.at<float>(0, 1) > thresh_face_in[3]) 
    {
        *label_in = 0; //正脸in
    } 
    else 
    {
        *label_in = 1; //侧脸in
    }

    *pitch_result = predict.at<float>(0, 0);
    *yaw_result = predict.at<float>(0, 1);
    return 0;
}

int FacePoseBJ::Run_eye_max_threshold(
    const cv::Mat& shape,
    float* ear_open_threshold) 
{
    if (shape.empty()) 
    {
        std::cout << "shape is empty" << std::endl;
        return -1;
    }

    cv::Mat landmarks = cv::Mat::zeros(1, 196, CV_32FC1);
    for (int i = 0; i < 98; i++) 
    {
        landmarks.at<float>(i) = shape.at<float>(2 * i);
        landmarks.at<float>(i + 98) = shape.at<float>(2 * i + 1);
    }

    float ear_point[32] = {0};
    for (int i = 0; i < 16; ++i) 
    {
        ear_point[i] = landmarks.at<float>(0, i + 60);
        ear_point[i + 16] = landmarks.at<float>(0, i + 60 + 98);
    }
    float EAR_formula_left = (sqrt(
                                pow(ear_point[1] - ear_point[7], 2) +
                                pow(ear_point[17] - ear_point[23], 2)) +
                            sqrt(
                                pow(ear_point[2] - ear_point[6], 2) +
                                pow(ear_point[18] - ear_point[22], 2)) +
                            sqrt(
                                pow(ear_point[3] - ear_point[5], 2) +
                                pow(ear_point[19] - ear_point[21], 2))) /
        (3 *
        sqrt(
            pow(ear_point[0] - ear_point[4], 2) +
            pow(ear_point[16] - ear_point[20], 2)));

    float EAR_formula_right = (sqrt(
                                    pow(ear_point[9] - ear_point[15], 2) +
                                    pow(ear_point[25] - ear_point[31], 2)) +
                                sqrt(
                                    pow(ear_point[10] - ear_point[14], 2) +
                                    pow(ear_point[26] - ear_point[30], 2)) +
                                sqrt(
                                    pow(ear_point[11] - ear_point[13], 2) +
                                    pow(ear_point[27] - ear_point[29], 2))) /
        (3 *
        sqrt(
            pow(ear_point[8] - ear_point[12], 2) +
            pow(ear_point[24] - ear_point[28], 2)));

    *ear_open_threshold = (EAR_formula_left + EAR_formula_right) / 2;
    *ear_open_threshold = *ear_open_threshold - 0.07;
    return 0;
}

int FacePoseBJ::Run_eye(
    const float& ear_open_threshold,
    const cv::Mat& shape,
    float* ear,
    int* label) 
{
  if (shape.empty()) 
  {
    std::cout << "shape is empty" << std::endl;
    return -1;
  }
  cv::Mat landmarks = cv::Mat::zeros(1, 196, CV_32FC1);
  for (int i = 0; i < 98; i++) 
  {
    landmarks.at<float>(i) = shape.at<float>(2 * i);
    landmarks.at<float>(i + 98) = shape.at<float>(2 * i + 1);
  }
  float ear_point[32] = {0};
  for (int i = 0; i < 16; ++i) 
  {
    ear_point[i] = landmarks.at<float>(0, i + 60);
    ear_point[i + 16] = landmarks.at<float>(0, i + 60 + 98);
  }
  float EAR_formula_left = (sqrt(
                                pow(ear_point[1] - ear_point[7], 2) +
                                pow(ear_point[17] - ear_point[23], 2)) +
                            sqrt(
                                pow(ear_point[2] - ear_point[6], 2) +
                                pow(ear_point[18] - ear_point[22], 2)) +
                            sqrt(
                                pow(ear_point[3] - ear_point[5], 2) +
                                pow(ear_point[19] - ear_point[21], 2))) /
      (3 *
       sqrt(
           pow(ear_point[0] - ear_point[4], 2) +
           pow(ear_point[16] - ear_point[20], 2)));
  float EAR_formula_right = (sqrt(
                                 pow(ear_point[9] - ear_point[15], 2) +
                                 pow(ear_point[25] - ear_point[31], 2)) +
                             sqrt(
                                 pow(ear_point[10] - ear_point[14], 2) +
                                 pow(ear_point[26] - ear_point[30], 2)) +
                             sqrt(
                                 pow(ear_point[11] - ear_point[13], 2) +
                                 pow(ear_point[27] - ear_point[29], 2))) /
      (3 *
       sqrt(
           pow(ear_point[8] - ear_point[12], 2) +
           pow(ear_point[24] - ear_point[28], 2)));
  *ear = (EAR_formula_left + EAR_formula_right) / 2;
   
  if (*ear > ear_open_threshold) 
  {
    *label = 0;
  } 
  else 
  {
    *label = 1;
  }
  return 0;
}

int FacePoseBJ::Run_mouth_per_frame(const cv::Mat& shape, float* distance) 
{
    if (shape.empty()) 
    {
        std::cout << "shape is empty" << std::endl;
        return -1;
    }

    cv::Mat landmarks = cv::Mat::zeros(1, 196, CV_32FC1);
    for (int i = 0; i < 98; i++) 
    {
        landmarks.at<float>(i) = shape.at<float>(2 * i);
        landmarks.at<float>(i + 98) = shape.at<float>(2 * i + 1);
    }
    float mouth_len_vertical = sqrt(
        (landmarks.at<float>(0, 90) - landmarks.at<float>(0, 94)) *
            (landmarks.at<float>(0, 90) - landmarks.at<float>(0, 94)) +
        (landmarks.at<float>(0, 90 + 98) - landmarks.at<float>(0, 94 + 98)) *
            (landmarks.at<float>(0, 90 + 98) - landmarks.at<float>(0, 94 + 98)));
    float mouth_len_horizontal = sqrt(
        (landmarks.at<float>(0, 76) - landmarks.at<float>(0, 82)) *
            (landmarks.at<float>(0, 76) - landmarks.at<float>(0, 82)) +
        (landmarks.at<float>(0, 76 + 98) - landmarks.at<float>(0, 82 + 98)) *
            (landmarks.at<float>(0, 76 + 98) - landmarks.at<float>(0, 82 + 98)));
    *distance = mouth_len_vertical / mouth_len_horizontal;
    return 0;
}

int FacePoseBJ::Run_mouth_frames(
    const float& mouse_threshold,
    const float* distances,
    int* label) 
{
    if (*distances > mouse_threshold) 
    {
        *label = 1;
    } 
    else 
    {
        *label = 0;
    }
    return 0;
}

//墨镜红外阻断
int FacePoseBJ::Wearglass(
    cv::Mat srcImage,
    cv::Mat landmarks,
    int len_landmarks,
    int threshold_pixel,
    float threshold_percent,
    float* Percentage,
    int* state_sunglass) 
{
    if (landmarks.empty()) 
    {
        std::cout << "landmarks is empty" << std::endl;
        return -1;
    }
    
    cv::Rect bbox_eye;
    float scale = 1;
    float left_eye_x = landmarks.at<float>(192);
    float left_eye_y = landmarks.at<float>(193);
    float right_eye_x = landmarks.at<float>(194);
    float right_eye_y = landmarks.at<float>(195);
    float middle_eye_instance = (right_eye_x - left_eye_x) / 2;
    bbox_eye.x = left_eye_x - middle_eye_instance * scale;
    bbox_eye.y = left_eye_y - middle_eye_instance * scale;
    bbox_eye.width = middle_eye_instance * 4 * scale;
    bbox_eye.height = middle_eye_instance * 2 * scale;

    //判断是否有超边界
    if (bbox_eye.x < 0) {
    bbox_eye.x = 0;
    }
    if (bbox_eye.y < 0) {
    bbox_eye.y = 0;
    }
    if (bbox_eye.y + bbox_eye.height >= srcImage.rows) {
    bbox_eye.height = srcImage.rows - bbox_eye.y;
    }
    if (bbox_eye.x + bbox_eye.width >= srcImage.cols) {
    bbox_eye.width = srcImage.cols - bbox_eye.x;
    }
    if (bbox_eye.height < 5 || bbox_eye.width < 5) {
    return -1;
    }

    cv::Mat crop_eye;
    srcImage(bbox_eye).copyTo(crop_eye);
    if (crop_eye.channels() > 1) 
    {
        cvtColor(crop_eye, crop_eye, CV_RGB2GRAY);
    }
    cv::Mat srcImageBin;
    threshold(crop_eye, srcImageBin, threshold_pixel, 255, CV_THRESH_BINARY);

    // 2019-9-18 墨镜算法修改为自适应阈值
    // cv::adaptiveThreshold(crop_eye, srcImageBin, 255,
    // cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 21, 10);
    int num = 0;
    int* colswidth = new int[crop_eye.cols]; //申请src.image.cols个int型的内存空间
    memset(colswidth,0,crop_eye.cols * 4); //数组必须赋初值为零，否则出错。无法遍历数组。
    int value;
    for (int i = 0; i < crop_eye.cols; i++)
    for (int j = 0; j < crop_eye.rows; j++) 
    {
        value = srcImageBin.at<uchar>(j, i);
        if (value == 0) 
        {
            num++;
        }
    }
    *Percentage = (float)num / (float)(crop_eye.cols * crop_eye.rows);
    if (*Percentage > threshold_percent) 
    {
        *state_sunglass = 1;
    } 
    else 
    {
        *state_sunglass = 0;
    }
    return 0;
}

int FacePoseBJ::Release() 
{
    if (pose_model_ != NULL) 
    {
        delete (PoseEastimation*)pose_model_;
    }
    return 0;
}