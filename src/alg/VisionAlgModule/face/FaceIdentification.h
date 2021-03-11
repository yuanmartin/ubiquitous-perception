#pragma once
#include <semaphore.h>
#include <vector>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "comm/CommDefine.h"

namespace Vision_FaceAlg
{
	class FaceIdentification
	{
	public:
		FaceIdentification():facenet_(nullptr), feature_dim_(0), img_h_(0), img_w_(0){}
		~FaceIdentification(){}

		//initial
		int Init(const Json& algCfg);

		int SetModeCPU();

		int SetModeGPU(int gpu_id);
		
		//release
		int Release();

		//change the batch size
		int Reshape(int batchsize);

		//extract features
		int ExtractFeature(const std::vector<cv::Mat>& imgs, const std::vector<cv::Mat>& landmarks, float* feature);

		//get the feature's dimension
		int GetFeatureDim(){ return feature_dim_; }

		//get image's height
		int GetImgHeight(){ return img_h_; }

		//get image's width
		int GetImgWidth(){ return img_w_; }

		//compute the score
		double FaceVerify(const float* feature1, const float* feature2);

	private:
		void* facenet_;
		int feature_dim_;
		int img_h_;
		int img_w_;
		int img_c_;
		void MatTransformBlob(const cv::Mat& cv_img, void* transformed_blob, std::vector<float>mean, float scale);
		void MatArrayTransformBlob(const std::vector<cv::Mat> & mat_vector,void* transformed_blob,std::vector<float>mean,float scale);
	};
}

