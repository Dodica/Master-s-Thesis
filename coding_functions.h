#ifndef CODING_FUNCTIONS_H
#define CODING_FUNCTIONS_H

#include <opencv2/opencv.hpp>
#include <fstream>
#include <string>

void convert_dng_to_yuv(const cv::Mat& raw_image, std::string& filename);

void demosaick_yuv_video(const std::string& encoded_yuv_artificial, const std::string& final_artificial_yuv_output, const std::string& input_folder);

void demosaick(const cv::Mat& bayer, cv::Mat& r_dem, cv::Mat& g_dem, cv::Mat& b_dem);

std::tuple<cv::Mat, cv::Mat, cv::Mat> extract_channels(cv::Mat bayer);

#endif