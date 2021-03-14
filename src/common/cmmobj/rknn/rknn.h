#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <functional>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/videoio.hpp"

#include "rknn/rknn_api.h"
#include "boost/BoostFun.h"
#include "comm/CommDefine.h"

const int GRID0 = 80;
const int GRID1 = 40;
const int GRID2 = 20;

const int nanchor = 3;                        
const int nboxes_0 = GRID0 * GRID0 * nanchor;
const int nboxes_1 = GRID1 * GRID1 * nanchor;
const int nboxes_2 = GRID2 * GRID2 * nanchor;
const int nboxes_total = nboxes_0 + nboxes_1 + nboxes_2;

#pragma pack(push, 1)  
    class box
    {
	public:
		box(){}
		~box(){}
		box(const box& obj)
		{
			copy(obj);
		}

		box& operator=(const box& obj)
		{
			copy(obj);
		}
		float x,y,w,h;
		
	private:
		void copy(const box& obj)
		{
			x = obj.x;
			y = obj.y;
			w = obj.w;
			h = obj.h;
		}
    	
    };

    class detection
    {
	public:
		detection(int s32Classes) : 
		classes(s32Classes),
		sort_class(0),
		objectness(0.0),
		prob(nullptr)
		{
			new_prob();
		}

		detection(const detection& obj)
		{
			copy(obj);
		}

		detection& operator=(const detection& obj)
		{
			copy(obj);
		}

		~detection()
		{
			delete_prob();
		}
		
		box bbox;
        int classes;
		int sort_class;
        float objectness; 
		float *prob;

	private:
		void copy(const detection& obj)
		{
			bbox = obj.bbox;
			classes = obj.classes;
			sort_class = obj.sort_class;
			objectness = obj.objectness;
			copy_prob(obj);
		}

		void new_prob()
		{
			if(0 >= classes)
			{
				classes = 1;
			}
			prob = new float[classes];
			memset((char*)prob,0,classes * nFloatLen);
		}

		void copy_prob(const detection& obj)
		{
			new_prob();
			memcpy((char*)prob,(char*)obj.prob,classes * nFloatLen);
		}

		void delete_prob()
		{
			delete [] prob;
		}
    };
#pragma pack(pop)

/*模型文件加载*/
const int s32AlgMaxWaitTime = 2;
static bool model_load(const std::string& strPath,rknn_context& rkContext,const int output_size)
{
	FILE *fp = fopen(strPath.c_str(), "rb");
	if(!fp) 
	{
		return false;
	}

	fseek(fp, 0, SEEK_END);   
	int model_len = ftell(fp);   
	void *model = malloc(model_len);
	fseek(fp, 0, SEEK_SET);   
	if(model_len != fread(model, 1, model_len, fp)) 
	{
		free(model);
		fclose(fp);
		return false;
	}
	fclose(fp);

	//init model
	int ret = rknn_init(&rkContext,model,model_len,RKNN_FLAG_PRIOR_MEDIUM);
	if(ret < 0) 
	{
		std::cout << "Alg : " << strPath << " rkn_init fail" << std::endl;
		free(model);
		return false;
    }
	std::cout << common_template::string_format("Alg : %s rknn_init successful ret : %d and m_tx : %d\n",strPath.c_str(),ret,rkContext) << std::endl;
	return true;

	//rknn query
	if(0 == output_size)
	{
		return true;
	}

	rknn_tensor_attr outputs_attr[output_size];
	outputs_attr[0].index = 0;
	ret = rknn_query(rkContext, RKNN_QUERY_OUTPUT_ATTR, &(outputs_attr[0]), sizeof(outputs_attr[0]));
	if (ret < 0)
	{
		std::cout << common_template::string_format("rknn_query fail! ret ： %d\n", ret) << std::endl;
		free(model);
		return false;
	}
	std::cout << common_template::string_format("rknn_query successful ret : %d\n",ret) << std::endl;

	return true;
}

//模型输入
static void model_input(rknn_input inputs[],const int& input_size,unsigned char* input_data)
{
	inputs[0].index = 0;
	inputs[0].size = input_size;
	inputs[0].pass_through = false;         
	inputs[0].type = RKNN_TENSOR_UINT8;
	inputs[0].fmt = RKNN_TENSOR_NHWC;
	inputs[0].buf = input_data;
}

//模型输出
static int model_output(rknn_input inputs[],const rknn_context& context,rknn_output outputs[],const int& output_size)
{
	//query
	rknn_tensor_attr outputs_attr[output_size];
	outputs_attr[0].index = 0;
	int ret = rknn_query(context, RKNN_QUERY_OUTPUT_ATTR, &(outputs_attr[0]), sizeof(outputs_attr[0]));
	if (ret < 0)
	{
		std::cout << common_template::string_format("rknn_query fail! ret=%d\n", ret) << std::endl;
		return -1;
	}

	//img input
	ret = rknn_inputs_set(context, 1, inputs);
	if (ret < 0) 
	{
		std::cout << common_template::string_format("rknn_inputs_set fail! ret=%d\n", ret) << std::endl;
		return -1;
	}
	
	//rknn run
	ret = rknn_run(context, nullptr);
	if (ret < 0) 
	{
		std::cout << common_template::string_format("rknn_run fail! ret=%d\n", ret) << std::endl;
		return -1;
	}

	//rknn outputs
	for(int i = 0;i < output_size;i++)
	{
		outputs[i].want_float = true;
		outputs[i].is_prealloc = false;
	}

	ret = rknn_outputs_get(context, output_size, outputs, NULL); 
	if (ret < 0)
	{
		std::cout << common_template::string_format("rknn_outputs_get fail! ret=%d\n", ret) << std::endl;
		return -1;
	}
	return 0;
}