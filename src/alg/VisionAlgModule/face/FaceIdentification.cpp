#include "FaceIdentification.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <vector>

#include "caffe/caffe.hpp"
#include "caffe/util/io.hpp"

#include <opencv2/imgproc/types_c.h>

#include "comm/Des.h"
#include "boost/BoostFun.h"

using namespace std;
using namespace common_template;
using namespace Vision_FaceAlg;
const string strEnModelPath("./model/infrared_face_indentification_linux32_arm_v3.1.1.wave.en");

static void rotate_img(const cv::Mat& src, cv::Mat& dst, const cv::Mat pts, double theta, cv::Point2f pt)
{
	double angle = theta * 180 / 3.141592654;
	cv::Mat rotate_mat = cv::getRotationMatrix2D(pt, angle, 1.0);
	int row_dst = src.rows * cos(abs(theta)) + src.cols * sin(abs(theta));
	int col_dst = src.rows * sin(abs(theta)) + src.cols * cos(abs(theta));
	cv::Mat roteMat;
	roteMat.create(rotate_mat.rows, rotate_mat.cols, CV_32FC1);
	float* pData = (float*)roteMat.data;
	double* pData1 = (double*)rotate_mat.data;
	for (int m = 0; m < roteMat.rows; m++)
	{
		pData = (float*)(roteMat.data + m*roteMat.step[0]);
		pData1 = (double*)(rotate_mat.data + m*rotate_mat.step[0]);
		for (int n = 0; n < roteMat.cols; n++)
		{
			pData[n] = pData1[n];
		}
	}
	cv::warpAffine(src, dst, rotate_mat, cv::Size(col_dst, row_dst));
}

static int CropImgByFace(const cv::Mat& img, cv::Mat& crop_img, const cv::Mat &landmarks)
{
	if (img.data== NULL ||landmarks.data == NULL )
	{
		return -1;
	}

	cv::Point2f pts[196];
	float* landmark_data = (float*)landmarks.data;
	for (int k = 0; k < 196; k++)
	{
		pts[k].x = *landmark_data++;
		pts[k].y = *landmark_data++;
	}
	cv::Point2f le, re, nose, lm, rm;
	le.x = pts[96].x;
	le.y = pts[96].y;
	re.x = pts[97].x;
	re.y = pts[97].y;
	nose.x = pts[54].x;
	nose.y = pts[54].y;
	lm.x = pts[76].x;
	lm.y = pts[76].y;
	rm.x = pts[82].x;
	rm.y = pts[82].y;
	cv::Point2f pt(nose.x, nose.y);
	float centor_x = (le.x + re.x + nose.x + lm.x + rm.x) / 5;
	float centor_y = (le.y + re.y + nose.y + lm.y + rm.y) / 5;
	if (centor_x <= 0 || centor_x > img.cols - 1 || centor_y <= 0 || centor_y > img.rows - 1)
	{
		img.copyTo(crop_img);
		return 0;
	}
	cv::Point2f pt_centor(centor_x, centor_y);
	double theta = atan((re.y - le.y) / (re.x - le.x));
	cv::Mat img_rotated;
	cv::Mat ptsMat = cv::Mat::ones(3, 98, CV_32FC1);
	float* pData = (float*)(ptsMat.data + ptsMat.step[0]);
	float* pData1 = (float*)(ptsMat.data);
	for (int k = 0; k < 98; k++)
	{
		pData[k] = pts[k].y;
		pData1[k] = pts[k].x;
	}
	rotate_img(img, img_rotated, ptsMat, theta, pt_centor);
	double dis_eye = sqrt((re.y - le.y)*(re.y - le.y) + (re.x - le.x)*(re.x - le.x));
	double dis_em = sqrt(((re.y + le.y) / 2 - (rm.y + lm.y) / 2)*((re.y + le.y) / 2 - (rm.y + lm.y) / 2) + ((re.x + le.x) / 2 - (rm.x + lm.x) / 2)*((re.x + le.x) / 2 - (rm.x + lm.x) / 2));
	double dis = fmax(dis_eye, dis_em);
	int x_min = fmax(0, floor(centor_x - dis*1.3));
	int x_max = fmin(img_rotated.cols - 1, ceil(centor_x + dis*1.3));
	int y_min = fmax(0, floor(centor_y - dis*1.3));
	int y_max = fmin(img_rotated.rows - 1, ceil(centor_y + dis*1.3));
	img_rotated(cv::Rect(x_min, y_min, x_max - x_min, y_max - y_min)).copyTo(crop_img);
	return 0;
}

void FaceIdentification::MatTransformBlob(const cv::Mat& cv_img, void* transformed_blob, std::vector<float>mean, float scale)
{
    caffe::Blob<float> * blob = (caffe::Blob<float> *)transformed_blob;
	const int img_channels = cv_img.channels();
	const int img_height = cv_img.rows;
	const int img_width = cv_img.cols;
	const int channels = blob->channels();
	const int height = blob->height();
	const int width = blob->width();
	const int num = blob->num();
	CHECK_EQ(channels, img_channels);
	CHECK_EQ(channels, mean.size());
	CHECK_LE(height, img_height);
	CHECK_LE(width, img_width);
	CHECK_GE(num, 1);
	CHECK(cv_img.depth() == CV_8U) << "Image data type must be unsigned byte";
	float* transformed_data = blob->mutable_cpu_data();
	int top_index;
	for (int h = 0; h < img_height; ++h) 
	{
		const uchar* ptr = cv_img.ptr<uchar>(h);
		int img_index = 0;
		for (int w = 0; w < img_width; ++w) 
		{
			for (int c = 0; c < img_channels; ++c) 
			{
				top_index = (c * img_height + h) * img_width + w;
				float pixel;
	
				if (c==0)
				{
					pixel = (static_cast<float>(ptr[img_index++]) - mean[0]);
				}
				else if(c==1)
				{
					pixel = (static_cast<float>(ptr[img_index++]) - mean[1]);
				}
				else if (c==2)
				{
					pixel = (static_cast<float>(ptr[img_index++]) - mean[2]);
				}
				else
				{
					pixel = (static_cast<float>(ptr[img_index++]) - mean[2]);
				}
				transformed_data[top_index] = pixel*scale;
			}
		}
	}
}

void FaceIdentification::MatArrayTransformBlob(const std::vector<cv::Mat> & mat_vector,void* transformed_blob,std::vector<float>mean,float scale) 
{
	caffe::Blob<float> * blob = (caffe::Blob<float> *)transformed_blob;
	const int mat_num = mat_vector.size();
	const int num = blob->num();
	const int channels = blob->channels();
	const int height = blob->height();
	const int width = blob->width();
	CHECK_GT(mat_num, 0) << "There is no MAT to add";
	CHECK_EQ(mat_num, num) << "The size of mat_vector must be equals to blob->num()";
	caffe::Blob<float> uni_blob(1, channels, height, width);
	for (int item_id = 0; item_id < mat_num; ++item_id) 
	{
		int offset = blob->offset(item_id);
		uni_blob.set_cpu_data(blob->mutable_cpu_data() + offset);
		MatTransformBlob(mat_vector[item_id], &uni_blob, mean, scale);
	}
}


int FaceIdentification::Init(const Json& algCfg)
{
	if (facenet_ != nullptr)
	{
		return -1;
	}

	//close google log
	google::InitGoogleLogging("face_identifiction");
	FLAGS_minloglevel = 4;
	google::ShutdownGoogleLogging();

	//decrypt model file and save for new model file
	string strDecModelPath("./model/infrared_face_indentification_linux32_arm_v3.1.1.wave.de");
	decrypt_file(strEnModelPath,strDecModelPath);

	//init model
	caffe::NetParameter net_param_input;
	if(!caffe::ReadProtoFromBinaryFile(strDecModelPath,&net_param_input))
	{
		return -2;
	}
	net_param_input.mutable_state()->set_phase(caffe::TEST);
	net_param_input.mutable_state()->set_level(0);

	facenet_ = new caffe::Net<float>(net_param_input);

	caffe::Blob<float> *input = ((caffe::Net<float> *)facenet_)->blob_by_name("data").get();
	img_h_ = input->height();
	img_w_ = input->width();
	img_c_ = input->channels();

	feature_dim_ = ((caffe::Net<float> *) facenet_)->blob_by_name("feature_norm")->shape(1);
	
	system(string_format("rm -rf %s",strDecModelPath.c_str()).c_str());

	return 0;
}

int FaceIdentification::SetModeCPU()
{
	caffe::Caffe::set_mode(caffe::Caffe::CPU);
	return 0;
}

int FaceIdentification::SetModeGPU(int gpu_id)
{
	caffe::Caffe::set_mode(caffe::Caffe::GPU);
	caffe::Caffe::SetDevice(gpu_id);
	return 0;
}

//release
int FaceIdentification::Release()
{
	if (facenet_ != nullptr)
	{
		delete ((caffe::Net<float> *) facenet_);
	}
	return 0;
}

//change the batch size
int FaceIdentification::Reshape(int batchsize)
{
	caffe::Blob<float> *input = ((caffe::Net<float> *)facenet_)->blob_by_name("data").get();
	input->Reshape(batchsize, img_c_, img_h_, img_w_);
	((caffe::Net<float> *) facenet_)->Reshape();
	return 0;
}

//extract features
int FaceIdentification::ExtractFeature(const std::vector<cv::Mat> &imgs, const std::vector<cv::Mat> &landmarks, float* feature)
{
	if(imgs.size()<=0 || landmarks.size()<=0 || landmarks.size() != imgs.size())
	{
		return -1;
	}
	
	caffe::Blob<float> *input = ((caffe::Net<float> *)facenet_)->blob_by_name("data").get();
	std::vector<cv::Mat> matArray;
 
	for(int i = 0; i< imgs.size();i++)
	{
		cv::Mat crop_img;
        int ret = CropImgByFace(imgs[i], crop_img, landmarks[i]);

		if(ret) return -2;
		cv::Mat gray;
		cv::cvtColor(crop_img,gray,CV_BGR2GRAY);
		cv::Mat resize_img;
		cv::resize(gray,resize_img,cv::Size(img_w_,img_h_));
		matArray.push_back(resize_img);	
	}
 
	if(input->num() != matArray.size())
	{
		Reshape(matArray.size());
	}

    std::vector<float> means;
	means.push_back(127.5);
	float scale = 0.0078125;
	MatArrayTransformBlob(matArray,input,means,scale);
	((caffe::Net<float> *) facenet_)->Forward();
	caffe::Blob<float>* feature_blob = ((caffe::Net<float> *) facenet_)->blob_by_name("feature_norm").get();
	memcpy(feature, feature_blob->cpu_data(), imgs.size()*feature_dim_*sizeof(float));
	return 0;
}

//compute the score
double FaceIdentification::FaceVerify(const float* feature1, const float* feature2)
{
	double simi = 0;
	for (int i = 0; i < feature_dim_; ++i)
	{
		simi += feature1[i] * feature2[i];
	}
	return (simi + 1.0) / 2.0 ;
}
