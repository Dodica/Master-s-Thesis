#ifndef CODING_FUNCTIONS_H
#define CODING_FUNCTIONS_H

#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
#include <cmath>
#include <tuple>
#include <string>
#include <filesystem>
#include <iostream>
#include <opencv2/core/core_c.h>
#include <algorithm>
#include <cstdint>

using namespace std;
using namespace cv;

struct PSNRValues {
    vector<double> psnr_avg;
    vector<double> psnr_y;
    vector<double> psnr_u;
    vector<double> psnr_v;
};

PSNRValues parse_psnr_log(const string& log_file);

double calculate_mean(const vector<double>& values);

void extract_metrics_final(const string& log_file);

void calculate_psnr2(const string& input_file, const string& encoded_file, const string& log_file, int width, int height, const string& yuv_format);

void read_yuv444_frame(const string& inputFile, int frameIndex, Mat& y_channel, Mat& u_channel, Mat& v_channel, int width, int height);

void display_yuv444_video(const string& inputFile, int frames, int width, int height);

void display_yuv_video(const string& inputFile, int frames, int width, int height);

void save_yuv_frame(const Mat& g_channel, const Mat& b_channel, const Mat& r_channel, ofstream& outFile);

void save_rgb_image(const Mat& g_channel, const Mat& b_channel, const Mat& r_channel, const string& outputFile, int width, int height);

void read_yuv_frame(const string& inputFile, int frameIndex, Mat& g_channel, Mat& b_channel, Mat& r_channel, int width, int height);

void convert_mp4_to_yuv(const string& encoded_mp4, const string& output_yuv, const string& yuv_format);

void encode_yuv_with_ffmpeg(const string& input_file, const string& output_file, int width, int height, const string& yuv_format, int crf);

void encode_yuv_with_ffmpeg_for_normal(const string& input_file, const string& output_file, int width, int height, const string& yuv_format, int crf);

void calculate_psnr(const string& input_file, const string& encoded_file, const string& log_file, int width, int height, const string& yuv_format);

void extract_metrics(const string& psnr_log_file, const string& encode_log_file);

void process_channels(Mat& r_channel, Mat& g_channel, Mat& b_channel);

void write_yuv_frame(const Mat& g_channel, const Mat& b_channel, const Mat& r_channel, const string& outputFile);

void save_color_channel(const Mat& channel, const string& filename, const Scalar& color);

void save_histogram(const Mat& image, const string& filename);

void extract_channels(const Mat& bayer, Mat& r, Mat& g, Mat& b);

#endif