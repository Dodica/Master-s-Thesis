#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
#include <cmath>
#include <tuple>
#include <string>
#include <filesystem>
#include <iostream>

void convert_dng_to_yuv(const cv::Mat& raw_image, const std::string& filename) {
    std::ofstream yuv_file(filename, std::ios::binary);
    if (!yuv_file) {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
        return;
    }
    // Extract G, R, and B channels directly from RGGB
    cv::Mat img_r, img_g, img_b;
    std::tie(img_r, img_g, img_b) = extract_channels(raw_image);

    // White balance
    cv::Scalar mean_r = cv::mean(img_r);
    cv::Scalar mean_g = cv::mean(img_g);
    cv::Scalar mean_b = cv::mean(img_b);

    img_g /= (mean_g[0] / mean_r[0]);
    img_b /= (mean_b[0] / mean_r[0]);

    // Clip outliers
    double min_max = std::max(std::max(cv::norm(img_r), cv::norm(img_g)), cv::norm(img_b));

    cv::threshold(img_r, img_r, min_max, min_max, cv::THRESH_TRUNC);
    cv::threshold(img_g, img_g, min_max, min_max, cv::THRESH_TRUNC);
    cv::threshold(img_b, img_b, min_max, min_max, cv::THRESH_TRUNC);

    // Scale to 0.0 - 1.0 range
    img_r.convertTo(img_r, CV_64F);
    img_g.convertTo(img_g, CV_64F);
    img_b.convertTo(img_b, CV_64F);
    img_r /= 4095.0;
    img_g /= 4095.0;
    img_b /= 4095.0;

    // Gamma correction
    cv::pow(img_r, 1 / 2.2, img_r);
    cv::pow(img_g, 1 / 2.2, img_g);
    cv::pow(img_b, 1 / 2.2, img_b);

    // Convert to 0-255 range
    img_r.convertTo(img_r, CV_8U, 255.0);
    img_g.convertTo(img_g, CV_8U, 255.0);
    img_b.convertTo(img_b, CV_8U, 255.0);

    // Write pixels in YUV422P format (planar)
    std::vector<cv::Mat> channels = {img_g, img_b, img_r};
    for (const auto& channel : channels) {
        for (int i = 0; i < channel.rows; ++i) {
            yuv_file.write(reinterpret_cast<const char*>(channel.ptr(i)), channel.cols);
        }
    }
}

void demosaick_yuv_video(const std::string& encoded_yuv_artificial, const std::string& final_artificial_yuv_output, const std::string& input_folder) {
    std::ofstream yuv_file(final_artificial_yuv_output, std::ios::binary);
    std::ifstream video(encoded_yuv_artificial, std::ios::binary);
    if (!yuv_file || !video) {
        std::cerr << "Error: Unable to open files." << std::endl;
        return;
    }

    int dim_x = 2160;
    int dim_y = 3840;
    cv::Mat bayer_rev(dim_x, dim_y, CV_8U);

    // Get filenames from directory
    std::vector<std::string> filenames;
    for (const auto& entry : std::filesystem::directory_iterator(input_folder)) {
        if (entry.path().extension() == ".DNG") {
            filenames.emplace_back(entry.path().string());
        }
    }

    // For each frame, read r, g, and b values
    // Here I'll probably use the "count" variable from main
    for (size_t frame = 0; frame < filenames.size(); ++frame) {
        std::string filename = filenames[frame];
        std::cout << "Encoding frame " << frame << "..." << std::endl;

        cv::Mat g(dim_x / 2, dim_y, CV_8U);
        cv::Mat b(dim_x / 2, dim_y / 2, CV_8U);
        cv::Mat r(dim_x / 2, dim_y / 2, CV_8U);

        // g channel
        for (int i = 0; i < dim_x / 2; ++i) {
            for (int j = 0; j < dim_y; ++j) {
                char byte;
                video.read(&byte, 1);
                g.at<uchar>(i, j) = static_cast<uchar>(byte);
            }
        }

        // b channel
        for (int i = 0; i < dim_x / 2; ++i) {
            for (int j = 0; j < dim_y / 2; ++j) {
                char byte;
                video.read(&byte, 1);
                b.at<uchar>(i, j) = static_cast<uchar>(byte);
            }
        }

        // r channel
        for (int i = 0; i < dim_x / 2; ++i) {
            for (int j = 0; j < dim_y / 2; ++j) {
                char byte;
                video.read(&byte, 1);
                r.at<uchar>(i, j) = static_cast<uchar>(byte);
            }
        }

        for (int x = 0; x < dim_x; x += 2) {
            int parity = 1;
            for (int y = 0; y < dim_y; ++y) {
                bayer_rev.at<uchar>(x + parity, y) = g.at<uchar>(x / 2, y);
                parity = 1 - parity;
            }
        }

        for (int x = 0; x < dim_x; x += 2) {
            for (int y = 0; y < dim_y; y += 2) {
                bayer_rev.at<uchar>(x, y) = r.at<uchar>(x / 2, y / 2);
            }
        }

        for (int x = 0; x < dim_x; x += 2) {
            for (int y = 0; y < dim_y; y += 2) {
                bayer_rev.at<uchar>(x + 1, y + 1) = b.at<uchar>(x / 2, y / 2);
            }
        }

        cv::Mat r_dem, g_dem, b_dem;
        demosaick(bayer_rev, r_dem, g_dem, b_dem);

        // r, g, b channels -> RGB image
        cv::Mat image_rec(dim_x, dim_y, CV_8UC3);
        std::vector<cv::Mat> channels = { b_dem, g_dem, r_dem };
        cv::merge(channels, image_rec);

        // Write to YUV file
        yuv_file.write(reinterpret_cast<const char*>(image_rec.data), image_rec.total() * image_rec.elemSize());
    }

    yuv_file.close();
}

void demosaick(const cv::Mat& bayer, cv::Mat& r_dem, cv::Mat& g_dem, cv::Mat& b_dem) {
    int dim_x = bayer.rows;
    int dim_y = bayer.cols;
    int parity = 0;

    r_dem.create(dim_x, dim_y, CV_8U);
    g_dem.create(dim_x, dim_y, CV_8U);
    b_dem.create(dim_x, dim_y, CV_8U);

    for (int x = 0; x < dim_x; x += 2) {
        for (int y = 0; y < dim_y; y += 2) {
            r_dem.at<uchar>(x, y) = bayer.at<uchar>(x, y);
            r_dem.at<uchar>(x + 1, y) = bayer.at<uchar>(x, y);
            r_dem.at<uchar>(x, y + 1) = bayer.at<uchar>(x, y);
            r_dem.at<uchar>(x + 1, y + 1) = bayer.at<uchar>(x, y);
        }
    }

    for (int x = 0; x < dim_x; x += 2) {
        for (int y = 0; y < dim_y; ++y) {
            parity = 1 - parity;
            g_dem.at<uchar>(x, y) = bayer.at<uchar>(x + parity, y);
            g_dem.at<uchar>(x + 1, y) = bayer.at<uchar>(x + parity, y);
        }
    }

    for (int x = 0; x < dim_x; x += 2) {
        for (int y = 0; y < dim_y; y += 2) {
            b_dem.at<uchar>(x, y) = bayer.at<uchar>(x + 1, y + 1);
            b_dem.at<uchar>(x + 1, y) = bayer.at<uchar>(x + 1, y + 1);
            b_dem.at<uchar>(x, y + 1) = bayer.at<uchar>(x + 1, y + 1);
            b_dem.at<uchar>(x + 1, y + 1) = bayer.at<uchar>(x + 1, y + 1);
        }
    }
}

std::tuple<cv::Mat, cv::Mat, cv::Mat> extract_channels(cv::Mat bayer) {
    // Assuming bayer is a 2D cv::Mat with the RGGB pattern

    // Red channel: Every other pixel in every other row starting from [0, 0]
    cv::Mat r = bayer(cv::Rect(0, 0, bayer.cols, bayer.rows) & cv::Rect(0, 0, bayer.cols & ~1, bayer.rows & ~1));

    // Green channel: A mix of every other pixel starting from [0, 1] and [1, 0]
    cv::Mat g1 = bayer(cv::Rect(0, 1, bayer.cols, bayer.rows) & cv::Rect(0, 1, bayer.cols & ~1, bayer.rows & ~1)); // Even rows, odd columns
    cv::Mat g2 = bayer(cv::Rect(1, 0, bayer.cols, bayer.rows) & cv::Rect(1, 0, bayer.cols & ~1, bayer.rows & ~1)); // Odd rows, even columns
    cv::Mat g(bayer.rows / 2, bayer.cols, bayer.type()); // Preparing green channel
    g.colRange(0, g.cols & ~1) = g2; // Filling in from g2
    g.colRange(1, g.cols & ~1) = g1; // Filling in from g1

    // Blue channel: Every other pixel in every other row starting from [1, 1]
    cv::Mat b = bayer(cv::Rect(1, 1, bayer.cols, bayer.rows) & cv::Rect(1, 1, bayer.cols & ~1, bayer.rows & ~1));

    return std::make_tuple(r, g, b);
}