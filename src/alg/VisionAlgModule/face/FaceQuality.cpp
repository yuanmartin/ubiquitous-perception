#include "FaceQuality.h"
#include <opencv2/imgproc/types_c.h>

using namespace std;
using namespace Vision_FaceAlg;

FaceQuality::FaceQuality()
{
    srand((unsigned)time(NULL));
}

FaceQuality::~FaceQuality()
{
}

//----------------------------------------
// 质量判定函数
// input:  input_image 输入图像，
//         landmarks 人脸关键点，68个点，排列顺序为x1,y1,x2,y2,...类型CV_32FC1
//         face_rect 人脸检测框（框的位置和大小）
//         camera_mode 摄像头模式，0为可见光，1为近红外
// output: quality_score 图像的质量分值
// return: 0 正常 -1 输入图像为空
//----------------------------------------
int FaceQuality::face_quality_final(cv::Mat input_image, cv::Mat landmarks,cv::Rect face_rect, int camera_mode, float &quality_score)
{
    if (input_image.empty()) 
    {
        return -1;
    }

    // 对输入图像根据输入的68个关键点进行裁剪
    cv::Mat crop_img, crop_img2;
    FaceCrop(input_image, landmarks, &crop_img, &crop_img2);
    cv::Mat crop_gray;
    if (crop_img.channels() == 3) 
    {
        cvtColor(crop_img, crop_gray, CV_BGR2GRAY);
    }
    else 
    {
        crop_img.copyTo(crop_gray);
    }

    cv::Mat crop2_gray;
    if (crop_img2.channels() == 3) 
    {
        cvtColor(crop_img2, crop2_gray, CV_BGR2GRAY);
    }
    else 
    {
        crop_img2.copyTo(crop2_gray);
    }

    // 首先进行亮度异常检测
    float ave_gray = 0.0, weight_diff = 0.0;
    brightnessException(crop2_gray, ave_gray, weight_diff);

    // 对红外图像
    if (camera_mode == 1) 
    {
        float ssim_thresh = 24.0;
        // 亮度正常，计算ssim的值
        // 对裁剪之后的图像进行运动模糊
        cv::Mat blur_gray(crop_img.size(), crop_img.type());
        MotionReblur(crop_gray, blur_gray);
        // 对图像进行质量判定
        float ssim_score = 0.0; // 本摄像头数据在0~45之间，需要进行调整
        GetMSSIM(crop_gray, blur_gray, ssim_score);
        if (ssim_score >= ssim_thresh) 
        {
            // 大于阈值为清晰图像，将分值映射到60~95之间
            quality_score = float(35) / 21 * (ssim_score - 24.0) + 60.0;
        }
        else 
        {
            // 小于阈值为模糊图像，将分值映射到30~60之间
            quality_score = float(30) / 24 * (ssim_score - 0.0) + 30.0;
        }

        if (quality_score >= 95.0) 
        {
            quality_score = 94.50;
        }

        if (quality_score < 30.0) 
        {
            quality_score = 35.50;
        }
    }

    // 对可见光图像
    if (camera_mode == 0) 
    {
        float ave_gray_thresh1 = -80.0, ave_gray_thresh2 = 95.0;
        float ssim_thresh = 28.0;
        // 亮度异常
        //使用time()获取不重复的种子
        if (ave_gray < ave_gray_thresh1) 
        {
            //dark,随机30~45之间的值
            quality_score = (rand() / (float)(RAND_MAX)) * (45 - 30) + 30;
        }
        else if (ave_gray > ave_gray_thresh2) 
        {
            //bright,随机35~55之间的值
            quality_score = (rand() / (float)(RAND_MAX)) * (55 - 35) + 35;
        }
        else 
        {
            // 亮度正常，计算ssim的值
            // 对裁剪之后的图像进行运动模糊
            cv::Mat blur_gray(crop_img.size(), crop_img.type());
            MotionReblur(crop_gray, blur_gray);

            // 对图像进行质量判定
            float ssim_score = 0.0; // 本摄像头数据在0~60之间，需要进行调整
            GetMSSIM(crop_gray, blur_gray, ssim_score);
            if (ssim_score >= ssim_thresh) 
            {
                // 大于阈值为清晰图像，将分值映射到60~95之间
                quality_score = float(35) / 25 * (ssim_score - 35.0) + 60.0;
            }
            else 
            {
                // 小于阈值为模糊图像，将分值映射到30~60之间
                quality_score = float(30) / 35 * (ssim_score - 0.0) + 30.0;
            }
            if (quality_score >= 95.0) 
            {
                quality_score = 94.50;
            }

            if (quality_score < 30.0) 
            {
                quality_score = 35.50;
            }
        }
    }
    return 0;
}

//----------------------------------------------------------
// 图像裁剪函数
// input: src_image，RGB图像
//        absolute_points，Mat类型, CV_32FC1，关键点在原图上的坐标
//        坐标顺序：x1,y1,x2,y2,...,x68,y68
// output: face_region,最小外接矩形扩充之后resize(用于ssim)
//         face_region2,最小外接矩形resize(用于bright)
// return: 0正常，-1非正常
//-----------------------------------------------------------
int FaceQuality::FaceCrop(cv::Mat &input_image,cv::Mat &absolute_points,cv::Mat *face_region,cv::Mat *face_region2)
{
    float x_left = 10000.0;
    float x_right = -1.0;
    float y_top = 10000.0;
    float y_bottom = -1.0;
    float *point_data = absolute_points.ptr<float>(0);
    for (int i = 0; i < 68; ++i)
    {
        float x_temp = point_data[2 * i];
        float y_temp = point_data[2 * i + 1];
 
        x_left = ((x_left) < (x_temp)) ? (x_left) : (x_temp);
        x_right = ((x_right) > (x_temp)) ? (x_right) : (x_temp);
        y_top = ((y_top) < (y_temp)) ? (y_top) : (y_temp);
        y_bottom = ((y_bottom) > (y_temp)) ? (y_bottom) : (y_temp);
    }

    // 根据关键点进行裁剪：68个关键点的最小外接矩形向上向左扩充0.4*h,0.2*w
    // 最终得到1.4w*1.6h的人脸框
    float rect_width = x_right - x_left, rect_height = y_bottom - y_top;
    float rect_x = x_left - 0.2 * rect_width;
    float rect_y = y_top - 0.4 * rect_height;
    rect_width = 1.4 * rect_width;
    rect_height = 1.6 * rect_height;
    if (rect_x < 0)
    {
        rect_x = 0;
    }

    if (rect_y < 0)
    {
        rect_y = 0;
    }

    if (rect_width > (input_image.cols - 1 - rect_x))
    {
        rect_width = input_image.cols - 1 - rect_x;
    }
    if (rect_height > (input_image.rows - 1 - rect_y))
    {
        rect_height = input_image.rows - 1 - rect_y;
    }

    if(rect_width < 5 ||  rect_height < 5)
    {
        return -1;
    }
    
    // 按最小外接矩形扩充裁剪之后resize
    cv::Rect face_rect(rect_x, rect_y, rect_width, rect_height);
    cv::Mat crop = input_image(face_rect);
    cv::Mat crop_resize;
    cv::resize(crop, crop_resize, cv::Size(100, 110));
    crop_resize.copyTo(*face_region);

    // 根据关键点进行裁剪：68个关键点的最小外接矩形
    float rect_width2 = x_right - x_left, rect_height2 = y_bottom - y_top;
    float rect_x2 = x_left;
    float rect_y2 = y_top;
    if (rect_x2 < 0)
    {
        rect_x2 = 0;
    }

    if (rect_y2 < 0)
    {
        rect_y2 = 0;
    }

    if (rect_width2 > (input_image.cols - 1 - rect_x2))
    {
        rect_width2 = input_image.cols - 1 - rect_x2;
    }

    if (rect_height2 > (input_image.rows - 1 - rect_y2))
    {
        rect_height2 = input_image.rows - 1 - rect_y2;
    }

    if(rect_width2 < 5 ||  rect_height2 < 5)
    {
        return -1;
    }

    // 按最小外接矩形裁剪之后resize
    cv::Rect face_rect2(rect_x2, rect_y2, rect_width2, rect_height2);
    cv::Mat crop2 = input_image(face_rect2);
    cv::Mat crop_resize2;
    cv::resize(crop2, crop_resize2, cv::Size(100, 110));
    crop_resize2.copyTo(*face_region2);
    return 0;
}

//-----------------------------------------------
// 对图像进行运动模糊
// input: src_img 输入图像
// output: blur_img 输出运动模糊图像
// return 0 正常 -1 异常
//--------------------------------------------------
int FaceQuality::MotionReblur(cv::Mat srcImg, cv::Mat &blurImg)
{
    double len = 20, angle = 0.1;
    double EPS = std::numeric_limits<double>::epsilon();
    double half = len / 2;
    double alpha = (angle - floor(angle / 180) * 180) / 180 * 3.14159265;
    double cosalpha = cos(alpha);
    double sinalpha = sin(alpha);
    int xsign;
    if (cosalpha < 0)
    {
        xsign = -1;
    }
    else
    {
        if (angle == 90)
        {
            xsign = 0;
        }
        else
        {
            xsign = 1;
        }
    }
    int psfwdt = 1;
    int sx = (int)fabs(half*cosalpha + psfwdt*xsign - len*EPS);
    int sy = (int)fabs(half*sinalpha + psfwdt - len*EPS);
    cv::Mat_<double> psf1(sy, sx, CV_64F);
    cv::Mat_<double> psf2(sy * 2, sx * 2, CV_64F);
    int row = 2 * sy;
    int col = 2 * sx;

    /*为减小运算量，先计算一半大小的PSF*/
    for (int i = 0; i < sy; i++)
    {
        double* pvalue = psf1.ptr<double>(i);
        for (int j = 0; j < sx; j++)
        {
            pvalue[j] = i*fabs(cosalpha) - j*sinalpha;
            double rad = sqrt(i*i + j*j);
            if (rad >= half && fabs(pvalue[j]) <= psfwdt)
            {
                double temp = half - fabs((j + pvalue[j] * sinalpha) / cosalpha);
                pvalue[j] = sqrt(pvalue[j] * pvalue[j] + temp*temp);
            }
            pvalue[j] = psfwdt + EPS - fabs(pvalue[j]);
            if (pvalue[j] < 0)
            {
                pvalue[j] = 0;
            }
        }
    }

    /*将模糊核矩阵扩展至实际大小*/
    for (int i = 0; i < sy; i++)
    {
        double* pvalue1 = psf1.ptr<double>(i);
        double* pvalue2 = psf2.ptr<double>(i);
        for (int j = 0; j < sx; j++)
        {
            pvalue2[j] = pvalue1[j];
        }
    }
    for (int i = 0; i < sy; i++)
    {
        for (int j = 0; j < sx; j++)
        {
            psf2[2 * sy - 1 - i][2 * sx - 1 - j] = psf1[i][j];
            psf2[sy + i][j] = 0;
            psf2[i][sx + j] = 0;
        }
    }
    /*保持图像总能量不变，归一化矩阵*/
    double sum = 0;
    for (int i = 0; i < row; i++)
    {
    for (int j = 0; j < col; j++)
    {
        sum += psf2[i][j];
    }
    }
    psf2 = psf2 / sum;
    if (cosalpha > 0)
    {
    flip(psf2, psf2, 0);
    }
    
    cv::Mat psf = psf2;
    cv::filter2D(srcImg, blurImg, srcImg.depth(), psf);
    return 0;
}

//-------------------------------------
// 计算mssim值
// input: i1,i2分别为参考图像和待评价图像，灰度图
// output: quality_score mssim的计算值
//---------------------------------
void FaceQuality::GetMSSIM(const cv::Mat& i1, const cv::Mat& i2, float &quality_score)
{
    const double C1 = 6.5025, C2 = 58.5225;
    /* INITS **/
    int d = CV_32F;
    cv::Mat I1, I2;
    i1.convertTo(I1, d); // cannot calculate on one byte large values
    i2.convertTo(I2, d);
    cv::Mat I2_2 = I2.mul(I2); // I2^2
    cv::Mat I1_2 = I1.mul(I1); // I1^2
    cv::Mat I1_I2 = I1.mul(I2); // I1 * I2

    /*PRELIMINARY COMPUTING **/
    cv::Mat mu1, mu2; //
    GaussianBlur(I1, mu1, cv::Size(11, 11), 1.5);
    GaussianBlur(I2, mu2, cv::Size(11, 11), 1.5);
    cv::Mat mu1_2 = mu1.mul(mu1);
    cv::Mat mu2_2 = mu2.mul(mu2);
    cv::Mat mu1_mu2 = mu1.mul(mu2);
    cv::Mat sigma1_2, sigma2_2, sigma12;
    GaussianBlur(I1_2, sigma1_2, cv::Size(11, 11), 1.5);
    sigma1_2 -= mu1_2;
    GaussianBlur(I2_2, sigma2_2, cv::Size(11, 11), 1.5);
    sigma2_2 -= mu2_2;
    GaussianBlur(I1_I2, sigma12, cv::Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;

    ///////////////////////////////// FORMULA ////////////////////////////////
    cv::Mat t1, t2, t3;
    t1 = 2 * mu1_mu2 + C1;
    t2 = 2 * sigma12 + C2;
    t3 = t1.mul(t2); // t3 = ((2mu1_mu2 + C1).(2*sigma12 + C2))
    t1 = mu1_2 + mu2_2 + C1;
    t2 = sigma1_2 + sigma2_2 + C2;
    t1 = t1.mul(t2); // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))
    cv::Mat ssim_map;
    divide(t3, t1, ssim_map); // ssim_map = t3./t1;
    cv::Scalar mssim = mean(ssim_map); // mssim = average of ssim map
    float ssim_score = mssim[0];
    quality_score = 100.0*(1.0 - ssim_score);
}

// 图像亮度异常检测
// input: InputImg 输入图像（已裁剪人脸）,灰度图像
// output: ave_gray 灰度均值，weight_diff 加权偏差
//------------------------------------------
void FaceQuality::brightnessException(cv::Mat gray_img,float& ave_gray, float& weight_diff)
{
    float a = 0;
    int Hist[256];
    for (int i = 0; i < 256; i++)
    Hist[i] = 0;
   
    int len = gray_img.rows*gray_img.cols;
    {
        uchar* pData = (uchar*)gray_img.data;
        for (int i = 0; i < len; ++i)
        {
            a += *pData - 128;//在计算过程中，考虑128为亮度均值点 
            int x = *pData;
            pData++;
            Hist[x]++;
        }
    }

    ave_gray = (float)a / len;
    float D = abs(ave_gray);
    float Ma = 0.0;
    for (int i = 0; i < 256; i++)
    {
        Ma += abs(i - 128 - ave_gray)*Hist[i];
    }
    Ma /= float(len);
    float M = abs(Ma);
    float K = D / M;
    weight_diff = K;
    return;
}