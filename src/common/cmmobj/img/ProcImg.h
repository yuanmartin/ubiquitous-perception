#pragma once
#include <vector>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgcodecs/legacy/constants_c.h"

#include "endecode/Base64.h"

static cv::Scalar colorArray[10] = 
{
    cv::Scalar(139,   0,   0, 255),
    cv::Scalar(139,   0, 139, 255),
    cv::Scalar(  0,   0, 139, 255),
    cv::Scalar(  0, 100,   0, 255),
    cv::Scalar(139, 139,   0, 255),
    cv::Scalar(209, 206,   0, 255),
    cv::Scalar(  0, 127, 255, 255),
    cv::Scalar(139,  61,  72, 255),
    cv::Scalar(  0, 255,   0, 255),
    cv::Scalar(255,   0,   0, 255),
};

static void ProportionalZoom(const cv::Mat& srcImg,const cv::Mat& dstImg,const int& img_channels)
{
	int s32Scale = srcImg.cols / dstImg.cols;
	int s32SrcMidCols = srcImg.cols / s32Scale;
	int s32SrcMidRows = srcImg.rows / s32Scale;
	int dh = (dstImg.rows - s32SrcMidRows) / 2;
	if(dstImg.cols != s32SrcMidCols || 0 > dh)
	{
		return;
	}

	unsigned char* ptrDataDst = dstImg.data + dh * dstImg.cols * img_channels;
	for(int i = dh;i < dstImg.rows - dh;i++)
	{
		for(int j = 0;j < dstImg.cols;j++)
		{
			memcpy(ptrDataDst,srcImg.data + ((i - dh) * srcImg.cols + j) * s32Scale * img_channels,img_channels);
			ptrDataDst += img_channels;
		}
	}
}

static std::string Base64EnImg(const cv::Mat& srcImg)
{
	std::vector<unsigned char> buff;
	cv::imencode(".jpg", srcImg, buff);
	common_cmmobj::CBase64 base64Encode;
	return base64Encode.Encode((char*)buff.data(), buff.size());
}

static void Base64DeImg(const std::string& strImgData,cv::Mat& dstImg)
{
	common_cmmobj::CBase64 base64Decode;
	int s32OutSize = 0;
	std::string&& strDecodeData = base64Decode.Decode(strImgData.data(),strImgData.size(),s32OutSize);

	std::vector<unsigned char> buff;
	buff.assign(strDecodeData.begin(),strDecodeData.end());
	cv::imdecode(buff, CV_LOAD_IMAGE_COLOR,&dstImg);
}

static int ImgToSlices(const cv::Mat &src, std::vector<cv::Mat> &result, std::vector<cv::Point> &transPts, int inWidth) 
{	
	cv::Mat squareImg(src.cols, src.cols, src.type());
	cv::Mat resizeImg(inWidth, inWidth, src.type());
	int sizeRowSrc = src.cols * src.channels();
	int dh = (src.cols - src.rows) / 2;
	for (int i = 0; i < squareImg.rows; i++) {
		if (i < dh || i >= dh + src.rows) {
			memset(squareImg.data + i* sizeRowSrc, 0, sizeRowSrc);
		}
		else {
			memcpy(squareImg.data + i * sizeRowSrc, src.data + (i-dh)*sizeRowSrc, sizeRowSrc);
		}
	}
	resize(squareImg, resizeImg, cv::Size(inWidth, inWidth));
	result.push_back(resizeImg);
	
	float k = src.cols / (float)inWidth;
	if (k < 2) return 0;

	float dx = inWidth - inWidth / k;
	k++;

	int x=0, y;
	cv::Rect roi;
	roi.width = roi.height = inWidth;
	for (int i = 0; i < k; i ++) {
		y = 0;
		for (int j = 0; j < k; j++) {
			//以x,y为左上角的图像		
			roi.x = x;
			roi.y = y;
			cv::Mat subImg = squareImg(roi);
			result.push_back(subImg);
			transPts.push_back(cv::Point(x,y));

			y += dx;
		}
		x += dx;
	}
	return 0;
}

//idx 表示要还原的是第几张图，也是transPts的下标
static void GetOriginRect(const box &inRect, const cv::Point &transPt, const int inWidth, const int srcWidth, box &outRect) 
{
	outRect = inRect;
	outRect.x *= inWidth;
	outRect.y *= inWidth;
	outRect.w *= inWidth;
	outRect.h *= inWidth;

	outRect.x = outRect.x  + transPt.x;
	outRect.y = outRect.y  + transPt.y;

	outRect.x /= srcWidth;
	outRect.y /= srcWidth;
	outRect.w /= srcWidth;
	outRect.h /= srcWidth;
}