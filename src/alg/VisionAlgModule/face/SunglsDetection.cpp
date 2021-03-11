#include "SunglsDetection.h"
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

#include <opencv2/opencv.hpp>

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

int SunglassDetection::Init(const char* model_folder_path) 
{
    if (model_folder_path == NULL) 
    {
        return -1;
    }
    sunglass_detection_ = new rknn_context;
    int ret;
    int model_len = 0;
    char model_path[1024];
    memset(model_path, 0, 1024);
    sprintf(model_path, "%s/sunglass_detection_v0.0.3.rknn", model_folder_path);
    unsigned char* model;
    model = load_model(model_path, &model_len);
    ret = rknn_init((rknn_context*)(&sunglass_detection_), model, model_len, 0);
    if (ret < 0) 
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    if (sunglass_detection_ == NULL) 
    {
        goto init_failed;
    }
    return 0;

    init_failed:
    int nRet = Release();
    return -1;
}

int SunglassDetection::Detection(const cv::Mat& image) 
{
    if (image.data == NULL || image.cols <= 0 || image.rows <= 0 || image.channels() != 1) 
    {
        printf("image data is null or channels is not 1 !\n");
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

    int ret = rknn_inputs_set((rknn_context)sunglass_detection_, 1, inputs);
    if (ret < 0) 
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    struct timeval total_timer_start, total_timer_end;
    double timeuse;
    rknn_output outputs[1];
    ret = rknn_run((rknn_context)sunglass_detection_, nullptr);
    if (ret < 0) 
    {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }
    // Get Output
    memset(outputs, 0, sizeof(outputs));
    outputs[0].want_float = 1;
    ret = rknn_outputs_get((rknn_context)sunglass_detection_, 1, outputs, NULL);
    if (ret < 0) 
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        return -1;
    }
    // rknn extract feature end 
    if (((float*)(outputs[0].buf))[0] > ((float*)(outputs[0].buf))[1]) 
    {
        return 0;
    } 
    else 
    {
        return 1;
    }
    rknn_outputs_release((rknn_context)sunglass_detection_, 1, outputs);
}

int SunglassDetection::Release() 
{
    if (NULL != sunglass_detection_) 
    {
        rknn_destroy((rknn_context)sunglass_detection_);
        sunglass_detection_ = NULL;
    }
    return 0;
}
