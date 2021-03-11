#include "DetectAlg.h"
#include "log4cxx/Loging.h"
#include "boost/BoostFun.h"
#include "SubTopic.h"
#include "endecode/Base64.h"
#include "comm/CommFun.h"
#include "img/ProcImg.h"

using namespace std;
using namespace common_cmmobj;
using namespace common_template;
using namespace Vision_DetectAlg;

const int s32ConstClasses = 26;

const int s32ConstNetWidth = 640;
const int s32ConstNetHeight = 640;
const int s32ConstImgChannels = 3;
const int s32ConstInputSize = 1;
const int s32ConstOutputSize = 4;

const float fConstIOUThresh = 0.4;   
const float fConstObjThresh = 0.4;             
const float fConstDrawThresh = 0.4;   

void free_detections(detection *dets, int n)
{
	if(!dets)
	{
		return;
	}
	
    for(int i = 0; i < n; ++i)
	{
        free(dets[i].prob);
    }
    free(dets);
}

float overlap(float x1,float w1,float x2,float w2)
{
	float l1=x1-w1/2;
	float l2=x2-w2/2;
	float left=l1>l2? l1:l2;
	float r1=x1+w1/2;
	float r2=x2+w2/2;
	float right=r1<r2? r1:r2;
	return right-left;
}

float box_intersection(box a, box b)
{
    float w = overlap(a.x, a.w, b.x, b.w);
    float h = overlap(a.y, a.h, b.y, b.h);
    if(w < 0 || h < 0) return 0;
    float area = w*h;
    return area;
}

float box_union(box a, box b)
{
    float i = box_intersection(a, b);
    float u = a.w*a.h + b.w*b.h - i;
    return u;
}

float box_iou(box a, box b)
{
    return box_intersection(a, b)/box_union(a, b);
}

int do_nms_sort(detection *dets, int total, int classes, float fIOUThresh, float fDrawThresh)
{
	detection * detbuf = new detection[total];
	int num = 0;
	int i, j;
	//将检测结果归类，取最大值下标表示类别
	for (i = 0; i < total; i++)
	{
		float maxProb = dets[i].prob[0];
		dets[i].sort_class = 0;
		for (j = 1; j < classes; j++)
		{
			if (dets[i].prob[j] > maxProb)
			{
				maxProb = dets[i].prob[j];
				dets[i].sort_class = j;
			}
		}
		if (maxProb > fDrawThresh)
		{
			detbuf[num++] = dets[i];
		}
	}
	
	memset(dets, 0, total * sizeof(detection));
	total = 0;

	for (i = 0; i < num; i ++)
	{
		if (detbuf[i].prob[detbuf[i].sort_class] == 0) 
		{
			continue;
		}
		box a = detbuf[i].bbox;
		for (j = i + 1; j < num; ++j)
		{
			if (detbuf[i].sort_class == detbuf[j].sort_class)  //同类别
			{
				box b = detbuf[j].bbox;
				if (box_iou(a, b) > fIOUThresh)   //去掉iou超过阈值的框，去掉概率小的一个
				{
					if (detbuf[j].prob[detbuf[j].sort_class] < detbuf[i].prob[detbuf[i].sort_class])
					{
						detbuf[j].prob[detbuf[j].sort_class] = 0; //删除第j个框
					}
					else
					{
						detbuf[i].prob[detbuf[i].sort_class] = 0;  //删除第i个框
						break;
					}

				}
			}
		}

		if (detbuf[i].prob[detbuf[i].sort_class] > 0)
		{
			dets[total++] = detbuf[i];   
		}
	}

	delete[] detbuf;
	return total;
}

box get_yolo_box(float x, float y, float w, float h, float *biases, int n,int i, int j, int lw, int lh, int netw, int neth)
{
	box b;
	b.x = (i + x) / lw;
	b.y = (j + y) / lh;
	b.w = (w) * biases[2 * n] / netw;
	b.h = (h) * biases[2 * n + 1] / neth;
	return b;
}

void get_network_boxes(float *predictions, int netw, int neth, int GRID, int *masks, float *anchors, int box_off, int classes, float obj_thresh, detection *dets)
{
	int lw = GRID;    //80
	int lh = GRID;
	int lwh = lw * lh;  //80*80
	int lst_size = 1 + 4 + classes;  //5+26=31
	int index_start = 0;
	int index_end = 0;

	int count = box_off;
	unsigned long long offset = 0;

  	//80*80*3*31
	for (int a = 0; a < nanchor; a++) //3
	{	
		for (int wh = 0; wh < lwh; wh++) //80*80
		{
			int row = wh / lw;
			int col = wh % lw;

			predictions[offset] = ((1. / (1. + exp(-predictions[offset]))) * 2. - 0.5); //x
			predictions[offset + 1] = ((1. / (1. + exp(-predictions[offset + 1]))) * 2. - 0.5); //y

			predictions[offset + 2] = pow((1. / (1. + exp(-predictions[offset + 2]))) * 2., 2); //w
			predictions[offset + 3] = pow((1. / (1. + exp(-predictions[offset + 3]))) * 2., 2);  //h

			predictions[offset + 4] = 1. / (1. + exp(-predictions[offset + 4])); //conf
			for (int c = 0; c < classes; c++)
			{
				predictions[offset + 5 + c] = 1. / (1. + exp(-predictions[offset + 5 + c]));  //prob
			}

			float objectness = predictions[offset + 4];
			if (objectness > obj_thresh)
			{
				dets[count].objectness = objectness;
				dets[count].classes = classes;
				dets[count].bbox = get_yolo_box(predictions[offset], predictions[offset + 1], predictions[offset + 2], predictions[offset + 3],
					anchors, masks[a], col, row, lw, lh, netw, neth);
				for (int c = 0; c < classes; c++)
				{
					dets[count].prob[c] = objectness * predictions[offset + 5 + c];
				}
				++count;
			}

			offset += lst_size;
		}
	}
}

//模型输出转换
static int outputs_transform(rknn_output rknn_outputs[], int net_width, int net_height,int classes,float obj_thresh,float iou_thresh,detection** dets)
{
	*dets = (detection*) calloc(nboxes_total,sizeof(detection));
	for(int i = 0; i < nboxes_total; ++i)
	{
		(*dets)[i].prob = (float*) calloc(classes,sizeof(float));
	}
	
    float *output_0 = (float *)rknn_outputs[1].buf;  //80
	float *output_1 = (float *)rknn_outputs[2].buf;  //40
	float *output_2 = (float *)rknn_outputs[3].buf;  //20*20*3*31
	int masks_0[3] = {0, 1, 2};
	int masks_1[3] = {3, 4, 5};
	int masks_2[3] = {6, 7, 8};
	float anchors[18] = {10, 13, 16, 30, 33, 23, 30, 61, 62, 45, 59, 119, 116, 90, 156, 198, 373, 326};

	//输出xywh均在0-1范围内
	get_network_boxes(output_0, net_width, net_height, GRID0, masks_0, anchors, 0, classes,obj_thresh,*dets);
	get_network_boxes(output_1, net_width, net_height, GRID1, masks_1, anchors, nboxes_0,classes,obj_thresh, *dets);
	get_network_boxes(output_2, net_width, net_height, GRID2, masks_2, anchors, nboxes_0 + nboxes_1,classes,obj_thresh,*dets);
	
    //非极大值抑制
    return  do_nms_sort(*dets, nboxes_total, classes, iou_thresh,fConstDrawThresh);
}

bool CDetectAlg::Init(const MsgBusShrPtr& ptrMsgBus,const Json& taskCfg,const Json& algCfg,const Json& DataSrcCfg)
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
	if(!AlgInit(algCfg))
	{
		return false;
	}

	//数据源配置初始化
	return DataSrcInit(DataSrcCfg);
}

bool CDetectAlg::DataSrcInit(const Json& DataSrcCfg)
{
	//订阅视频流数据
	string strIp = DataSrcCfg[strAttriIp];
	string strTopic = RtspStreamTopic(strIp,DataSrcCfg[strAttriParams][strAttriCode]);
	if(0 == m_mapDataSrcTopic.count(strIp))
	{
		m_mapDataSrcTopic.insert(pair<string,string>(strIp,strTopic));
		auto&& fun = [this](const string& strIp,const string& strCamCode,const cv::Mat& srcImg){ProcMat(strIp,strCamCode,srcImg);};
		m_ptrMsgBus->Attach(move(fun),move(strTopic));
	}

	return true;
}

bool CDetectAlg::AlgInit(const Json& algCfg)
{
	//保存算法模型
	string&& strAlgMapKey = string(algCfg[strAttriType]) + string(algCfg[strAttriIdx]);
	auto ite = m_mapAlgModel.find(strAlgMapKey);
	if(ite == m_mapAlgModel.end())
	{
		rknn_context tx;
		m_mapAlgModel.insert(make_pair<string,rknn_context>(string(strAlgMapKey),move(tx)));
	}
	else
	{
		return true;
	}
	
	// open and load model file
	string strPath = string("./model/") + string(algCfg[strAttriModel][strAttriName]);
	return model_load(strPath,m_mapAlgModel[strAlgMapKey],RKNN_FLAG_PRIOR_MEDIUM);
}

void CDetectAlg::ProcMat(const string& strIp,const string& strCamCode,const cv::Mat& srcImg)
{	
	//原图切片
	vector<cv::Mat> dstImgs;
	vector<cv::Point> transPts;
	ImgToSlices(srcImg, dstImgs, transPts, s32ConstNetWidth);

	//算法检测
	int i = 0;
	for(auto &dstImg : dstImgs)
	{
		//通道颜色转换
		cvtColor(dstImg, dstImg, cv::COLOR_BGR2RGB);	

		//def input
		rknn_input inputs[s32ConstInputSize];
		model_input(inputs,s32ConstNetHeight * s32ConstNetWidth * s32ConstImgChannels,dstImg.data);

		for(auto valModel : m_mapAlgModel)
		{
			//model output
			rknn_output outputs[s32ConstOutputSize];
			int&& ret = model_output(inputs,valModel.second,outputs,s32ConstOutputSize);
			if(0 != ret)
			{
				continue;
			}

			//output transfor
			detection* dets = nullptr;	
			int nboxes_left = outputs_transform(outputs, s32ConstNetWidth, s32ConstNetHeight,s32ConstClasses,fConstObjThresh,fConstIOUThresh,&dets);
			if(0 < nboxes_left && nullptr != dets)
			{
				if(i > 0)
				{
					for(int k = 0; k < nboxes_left; k ++)
					{
						box inBox = dets[k].bbox;
						GetOriginRect(inBox, transPts[i-1], s32ConstNetWidth, srcImg.cols, dets[k].bbox);
					}
				}
				
				CLockType lock(&m_mapReportTopicMutex);
				for(auto& valTopic : m_mapResultReportTopic)
				{
					m_ptrMsgBus->SendReq<void,const std::string&,const cv::Mat&,const detection*,const int&,const int&>(strIp,srcImg,dets,nboxes_left,s32ConstClasses,valTopic.first);
				}
			}

			//release resource
			rknn_outputs_release(valModel.second, s32ConstOutputSize,outputs);
			free_detections(dets,nboxes_total);
		}
		i++;
	}
}

void CDetectAlg::Release()
{
	for(auto& val : m_mapAlgModel)
	{
		rknn_destroy(val.second);
	}
}