#include "stdio.h"
#include <iostream>
#include "ASICamera2.h"
#include "opencv2/highgui/highgui_c.h"
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <thread>
#include "coding_functions.h"

using namespace std;
using namespace cv;

#define FPS 20
constexpr int CALLBACK_PERIOD = 1000 / FPS;

int bDisplay = 0;
int bMain = 1;
ASI_CAMERA_INFO CamInfo;

static void Display(IplImage* pImg)
{
    cvNamedWindow("video", 1);
    while (bDisplay)
    {
        cvShowImage("video", pImg);

        char c = cvWaitKey(1);
        if (c == 27) {
            bDisplay = false;
            bMain = false;
            break;
        }
    }
    cvDestroyWindow("video");
    ASIStopVideoCapture(CamInfo.CameraID);
}

int main(int argc, char* argv[])
{
    const char* bayer[] = { "RG","BG","GR","GB" };

    int width;
    int height;
    int i;
    bool bresult;
    int frames = 0;

    int CamIndex = 0;
    int inputformat;
    int definedformat;

    IplImage* pRgb;

    int numDevices = ASIGetNumOfConnectedCameras();
    if (numDevices <= 0)
    {
        printf("no camera connected, press any key to exit\n");
        getchar();
        return -1;
    }
    else
        printf("attached cameras:\n");

    for (i = 0; i < numDevices; i++)
    {
        ASIGetCameraProperty(&CamInfo, i);
        printf("%d %s\n", i, CamInfo.Name);
    }

    printf("\nselect one to preview\n");
    scanf_s("%d", &CamIndex);

    ASIGetCameraProperty(&CamInfo, CamIndex);
    bresult = ASIOpenCamera(CamInfo.CameraID);
    bresult += ASIInitCamera(CamInfo.CameraID);
    if (bresult)
    {
        printf("OpenCamera error, press any key to exit\n");
        getchar();
        return -1;
    }

    printf("%s information\n", CamInfo.Name);
    int iMaxWidth, iMaxHeight;
    iMaxWidth = CamInfo.MaxWidth;
    iMaxHeight = CamInfo.MaxHeight;
    printf("resolution:%dX%d\n", iMaxWidth, iMaxHeight);
    if (CamInfo.IsColorCam)
        printf("Color Camera: bayer pattern:%s\n", bayer[CamInfo.BayerPattern]);
    else
        printf("Mono camera\n");

    int ctrlnum;
    ASIGetNumOfControls(CamInfo.CameraID, &ctrlnum);
    ASI_CONTROL_CAPS ctrlcap;
    for (i = 0; i < ctrlnum; i++)
    {
        ASIGetControlCaps(CamInfo.CameraID, i, &ctrlcap);

        printf("%s\n", ctrlcap.Name);
    }

    int bin = 1, Image_type;
    printf("Use custom format or predefined format resolution?\n 0:custom format \n 1:predefined format\n");
    scanf_s("%d", &inputformat);
    if (inputformat)
    {
        printf("0:Size 1024 X 768, BIN 2, ImgType raw8\n");
        printf("1:Size 3840 X 2160, BIN 1, ImgType raw8\n");
        printf("2:Size 1024 X 768, BIN 4, ImgType RGB24\n");
        scanf_s("%d", &definedformat);
        if (definedformat == 0)
        {
            ASISetROIFormat(CamInfo.CameraID, 1024, 768, 4, ASI_IMG_RAW8);
            width = 1024;
            height = 768;
            bin = 2;
            Image_type = ASI_IMG_RAW8;
        }
        else if (definedformat == 1)
        {
            ASISetROIFormat(CamInfo.CameraID, 3840, 2160, 1, ASI_IMG_RAW8);
            width = 3840;
            height = 2160;
            bin = 1;
            Image_type = ASI_IMG_RAW8;
        }
        else if (definedformat == 2)
        {
            ASISetROIFormat(CamInfo.CameraID, 1024, 768, 4, ASI_IMG_RGB24);
            width = 1024;
            height = 768;
            bin = 4;
            Image_type = ASI_IMG_RGB24;
        }
    }
    else
    {
        printf("please input the resolution you want to set\n");
        printf("width\n");
        scanf_s("%d", &width);
        printf("height\n");
        scanf_s("%d", &height);
        printf("bin\n");
        scanf_s("%d (1, 2, 3, 4)", &bin);
        printf("Image type (0: RAW8, 1: RGB24, 2: RAW16, 3: Y8)\n");
        scanf_s("%d", &Image_type);
        ASISetROIFormat(CamInfo.CameraID, width, height, bin, (ASI_IMG_TYPE)Image_type);
    }

    printf("start video capture\n");

    ASIStartVideoCapture(CamInfo.CameraID);
    printf("after start video capture\n");

    uint8_t* pRawBuf = nullptr;
    if (Image_type == ASI_IMG_RAW16) {
        pRawBuf = (uint8_t*)malloc(width * height * sizeof(uint16_t));
    }
    else {
        pRawBuf = (uint8_t*)malloc(width * height);
    }

    if (pRawBuf == nullptr) {
        cerr << "Memory allocation failed for pRawBuf" << endl;
        return -1;
    }

    pRgb = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
    bDisplay = true;
    thread displayThread(Display, pRgb);
    string normal_yuv = "output_video_normal.yuv";
    ofstream oFile(normal_yuv, ios::binary | ios::trunc);

    while (bMain)
    {
        int ret;
        if (Image_type != ASI_IMG_RAW8) {
            ret = ASIGetVideoData(CamInfo.CameraID, (unsigned char*)pRgb->imageData, pRgb->imageSize, CALLBACK_PERIOD);
        }
        else {
            ret = ASIGetVideoData(CamInfo.CameraID, pRawBuf, width * height, CALLBACK_PERIOD);
        }

        if (ret != ASI_SUCCESS) {
            printf("Failed to get video data, error code: %d\n", ret);
        }
        else {
            printf("Successfully retrieved video data\n");

            if (Image_type == ASI_IMG_RAW8) {
                frames += 1;
                Mat bayer_img(height, width, CV_8UC1, pRawBuf);
                Mat rgb_img;

                cvtColor(bayer_img, rgb_img, COLOR_BayerRG2RGB);

                memcpy(pRgb->imageData, rgb_img.data, rgb_img.total() * rgb_img.elemSize());
                Mat r_channel, g_channel, b_channel;

                extract_channels(bayer_img, r_channel, g_channel, b_channel);

                save_yuv_frame(g_channel, b_channel, r_channel, oFile);

                /*cout << "Red Channel size: " << r_channel.rows << " x " << r_channel.cols << endl;
                cout << "Green Channel size: " << g_channel.rows << " x " << g_channel.cols << endl;
                cout << "Blue Channel size: " << b_channel.rows << " x " << b_channel.cols << endl;
                Mat r_resized, g_resized, b_resized;
                resize(r_channel, r_resized, bayer_img.size(), 0, 0, INTER_NEAREST);
                resize(g_channel, g_resized, bayer_img.size(), 0, 0, INTER_NEAREST);
                resize(b_channel, b_resized, bayer_img.size(), 0, 0, INTER_NEAREST);

                vector<Mat> channels = { b_resized, g_resized, r_resized }; // Note: OpenCV uses BGR format by default
                Mat color_image;
                merge(channels, color_image);

                imwrite("color_image.png", color_image);

                cout << "Color image saved as color_image.png" << endl;*/
                //save_color_channel(r_channel, "r_channel.png", Scalar(2, 0, 0));
                //save_color_channel(g_channel, "g_channel.png", Scalar(1, 1, 0));
                //save_color_channel(b_channel, "b_channel.png", Scalar(0, 0, 2));

                //save_histogram(r_channel, "r_histogram.png");
                //save_histogram(g_channel, "g_histogram.png");
                //save_histogram(b_channel, "b_histogram.png");

                string outputFile = "output_video.yuv";
                write_yuv_frame(g_channel, b_channel, r_channel, outputFile);

            }
        }
    }
    displayThread.join();
    cvReleaseImage(&pRgb);
    ASICloseCamera(CamInfo.CameraID);

    string input_file = "output_video.yuv"; // yuv422p masked
    string input_file_normal = "output_video_normal.yuv"; // yuv444p normal
    string encoded_file = "encoded_video.mp4"; // mp4 masked
    string encoded_file_normal = "encoded_video_normal.mp4"; // mp4 normal
    string psnr_log_file = "psnr_output.log"; // psnr masked
    string psnr_log_file_normal = "psnr_output_normal.log"; // psnr normal
    string encode_log_file = "ffmpeg_output.log"; // encode log masked
    string encode_log_file_normal = "ffmpeg_output_normal.log"; // encode log normal
    string decoded_yuv_file = "decoded_output_video.yuv"; // decoded masked yuv422p
    string decoded_yuv_file_normal = "decoded_output_video_normal.yuv"; // decoded normal yuv444p
    string final_yuv_output = "final_output_video.yuv"; // decoded masked and transformed yuv444p
    string psnr_log_file_final = "psnr_output_final.log"; // psnr between 2 final videos

    //int width = 1024;
    //int height = 768;
    //int frames = 202;
    string yuv_format_422p = "yuv422p";
    string yuv_format_444p = "yuv444p";
    int crf = 21;

    cout << "Number of frames is " << frames << endl;

    cout << "Encoding masked YUV " << endl;
    encode_yuv_with_ffmpeg(input_file, encoded_file, width, height / 2, yuv_format_422p, crf);
    cout << "Masked YUV encoding finished, saved as \"encoded_video.mp4\", now encoding normal YUV " << endl;

    encode_yuv_with_ffmpeg_for_normal(input_file_normal, encoded_file_normal, width, height, yuv_format_444p, crf);
    cout << "Normal YUV encoding finished, saved as \"encoded_video_normal.mp4\", now calculating PSNR for masked YUV " << endl;


    calculate_psnr(input_file, encoded_file, psnr_log_file, width, height, yuv_format_422p);
    cout << "PSNR calculation for masked finished, now calculating PSNR for normal YUV " << endl;

    calculate_psnr(input_file_normal, encoded_file_normal, psnr_log_file_normal, width, height, yuv_format_444p);
    cout << "PSNR calculation for normal finished, now extracting metrics for masked YUV " << endl;


    extract_metrics(psnr_log_file, encode_log_file);
    cout << "Extracting metrics for masked finished, now extracting metrics for normal " << endl;

    extract_metrics(psnr_log_file_normal, encode_log_file_normal);
    cout << "Extracting metrics for normal finished, now decoding masked video to mp4" << endl;


    convert_mp4_to_yuv(encoded_file, decoded_yuv_file, yuv_format_422p);
    cout << "Decoding masked video to mp4 finished and saved to \"decoded_output_video.yuv\", now decoding normal video to mp4" << endl;

    convert_mp4_to_yuv(encoded_file_normal, decoded_yuv_file_normal, yuv_format_444p);
    cout << "Decoding normal video to mp4 finished and saved to \"decoded_output_video_normal.yuv\", now extracting channels from masked decoded video" << endl;

    Mat g_channel, b_channel, r_channel;
    ofstream outFile(final_yuv_output, ios::binary | ios::trunc);


    for (int i = 0; i < frames; i++) {
        read_yuv_frame(decoded_yuv_file, i, g_channel, b_channel, r_channel, width, height);
        save_yuv_frame(g_channel, b_channel, r_channel, outFile);
    }
    cout << "Upscaled video saved to \"final_output_video.yuv\" " << endl;

    outFile.close();

    //display_yuv_video(decoded_yuv_file, frames, width, height);

    //display_yuv444_video(decoded_yuv_file_normal, frames, width, height);

    /*string outputFile = "output.png";
    int frameIndex = frames-2;

    read_yuv_frame(decoded_yuv_file, frameIndex, g_channel, b_channel, r_channel, width, height);
    save_rgb_image(g_channel, b_channel, r_channel, outputFile, width, height);
    cout << "RGB image saved as " << outputFile << endl;*/


    cout << "Final YUV made for masked, calculating PSNR between the 2 final videos" << endl;
    calculate_psnr2(final_yuv_output, decoded_yuv_file_normal, psnr_log_file_final, width, height, yuv_format_444p);

    cout << "PSNR calculation finished, now extracting metrics" << endl;
    extract_metrics_final(psnr_log_file_final);

    return 0;
}
