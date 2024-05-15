#include "stdio.h"
#include <time.h>
#include "ASICamera2.h"
#include "opencv2/highgui/highgui_c.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <thread>
#include <mutex>
#include <filesystem>
#include "coding_functions.h"

#define MAX_CONTROL 7
#define FPS 20
#define HEIGHT 2160
#define WIDTH 3840
#define SIZE (HEIGHT*WIDTH*3+HEIGHT)
constexpr int CALLBACK_PERIOD = 1000/FPS;

extern unsigned long GetTickCount();

int bDisplay = 0;
int bMain = 1;
int bChangeFormat = 0;
int bSendTiggerSignal = 0;
ASI_CAMERA_INFO CamInfo;

enum CHANGE{
    change_imagetype = 0,
    change_bin,
    change_size_bigger,
    change_size_smaller
};

CHANGE change;

//zwo FUNCTIONS
void cvText(IplImage* img, const char* text, int x, int y)
{
    CvFont font;

    double hscale = 0.6;
    double vscale = 0.6;
    int linewidth = 2;
    cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC,hscale,vscale,0,linewidth);

    CvScalar textColor =cvScalar(255,255,255);
    CvPoint textPos =cvPoint(x, y);

    cvPutText(img, text, textPos, &font,textColor);
}

void* Display(void* params)
{
    IplImage *pImg = (IplImage *)params;
    cvNamedWindow("video", 1);
    while(bDisplay)
    {
        // show image on screen
        cvShowImage("video", pImg);

        // push image to buffer
        //g_async_queue_push(queue, pImg);

        /*
         * Commands:
         * esc - stop
         * i - change image type
         * b - change bin
         * w/s - make image smaller or bigger
         * t - trigger
         * */

        char c=cvWaitKey(1);
        switch(c)
        {
            case 27://esc
                bDisplay = false;
                bMain = false;
                goto END;

            case 'i'://space
                bChangeFormat = true;
                change = change_imagetype;
                break;

            case 'b'://space
                bChangeFormat = true;
                change = change_bin;
                break;

            case 'w'://space
                bChangeFormat = true;
                change = change_size_smaller;
                break;

            case 's'://space
                bChangeFormat = true;
                change = change_size_bigger;
                break;

            case 't'://triiger
                bSendTiggerSignal = true;
                break;
        }
    }
    END:
    cvDestroyWindow("video");
    printf("Display thread over\n");
    ASIStopVideoCapture(CamInfo.CameraID);
    return (void*)0;
}

std::string saveImage(const cv::Mat& image, int count) {
    std::string folder = "DNG";
    if (!std::filesystem::exists(folder)) {
        std::filesystem::create_directory(folder);
    }
    std::string filename = folder + "/" + std::to_string(count) + ".dng";
    cv::imwrite(filename, image);
    return filename;
}

int  main(int argc, char *argv[])
{
    int width;
    const char* bayer[] = {"RG","BG","GR","GB"};
    const char* controls[MAX_CONTROL] = {"Exposure", "Gain", "Gamma", "WB_R", "WB_B", "Brightness", "USB Traffic"};

    int height;
    int i;
    char c;
    bool bresult;
    int modeIndex;

    int time1,time2;
    int count=0;

    char buf[128]={0};

    int CamIndex=0;
    int inputformat;
    int definedformat;


    IplImage *pRgb;


    int numDevices = ASIGetNumOfConnectedCameras();
    if(numDevices <= 0)
    {
        printf("no camera connected, press any key to exit\n");
        getchar();
        return -1;
    }
    else
        printf("attached cameras:\n");

    for(i = 0; i < numDevices; i++)
    {
        ASIGetCameraProperty(&CamInfo, i);
        printf("%d %s\n",i, CamInfo.Name);
    }

    printf("\nselect one to privew\n");
    scanf("%d", &CamIndex);


    ASIGetCameraProperty(&CamInfo, CamIndex);
    bresult = ASIOpenCamera(CamInfo.CameraID);
    bresult += ASIInitCamera(CamInfo.CameraID);
    if(bresult)
    {
        printf("OpenCamera error,are you root?,press any key to exit\n");
        getchar();
        return -1;
    }

    printf("%s information\n",CamInfo.Name);
    int iMaxWidth, iMaxHeight;
    iMaxWidth = CamInfo.MaxWidth;
    iMaxHeight =  CamInfo.MaxHeight;
    printf("resolution:%dX%d\n", iMaxWidth, iMaxHeight);
    if(CamInfo.IsColorCam)
        printf("Color Camera: bayer pattern:%s\n",bayer[CamInfo.BayerPattern]);
    else
        printf("Mono camera\n");

    int ctrlnum;
    ASIGetNumOfControls(CamInfo.CameraID, &ctrlnum);
    ASI_CONTROL_CAPS ctrlcap;
    for( i = 0; i < ctrlnum; i++)
    {
        ASIGetControlCaps(CamInfo.CameraID, i,&ctrlcap);

        printf("%s\n", ctrlcap.Name);

    }

    int bin = 1, Image_type;
    printf("Use customer format or predefined format resolution?\n 0:customer format \n 1:predefined format\n");
    scanf("%d", &inputformat);
    if(inputformat)
    {
        printf("0:Size %d X %d, BIN 1, ImgType raw8 (note: crashes)\n", iMaxWidth/2, iMaxHeight/2);
        printf("1:Size %d X %d, BIN 1, ImgType raw16 (note: drops all frames)\n", iMaxWidth/2, iMaxHeight/2);
        printf("2:Size 3840 X 2160, BIN 1, ImgType RGB24 (42 fps @10ms)\n");
        printf("3:Size 2560 X 1440, BIN 1, ImgType RGB24 (61 fps @10ms)\n");
        printf("4:Size 1920 X 1080, BIN 1, ImgType RGB24 (71 fps @10ms)\n");
        printf("5:Size 1024 X 720, BIN 1, ImgType raw16 (100 fps @10ms)\n");
        printf("6:Size 1024 X 720, BIN 2, ImgType RGB24 (50 fps @10ms)\n");
        scanf("%d", &definedformat);
        if(definedformat == 0)
        {
            ASISetROIFormat(CamInfo.CameraID, iMaxWidth/2, iMaxHeight/2, 1, ASI_IMG_RAW8);
            width = iMaxWidth/2;
            height = iMaxHeight/2;
            bin = 1;
            Image_type = ASI_IMG_RAW8;
        }
        else if(definedformat == 1)
        {
            ASISetROIFormat(CamInfo.CameraID, iMaxWidth/2, iMaxHeight/2, 1, ASI_IMG_RAW16);
            width = iMaxWidth/2;
            height = iMaxHeight/2;
            bin = 1;
            Image_type = ASI_IMG_RAW16;
        }
        else if(definedformat == 2)
        {
            ASISetROIFormat(CamInfo.CameraID, 3840, 2160, 1, ASI_IMG_RGB24);
            width = 3840;
            height = 2160;
            bin = 1;
            Image_type = ASI_IMG_RGB24;
        }
        else if(definedformat == 3)
        {
            ASISetROIFormat(CamInfo.CameraID, 2560, 1440, 1, ASI_IMG_RGB24);
            width = 2560;
            height = 1440;
            bin = 1;
            Image_type = ASI_IMG_RGB24;
        }
        else if(definedformat == 4)
        {
            ASISetROIFormat(CamInfo.CameraID, 1920, 1080, 1, ASI_IMG_RGB24);
            width = 1920;
            height = 1080;
            bin = 1;
            Image_type = ASI_IMG_RGB24;
        }
        else if(definedformat == 5)
        {
            ASISetROIFormat(CamInfo.CameraID, 1024, 720, 1, ASI_IMG_RAW16);
            width = 1024;
            height = 720;
            bin = 1;
            Image_type = ASI_IMG_RAW16;
        }
        else if(definedformat == 6)
        {
            ASISetROIFormat(CamInfo.CameraID, 1024, 720, 2, ASI_IMG_RGB24);
            width = 1024;
            height = 720;
            bin = 2;
            Image_type = ASI_IMG_RGB24;

        }
        else
        {
            printf("Wrong input! Will use the resolution0 as default.\n");
            ASISetROIFormat(CamInfo.CameraID, iMaxWidth, iMaxHeight, 1, ASI_IMG_RAW8);
            width = iMaxWidth;
            height = iMaxHeight;
            bin = 1;
            Image_type = ASI_IMG_RAW8;
        }

    }
    else
    {
        printf("\nPlease input the <width height bin image_type> with one space\n"
               "supported bin modes are '1' (bin1) and '2' (bin2)\n"
               "supported image types are %d (ASI_IMG_RAW8), %d (ASI_IMG_RGB24) and %d (ASI_IMG_RAW16)\n"
               "ie. 1024 720 1 1 10ms (100 fps), 1024 720 2 1 10ms (50 fps), 1920 1080 1 1 10 ms - 71 fps\n"
               "use max resolution (5496 X 3672) if input is 0\n"
               "Press ESC when video window is focused to quit capture\n", ASI_IMG_RAW8, ASI_IMG_RGB24, ASI_IMG_RAW16);
        scanf("%d %d %d %d", &width, &height, &bin, &Image_type);
        if(width == 0 || height == 0)
        {
            width = iMaxWidth;
            height = iMaxHeight;
        }


        while(ASISetROIFormat(CamInfo.CameraID, width, height, bin, (ASI_IMG_TYPE)Image_type))//IMG_RAW8
        {
            printf("Set format error, please check the width and height\n ASI120's data size(width*height) must be integer multiple of 1024\n");
            printf("Please input the width and height again, ie. 640 480\n");
            scanf("%d %d %d %d", &width, &height, &bin, &Image_type);
        }
        printf("\nset image format %d %d %d %d success, start privew, press ESC to stop \n", width, height, bin, Image_type);
    }

    if(Image_type == ASI_IMG_RAW16)
        pRgb=cvCreateImage(cvSize(width,height), IPL_DEPTH_16U, 1);
    else if(Image_type == ASI_IMG_RGB24)
        pRgb=cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, 3);
    else
        pRgb=cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, 1);

    int exp_ms;
    printf("Please input exposure time(ms):\n"
           "1 ms - sunny windows\n"
           "10 ms - indoor lighting (recommended)\n"
           "50 ms - big pinhole\n"
           "300 ms - small pinhole\n");
    scanf("%d", &exp_ms);


    int piNumberOfControls = 0;
    ASI_ERROR_CODE errCode = ASIGetNumOfControls(CamInfo.CameraID, &piNumberOfControls);

    ASI_CONTROL_CAPS pCtrlCaps;
    int transferSpeed = -1;
    for(int j = 0; j < piNumberOfControls; j++)
    {
        ASIGetControlCaps(CamInfo.CameraID, j, &pCtrlCaps);
        if (pCtrlCaps.ControlType == ASI_BANDWIDTHOVERLOAD)
        {
            printf("setting ASI_BANDWIDTHOVERLOAD to pCtrlCaps.MaxValue=%ld\n", pCtrlCaps.MaxValue);
            transferSpeed = pCtrlCaps.MaxValue;
            break;
        }
    }

    if (transferSpeed == -1)
    {
        printf("Couldn't find ASI_BANDWIDTHOVERLOAD MaxValue. Setting to 40\n");
        transferSpeed = 40; //default
    }

    ASISetControlValue(CamInfo.CameraID,ASI_EXPOSURE, exp_ms*1000, ASI_FALSE);
    ASISetControlValue(CamInfo.CameraID,ASI_GAIN,0, ASI_FALSE);
    ASISetControlValue(CamInfo.CameraID,ASI_BANDWIDTHOVERLOAD, transferSpeed, ASI_FALSE); //low transfer speed
    ASISetControlValue(CamInfo.CameraID,ASI_HIGH_SPEED_MODE, 1, ASI_FALSE);
    ASISetControlValue(CamInfo.CameraID,ASI_WB_B, 90, ASI_FALSE);
    ASISetControlValue(CamInfo.CameraID,ASI_WB_R, 48, ASI_TRUE);

    ASI_SUPPORTED_MODE cammode;
    ASI_CAMERA_MODE mode;
    if(CamInfo.IsTriggerCam)
    {
        i = 0;
        printf("This is multi mode camera, you need to select the camera mode:\n");
        ASIGetCameraSupportMode(CamInfo.CameraID, &cammode);
        while(cammode.SupportedCameraMode[i]!= ASI_MODE_END)
        {
            if(cammode.SupportedCameraMode[i]==ASI_MODE_NORMAL)
                printf("%d:Normal Mode\n", i);
            if(cammode.SupportedCameraMode[i]==ASI_MODE_TRIG_SOFT_EDGE)
                printf("%d:Trigger Soft Edge Mode\n", i);
            if(cammode.SupportedCameraMode[i]==ASI_MODE_TRIG_RISE_EDGE)
                printf("%d:Trigger Rise Edge Mode\n", i);
            if(cammode.SupportedCameraMode[i]==ASI_MODE_TRIG_FALL_EDGE)
                printf("%d:Trigger Fall Edge Mode\n", i);
            if(cammode.SupportedCameraMode[i]==ASI_MODE_TRIG_SOFT_LEVEL)
                printf("%d:Trigger Soft Level Mode\n", i);
            if(cammode.SupportedCameraMode[i]==ASI_MODE_TRIG_HIGH_LEVEL)
                printf("%d:Trigger High Level Mode\n", i);
            if(cammode.SupportedCameraMode[i]==ASI_MODE_TRIG_LOW_LEVEL)
                printf("%d:Trigger Low  Lovel Mode\n", i);

            i++;
        }

        scanf("%d", &modeIndex);
        ASISetCameraMode(CamInfo.CameraID, cammode.SupportedCameraMode[modeIndex]);
        ASIGetCameraMode(CamInfo.CameraID, &mode);
        if(mode != cammode.SupportedCameraMode[modeIndex])
            printf("Set mode failed!\n");

    }

    ASIStartVideoCapture(CamInfo.CameraID); //start privew

    printf("controls:\n"
           "         esc - stop\n"
           "         i - change image type\n"
           "         b - change bin\n"
           "         w/s - make image smaller or bigger\n"
           "         t - trigger\n");

    long lVal;
    ASI_BOOL bAuto;
    ASIGetControlValue(CamInfo.CameraID, ASI_TEMPERATURE, &lVal, &bAuto);
    printf("sensor temperature:%.1f\n", lVal/10.0);

    time1 = GetTickCount();
    int iStrLen = 0, iTextX = 40, iTextY = 60;
    void* retval;


    int iDropFrmae;
    while(bMain)
    {

        if(mode == ASI_MODE_NORMAL)
        {
            if(ASIGetVideoData(CamInfo.CameraID, (unsigned char*)pRgb->imageData, pRgb->imageSize, 500) == ASI_SUCCESS)
                count++;
        }
        else
        {
            if(ASIGetVideoData(CamInfo.CameraID, (unsigned char*)pRgb->imageData, pRgb->imageSize, 1000) == ASI_SUCCESS)
                count++;
        }

        time2 = GetTickCount();



        if(time2-time1 > 1000 )
        {
            ASIGetDroppedFrames(CamInfo.CameraID, &iDropFrmae);
            sprintf(buf, "fps:%d dropped frames:%d ImageType:%d",count, iDropFrmae, (int)Image_type);

            count = 0;
            time1=GetTickCount();
            printf(buf);
            printf("\n");

        }
        if(Image_type != ASI_IMG_RGB24 && Image_type != ASI_IMG_RAW16)
        {
            if (mode == ASI_MODE_NORMAL) {
                if (ASIGetVideoData(CamInfo.CameraID, (unsigned char*)pRgb->imageData, pRgb->imageSize, 500) == ASI_SUCCESS) {
                    count++;
                    std::string input_folder = saveImage(cv::cvarrToMat(pRgb), count);
                    std::string yuv_output;
                    std::string final_yuv_output;
                    convert_dng_to_yuv(cv::cvarrToMat(pRgb), yuv_output); // dng images from camera to yuv
                    demosaick_yuv_video(yuv_output, final_yuv_output, input_folder); // demosaick yuv before sending to coder
                    // send to coder (ffmpeg?)
                }
            }
            iStrLen = strlen(buf);
            CvRect rect = cvRect(iTextX, iTextY - 15, iStrLen* 11, 20);
            cvSetImageROI(pRgb , rect);
            cvSet(pRgb, cvScalar(180, 180, 180));
            cvResetImageROI(pRgb);
        }
        cvText(pRgb, buf, iTextX,iTextY );

        if(bSendTiggerSignal)
        {
            ASISendSoftTrigger(CamInfo.CameraID, ASI_TRUE);
            bSendTiggerSignal = 0;
        }

        if(bChangeFormat)
        {
            bChangeFormat = 0;
            cvReleaseImage(&pRgb);
            ASIStopVideoCapture(CamInfo.CameraID);

            switch(change)
            {
                case change_imagetype:
                    Image_type++;
                    if(Image_type > 3)
                        Image_type = 0;

                    break;
                case change_bin:
                    if(bin == 1)
                    {
                        bin = 2;
                        width/=2;
                        height/=2;
                    }
                    else
                    {
                        bin = 1;
                        width*=2;
                        height*=2;
                    }
                    break;
                case change_size_smaller:
                    if(width > 320 && height > 240)
                    {
                        width/= 2;
                        height/= 2;
                    }
                    break;

                case change_size_bigger:

                    if(width*2*bin <= iMaxWidth && height*2*bin <= iMaxHeight)
                    {
                        width*= 2;
                        height*= 2;
                    }
                    break;
            }
            ASISetROIFormat(CamInfo.CameraID, width, height, bin, (ASI_IMG_TYPE)Image_type);
            if(Image_type == ASI_IMG_RAW16)
                pRgb=cvCreateImage(cvSize(width,height), IPL_DEPTH_16U, 1);
            else if(Image_type == ASI_IMG_RGB24)
                pRgb=cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, 3);
            else
                pRgb=cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, 1);
            ASIStartVideoCapture(CamInfo.CameraID); //start privew
        }
    }
    END:

    ASIStopVideoCapture(CamInfo.CameraID);
    ASICloseCamera(CamInfo.CameraID);
    cvReleaseImage(&pRgb);
    printf("main function over\n");
    return 1;
}
