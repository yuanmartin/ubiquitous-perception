#include "EyeOpening.h"
#include "rknn/rknn.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;
using namespace Vision_FaceAlg;
using std::max;
using std::min;

static unsigned char* load_model(const char* filename, int* model_size) 
{
    FILE* fp = fopen(filename, "rb");
    if (fp == nullptr) 
    {
        printf("fopen %s fail!\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    int model_len = ftell(fp);
    unsigned char* model = (unsigned char*)malloc(model_len);
    fseek(fp, 0, SEEK_SET);
    if (model_len != fread(model, 1, model_len, fp)) 
    {
        printf("fread %s fail!\n", filename);
        free(model);
        return NULL;
    }
    *model_size = model_len;
    if (fp) 
    {
        fclose(fp);
    }
    return model;
}

int EyeDetection::Init(const char* model_folder_path) 
{
    if (model_folder_path == NULL) 
    {
        return -1;
    }
    eye_detection_ = new rknn_context;
    int ret;
    int model_len = 0;
    char model_path[1024];
    memset(model_path, 0, 1024);
    sprintf(model_path, "%s/eye_opening_v0.0.2.rknn", model_folder_path);
    unsigned char* model;
    model = load_model(model_path, &model_len);
    ret = rknn_init((rknn_context*)(&eye_detection_), model, model_len, 0);
    if (ret < 0) 
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    if (eye_detection_ == NULL) 
    {
        goto init_failed;
    }

    return 0;
    init_failed:
    int nRet = Release();
    return -1;
}

int EyeDetection::Process(const cv::Mat& img_src,float* landmarks,cv::Mat* out) 
{
    cv::Rect bbox_eye;
    float left_eye_x = landmarks[96 * 2];
    float left_eye_y = landmarks[96 * 2 + 1];
    float right_eye_x = landmarks[97 * 2];
    float right_eye_y = landmarks[97 * 2 + 1];

    //图像旋转，旋转矩阵，仿射变换
    float tan = (left_eye_y - right_eye_y) / (left_eye_x - right_eye_x);
    float PI = 3.1415926;
    float tan_atan = atan(tan) * 180 / PI;
    float middle_eye_x = (left_eye_x + right_eye_x) / 2;
    float middle_eye_y = (left_eye_y + right_eye_y) / 2;
    cv::Point center = cv::Point(middle_eye_x, middle_eye_y);
    cv::Mat rot_mat;
    rot_mat = getRotationMatrix2D(center, tan_atan, 1.0);
    cv::Mat img_rotated;
    cv::warpAffine(img_src, img_rotated, rot_mat, img_src.size());
    
    //点围绕点旋转
    float left_eye_new_x =
        (left_eye_x - middle_eye_x) * cos(tan_atan * PI / 180) -
        (left_eye_y - middle_eye_y) * sin(tan_atan * PI / 180) + middle_eye_x;
    float left_eye_new_y =
        (left_eye_y - middle_eye_y) * cos(tan_atan * PI / 180) -
        (left_eye_x - middle_eye_x) * sin(tan_atan * PI / 180) + middle_eye_y;
    float right_eye_new_x =
        (right_eye_x - middle_eye_x) * cos(tan_atan * PI / 180) -
        (right_eye_y - middle_eye_y) * sin(tan_atan * PI / 180) + middle_eye_x;
    float right_eye_new_y =
        (right_eye_y - middle_eye_y) * cos(tan_atan * PI / 180) -
        (right_eye_x - middle_eye_x) * sin(tan_atan * PI / 180) + middle_eye_y;
    float middle_eye_instance = (right_eye_new_x - left_eye_new_x) / 2;

    bbox_eye.x = left_eye_new_x - middle_eye_instance;
    bbox_eye.y = left_eye_new_y - middle_eye_instance;
    bbox_eye.width = middle_eye_instance * 4;
    bbox_eye.height = middle_eye_instance * 2;
    bbox_eye.x = bbox_eye.x < 0 ? 0 : bbox_eye.x;
    bbox_eye.y = bbox_eye.y < 0 ? 0 : bbox_eye.y;
    bbox_eye.width = bbox_eye.width + bbox_eye.x >= img_rotated.cols
        ? img_rotated.cols - bbox_eye.x - 1
        : bbox_eye.width;
    bbox_eye.height = bbox_eye.height + bbox_eye.y >= img_rotated.rows
        ? img_rotated.rows - bbox_eye.y - 1
        : bbox_eye.height;
    if (bbox_eye.width <= 0 || bbox_eye.height <= 0) 
    {
        return -1;
    }
    cv::Mat frame_eye;
    img_rotated(bbox_eye).copyTo(frame_eye);
    cv::Mat frame_normal;
    cv::resize(frame_eye, frame_normal, cv::Size(128, 64), 0, 0, CV_INTER_LINEAR);
    cv::cvtColor(frame_normal, *out, CV_BGR2GRAY);
    return 0;
}

int EyeDetection::Detection(const cv::Mat& image, int* label) 
{
    if (image.data == NULL || image.cols <= 0 || image.rows <= 0 || image.channels() != 1) 
    {
        printf("image data is null or channels is not 3 !\n");
        return -1;
    }
    // rknn extract feature
    // Set Input Data
    // cvtColor(image, image_gray, COLOR_BGR2GRAY);
    // Run
    // Set Input Data
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = image.cols * image.rows * image.channels();
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = image.data;

    int ret = rknn_inputs_set((rknn_context)eye_detection_, 1, inputs);
    if (ret < 0) 
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    struct timeval total_timer_start, total_timer_end;
    double timeuse;
    rknn_output outputs[1];
    
    // gettimeofday(&total_timer_start,NULL);
    ret = rknn_run((rknn_context)eye_detection_, nullptr);
    if (ret < 0) 
    {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }

    // Get Output
    memset(outputs, 0, sizeof(outputs));
    outputs[0].want_float = 1;
    ret = rknn_outputs_get((rknn_context)eye_detection_, 1, outputs, NULL);
    if (ret < 0) 
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        return -1;
    }
  
    if (((float*)(outputs[0].buf))[0] > 0.48) 
    {
        *label = 0;
    } 
    else 
    {
        *label = 1;
    }
    rknn_outputs_release((rknn_context)eye_detection_, 1, outputs);
    return 0;
}

int EyeDetection::Release() 
{
    if (NULL != eye_detection_) 
    {
        rknn_destroy((rknn_context)eye_detection_);
        eye_detection_ = NULL;
    }

    return 0;
}
