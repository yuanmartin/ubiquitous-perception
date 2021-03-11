#include "FaceAlg.h"
#include "log4cxx/Loging.h"
#include "boost/BoostFun.h"
#include "SubTopic.h"
#include "endecode/Base64.h"
#include "comm/CommFun.h"
#include "CommData.h"
#include "comm/FuncInitLst.h"

using namespace std;
using namespace cv;
using namespace common_cmmobj;
using namespace common_template;
using namespace Vision_FaceAlg;

const int net_width = 800;
const int net_height = 464;
const int roi_size = 112;
const float fFaceThershold = 0.77;

bool CFaceAlg::Init(const MsgBusShrPtr& ptrMsgBus,const Json& taskCfg,const Json& algCfg,const vector<Json>& lstDataSrcCfg,const map<int,vector<float>>& mapFaceFeature)
{
    //消息总线初始化
	if(!m_ptrMsgBus)
	{
		m_ptrMsgBus = ptrMsgBus;
	}
	
	//结果上报主题列表
	string&& strTopicKey = AlgResultTopic(string(taskCfg[strAttriType]),string(taskCfg[strAttriIdx]));
	SetReportTopic(strTopicKey);

	//算法初始化
	if(!GetInitStatus())
	{
		AlgInitLst(algCfg,m_ptrFaceDetect,m_ptrFaceAlign,m_ptrFaceIdentify);

		//设置特征维度
		SetFeatureDim();

		//设置初始化标志
		SetInitStatus();
	}
	
	//保存人脸特征
	SetFaceFeature(mapFaceFeature);

	//数据源配置初始化
	return DataSrcInit(lstDataSrcCfg);
}

bool CFaceAlg::DataSrcInit(const vector<Json>& lstDataSrcCfg)
{
	for(const auto& DataSrcCfg : lstDataSrcCfg)
	{
		//订阅视频流数据
		string strIp = DataSrcCfg[strAttriIp];
		string strTopic = RtspStreamTopic(strIp,DataSrcCfg[strAttriParams][strAttriCode]);
		if(0 == m_mapDataSrcTopic.count(strTopic))
		{
			auto&& fun = [this](const string& strIp,const string& strCamCode,const cv::Mat& srcImg){ProcVideoStream(strIp,strCamCode,srcImg);};
			m_ptrMsgBus->Attach(move(fun),move(strTopic));
		}
	}
	return true;
}

bool CFaceAlg::PreprocMat(const Mat& srcImg,Mat& dstImg,int& s32Width,int& s32Height)
{
    bool bResize = false;
	if(net_width != srcImg.cols || net_height != srcImg.rows)
	{
		s32Height = s32Width * srcImg.rows / srcImg.cols;
		if(s32Height > net_height)
		{
			s32Height = net_height;
			s32Width = s32Height * srcImg.cols / srcImg.rows;
		}

		cv::resize(srcImg, dstImg, cv::Size(s32Width, s32Height));
    	cv::copyMakeBorder(dstImg, dstImg, 0, net_height - s32Height, 0, net_width - s32Width,cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
		bResize = true;
	}
	else
	{
		srcImg.copyTo(dstImg);
	}
	
	return bResize;
}

void CFaceAlg::FaceFeatureExt(const Mat& matFaceImg,vector<vector<float>>& vecFeature,vector<Mat>& vecRoiMat,vector<FaceRectInfo>& vecFaceRect)
{
  	/*源图像预处理*/
	int s32Width = net_width;
	int s32Height = net_height;
    Mat dstImg;
    bool bRet = PreprocMat(matFaceImg,dstImg,s32Width,s32Height);
    float scale = matFaceImg.rows / (float)dstImg.rows;

	/*人脸检测*/
	if(m_ptrFaceDetect)
	{
		m_ptrFaceDetect->Run(matFaceImg,dstImg,bRet ? 0 : s32Width,bRet ? 0 : s32Height,vecFaceRect);
	}
	if(vecFaceRect.empty())
	{
		return;
	}
	LOG_DEBUG("face_alg") << string_format("Face Detect size %d\n",vecFaceRect.size());

	/*切图*/
	for(auto& val : vecFaceRect)
	{
		Mat faceResized;
		resize(dstImg(val.bbox.GetRect()), faceResized, Size(roi_size, roi_size));
		vecRoiMat.push_back(faceResized);
		
		//恢复原图的人脸框
		val.bbox.ResizeRect(scale);
	}

	/*人脸定位*/
	vector<Mat> vecLandmark;
	Rect facerect(0, 0, roi_size, roi_size);
	Mat landmark;
	if(m_ptrFaceAlign)
	{
		for(auto& val : vecRoiMat)
		{
			m_ptrFaceAlign->Run(val, facerect,landmark);
			vecLandmark.push_back(landmark);
		}
	}
	if(vecLandmark.empty())
	{
		return;
	}
	LOG_DEBUG("face_alg") << string_format("Face Align size %d\n",vecLandmark.size());
	
	/*人脸特征提取*/
	if(m_ptrFaceIdentify)
	{
		float face_feature[m_s32FeatureDim] = {0.0};
		int&& s32VecSize = vecLandmark.size();
		for(int i = 0;i < s32VecSize;i++)
		{
			vector<Mat> vecSrcImg;
			vecSrcImg.push_back(vecRoiMat[i]);

			vector<Mat> lstLandmark;
			lstLandmark.push_back(vecLandmark[i]);

			m_ptrFaceIdentify->ExtractFeature(vecSrcImg,lstLandmark,face_feature);

			vector<float> vec;
			vec.assign(face_feature,face_feature + m_s32FeatureDim - 1);
			vecFeature.push_back(vec);
		}
	}
}

void CFaceAlg::FaceFeatureExt(const Mat& matFaceImg,vector<string>& vecStrFeature)
{
	vector<vector<float>> vecFloatFeature;
	vector<Mat> vecRoiMat;
	vector<FaceRectInfo> vecFaceRect;
	FaceFeatureExt(matFaceImg,vecFloatFeature,vecRoiMat,vecFaceRect);

	for(auto& vecVal : vecFloatFeature)
	{
		std::stringstream stream;
		for(auto& val : vecVal)
		{
			stream << val << " ";
		}
		vecStrFeature.push_back(stream.str());
	}
}

void CFaceAlg::ProcVideoStream(const string& strIp,const string& strCameCode,const Mat& srcImg)
{
	/*算法是否存在*/
	if(!m_ptrFaceIdentify)
	{
		return;
	}

	/*人脸特征提取*/
	vector<vector<float>> vecFloatFeature;
	vector<Mat> vecRoiMat;
	vector<FaceRectInfo> vecFaceRect;
	FaceFeatureExt(srcImg,vecFloatFeature,vecRoiMat,vecFaceRect);

	/*人脸识别*/
	int&& s32FeatureCount = vecFloatFeature.size();
	if(0 == s32FeatureCount)
	{
		return;
	}
	
	float srcFeature[m_s32FeatureDim] = {0.0};
	float dstFeature[m_s32FeatureDim] = {0.0};
	for(int i = 0;i < s32FeatureCount;i++)
	{
		//拷贝被比对的特征
		copy(vecFloatFeature[i].begin(),vecFloatFeature[i].end(),srcFeature);

		//特征比对
		vector<stFaceIdentify> vecScore;
		CLockType lock(&m_mapFaceFeatureMutex);
		for(auto& mapFeature : m_mapFaceFeature)
		{
			copy(mapFeature.second.begin(),mapFeature.second.end(),dstFeature);

			float&& fScore = m_ptrFaceIdentify->FaceVerify(srcFeature,dstFeature);
			if(fFaceThershold <= fScore)
			{
				stFaceIdentify stIdy;
				stIdy.s32FaceId = mapFeature.first;
				stIdy.fScore = fScore;
				vecScore.push_back(stIdy);
			}
		}
		if(vecScore.empty())
		{
			continue;
		}

		//分数排序
		sort(vecScore.begin(),vecScore.end(),TFaceIdentify());

		//上报识别结果
		{
			CLockType reportLock(&m_mapReportTopicMutex);	
			cv::Rect&& rect = vecFaceRect[i].bbox.GetRect();	
			for(auto& valTopic : m_mapResultReportTopic)
			{
				m_ptrMsgBus->SendReq<void,const std::string&,const cv::Mat&,const cv::Rect&,const int&,const float&>(strIp,srcImg,rect,vecScore[0].s32FaceId,vecScore[0].fScore,valTopic.first);
			}
		}
	}
}