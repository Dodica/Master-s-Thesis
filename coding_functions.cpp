#include <opencv2/opencv.hpp>
#include <opencv2/core/core_c.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <tuple>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <regex>
#include <sys/stat.h>

using namespace std;
using namespace cv;

void read_yuv444_frame(const string& inputFile, int frameIndex, Mat& y_channel, Mat& u_channel, Mat& v_channel, int width, int height) {
    int y_rows = height;
    int y_cols = width;
    int u_rows = height;
    int u_cols = width;
    int v_rows = height;
    int v_cols = width;

    size_t y_size = y_rows * y_cols;
    size_t u_size = u_rows * u_cols;
    size_t v_size = v_rows * v_cols;

    size_t frame_size = y_size + u_size + v_size;
    size_t frame_offset = frameIndex * frame_size;

    ifstream inFile(inputFile, ios::binary);
    if (!inFile.is_open()) {
        cerr << "Could not open input file: " << inputFile << endl;
        return;
    }

    inFile.seekg(frame_offset, ios::beg);

    y_channel.create(y_rows, y_cols, CV_8UC1);
    u_channel.create(u_rows, u_cols, CV_8UC1);
    v_channel.create(v_rows, v_cols, CV_8UC1);

    inFile.read(reinterpret_cast<char*>(y_channel.data), y_size);
    inFile.read(reinterpret_cast<char*>(u_channel.data), u_size);
    inFile.read(reinterpret_cast<char*>(v_channel.data), v_size);

    inFile.close();
}

void display_yuv444_video(const string& inputFile, int frames, int width, int height) {
    Mat y_channel, u_channel, v_channel;

    for (int i = 0; i < frames; i++) {
        read_yuv444_frame(inputFile, i, y_channel, u_channel, v_channel, width, height);

        Mat yuv_image, bgr_image;
        vector<Mat> yuv_channels = { y_channel, u_channel, v_channel };
        merge(yuv_channels, yuv_image);
        cvtColor(yuv_image, bgr_image, COLOR_YUV2BGR);

        imshow("YUV444 Video", bgr_image);

        waitKey(33);
    }

    destroyAllWindows();
}

void YUVfromRGB(double& Y, double& U, double& V, const double R, const double G, const double B)
{
    Y = ((66 * R + 129 * G + 25 * B + 128) / 256) + 16;
    U = ((-38 * R - 74 * G + 112 * B + 128) / 256) + 128;
    V = ((112 * R - 94 * G - 18 * B + 128) / 256) + 128;
}

void read_yuv_frame(const string& inputFile, int frameIndex, Mat& g_channel, Mat& b_channel, Mat& r_channel, int width, int height) {
    int g_rows = height / 2;
    int g_cols = width;
    int b_rows = height / 2;
    int b_cols = width / 2;
    int r_rows = height / 2;
    int r_cols = width / 2;

    size_t g_size = height / 2 * width;
    size_t b_size = height / 2 * width / 2;
    size_t r_size = height / 2 * width / 2;

    size_t frame_size = g_size + b_size + r_size;
    size_t frame_offset = frameIndex * frame_size;

    ifstream inFile(inputFile, ios::binary);
    if (!inFile.is_open()) {
        cerr << "Could not open input file: " << inputFile << endl;
        return;
    }

    inFile.seekg(frame_offset, ios::beg);

    g_channel.create(g_rows, g_cols, CV_8UC1);
    b_channel.create(b_rows, b_cols, CV_8UC1);
    r_channel.create(r_rows, r_cols, CV_8UC1);

    inFile.read(reinterpret_cast<char*>(g_channel.data), g_size);
    inFile.read(reinterpret_cast<char*>(b_channel.data), b_size);
    inFile.read(reinterpret_cast<char*>(r_channel.data), r_size);

    inFile.close();
}

void display_yuv_video(const string& inputFile, int frames, int width, int height) {
    Mat g_channel, b_channel, r_channel;

    for (int i = 0; i < frames; i++) {
        read_yuv_frame(inputFile, i, g_channel, b_channel, r_channel, width, height);

        Mat g_resized, b_resized, r_resized;
        resize(g_channel, g_resized, Size(width, height), 0, 0, INTER_CUBIC);
        resize(b_channel, b_resized, Size(width, height), 0, 0, INTER_CUBIC);
        resize(r_channel, r_resized, Size(width, height), 0, 0, INTER_CUBIC);

        vector<Mat> channels = { b_resized, g_resized, r_resized };
        Mat color_image;
        merge(channels, color_image);

        imshow("YUV Video", color_image);

        waitKey(33);
    }

    destroyAllWindows();
}


void save_yuv_frame(const Mat& g_channel, const Mat& b_channel, const Mat& r_channel, ofstream& outFile) {
    int full_width = g_channel.cols;
    int full_height = g_channel.rows * 2;

    Mat y_channel, u_channel, v_channel;
    resize(g_channel, y_channel, Size(full_width, full_height), 0, 0, INTER_LINEAR);
    resize(b_channel, u_channel, Size(full_width, full_height), 0, 0, INTER_LINEAR);
    resize(r_channel, v_channel, Size(full_width, full_height), 0, 0, INTER_LINEAR);

    outFile.write(reinterpret_cast<const char*>(y_channel.data), y_channel.total() * y_channel.elemSize());
    outFile.write(reinterpret_cast<const char*>(u_channel.data), u_channel.total() * u_channel.elemSize());
    outFile.write(reinterpret_cast<const char*>(v_channel.data), v_channel.total() * v_channel.elemSize());
}



void save_rgb_image(const Mat& g_channel, const Mat& b_channel, const Mat& r_channel, const string& outputFile, int width, int height) {
    Mat g_resized, b_resized, r_resized;
    resize(g_channel, g_resized, Size(width, height), 0, 0, INTER_CUBIC);
    resize(b_channel, b_resized, Size(width, height), 0, 0, INTER_CUBIC);
    resize(r_channel, r_resized, Size(width, height), 0, 0, INTER_CUBIC);

    vector<Mat> channels = { b_resized, g_resized, r_resized }; // Note: OpenCV uses BGR format by default
    Mat color_image;
    merge(channels, color_image);

    imwrite(outputFile, color_image);
}

void encode_yuv_with_ffmpeg(const string& input_file, const string& output_file, int width, int height, const string& yuv_format, int crf) {
    ostringstream command;
    command << "ffmpeg -s " << width << "x" << height
        << " -pixel_format " << yuv_format
        << " -i " << input_file
        << " -c:v libx265 -crf " << crf
        << " -y " << output_file
        << " > ffmpeg_output.log 2>&1";

    string command_str = command.str();
    system(command_str.c_str());
}

void encode_yuv_with_ffmpeg_for_normal(const string& input_file, const string& output_file, int width, int height, const string& yuv_format, int crf) {
    ostringstream command;
    command << "ffmpeg -s " << width << "x" << height
        << " -pixel_format " << yuv_format
        << " -i " << input_file
        << " -c:v libx265 -crf " << crf
        << " -y " << output_file
        << " > ffmpeg_output_normal.log 2>&1";

    string command_str = command.str();
    system(command_str.c_str());
}


void convert_mp4_to_yuv(const string& encoded_mp4, const string& output_yuv, const string& yuv_format) {
    ostringstream command;
    command << "ffmpeg"
        << " -i " << encoded_mp4
        << " -c:v rawvideo"
        << " -pix_fmt " << yuv_format
        << " -y " << output_yuv
        << " > ffmpeg_decode_output.log 2>&1";

    string command_str = command.str();
    system(command_str.c_str());
}


void calculate_psnr(const string& input_file, const string& encoded_file, const string& log_file, int width, int height, const string& yuv_format) {
    ostringstream command;
    command << "ffmpeg -s " << width << "x" << height
        << " -pixel_format " << yuv_format
        << " -i " << input_file
        << " -i " << encoded_file
        << " -filter_complex \"[0:v][1:v]psnr\""
        << " -f null -"
        << " > " << log_file << " 2>&1";

    string command_str = command.str();
    system(command_str.c_str());
}

struct PSNRValues {
    vector<double> psnr_avg;
    vector<double> psnr_y;
    vector<double> psnr_u;
    vector<double> psnr_v;
};

PSNRValues parse_psnr_log(const string& log_file) {
    PSNRValues values;
    ifstream infile(log_file);
    string line;
    regex psnr_regex(R"(psnr_avg:(\d+\.\d+)\spsnr_y:(\d+\.\d+)\spsnr_u:(\d+\.\d+)\spsnr_v:(\d+\.\d+))");

    while (getline(infile, line)) {
        smatch match;
        if (regex_search(line, match, psnr_regex)) {
            values.psnr_avg.push_back(stod(match[1]));
            values.psnr_y.push_back(stod(match[2]));
            values.psnr_u.push_back(stod(match[3]));
            values.psnr_v.push_back(stod(match[4]));
        }
    }
    return values;
}

double calculate_mean(const vector<double>& values) {
    double sum = 0.0;
    for (double value : values) {
        sum += value;
    }
    return values.empty() ? 0.0 : sum / values.size();
}

void extract_metrics_final(const string& log_file) {
    PSNRValues values = parse_psnr_log(log_file);

    double mean_psnr_avg = calculate_mean(values.psnr_avg);
    double mean_psnr_y = calculate_mean(values.psnr_y);
    double mean_psnr_u = calculate_mean(values.psnr_u);
    double mean_psnr_v = calculate_mean(values.psnr_v);

    cout << "Mean PSNR (Average): " << mean_psnr_avg << " dB" << endl;
    cout << "Mean PSNR (Y): " << mean_psnr_y << " dB" << endl;
    cout << "Mean PSNR (U): " << mean_psnr_u << " dB" << endl;
    cout << "Mean PSNR (V): " << mean_psnr_v << " dB" << endl;
}

void calculate_psnr2(const string& file1, const string& file2, const string& log_file, int width, int height, const string& pixel_format) {
    string command = "ffmpeg -s " + to_string(width) + "x" + to_string(height) +
        " -pix_fmt " + pixel_format +
        " -i " + file1 +
        " -s " + to_string(width) + "x" + to_string(height) +
        " -pix_fmt " + pixel_format +
        " -i " + file2 +
        " -lavfi psnr=stats_file=" + log_file + " -f null -";

    int result = system(command.c_str());

    if (result != 0) {
        cerr << "Error: FFmpeg command failed with exit code " << result << endl;
    }
    else {
        cout << "PSNR calculation completed successfully. Log file: " << log_file << endl;
    }
}

void extract_metrics(const string& psnr_log_file, const string& encode_log_file) {
    ifstream psnr_log(psnr_log_file);
    if (!psnr_log.is_open()) {
        cerr << "Error opening PSNR log file: " << psnr_log_file << endl;
        return;
    }

    stringstream psnr_buffer;
    psnr_buffer << psnr_log.rdbuf();
    string psnr_result = psnr_buffer.str();

    regex psnr_pattern("average:([\\d\\.]+)");
    smatch psnr_match;
    string psnr = "N/A";
    if (regex_search(psnr_result, psnr_match, psnr_pattern)) {
        psnr = psnr_match[1].str();
    }

    ifstream encode_log(encode_log_file);
    if (!encode_log.is_open()) {
        cerr << "Error opening encoding log file: " << encode_log_file << endl;
        return;
    }

    stringstream encode_buffer;
    encode_buffer << encode_log.rdbuf();
    string encode_result = encode_buffer.str();

    regex time_pattern("encoded \\d+ frames in ([\\d\\.]+)s.*?Avg QP:([\\d\\.]+)");
    smatch time_match;
    string encode_time = "N/A";
    string avg_qp = "N/A";
    if (regex_search(encode_result, time_match, time_pattern)) {
        encode_time = time_match[1].str();
        avg_qp = time_match[2].str();
    }

    struct stat stat_buf;
    int rc = stat("output_video.yuv", &stat_buf);
    long raw_size = rc == 0 ? stat_buf.st_size : -1;
    rc = stat("encoded_video.mp4", &stat_buf);
    long encoded_size = rc == 0 ? stat_buf.st_size : -1;
    double compression_ratio = (encoded_size > 0) ? static_cast<double>(raw_size) / encoded_size : 0.0;

    cout << "PSNR: " << psnr << endl;
    cout << "Encoding Time (s): " << encode_time << endl;
    cout << "Average QP: " << avg_qp << endl;
    cout << "Compression Ratio: " << compression_ratio << endl;
}

void process_channels(Mat& r_channel, Mat& g_channel, Mat& b_channel) {
    double g_mean = mean(g_channel)[0];
    double r_mean = mean(r_channel)[0];
    double b_mean = mean(b_channel)[0];

    g_channel = g_channel / (g_mean / r_mean);
    b_channel = b_channel / (b_mean / r_mean);

    double r_max, g_max, b_max;
    minMaxLoc(r_channel, nullptr, &r_max);
    minMaxLoc(g_channel, nullptr, &g_max);
    minMaxLoc(b_channel, nullptr, &b_max);

    double max_val = min({ r_max, g_max, b_max });

    r_channel.setTo(max_val, r_channel > max_val);
    g_channel.setTo(max_val, g_channel > max_val);
    b_channel.setTo(max_val, b_channel > max_val);

    r_channel.convertTo(r_channel, CV_32F, 1.0 / 4095.0);
    g_channel.convertTo(g_channel, CV_32F, 1.0 / 4095.0);
    b_channel.convertTo(b_channel, CV_32F, 1.0 / 4095.0);

    pow(r_channel, 1.0 / 2.2, r_channel);
    pow(g_channel, 1.0 / 2.2, g_channel);
    pow(b_channel, 1.0 / 2.2, b_channel);

    r_channel.convertTo(r_channel, CV_8U, 255.0);
    g_channel.convertTo(g_channel, CV_8U, 255.0);
    b_channel.convertTo(b_channel, CV_8U, 255.0);
}

void write_yuv_frame(const Mat& g_channel, const Mat& b_channel, const Mat& r_channel, const string& outputFile) {
    ofstream outFile(outputFile, ios::binary | ios::app);
    if (!outFile.is_open()) {
        cerr << "Could not open output file: " << outputFile << endl;
        return;
    }

    outFile.write(reinterpret_cast<const char*>(g_channel.data), g_channel.total() * g_channel.elemSize());
    outFile.write(reinterpret_cast<const char*>(b_channel.data), b_channel.total() * b_channel.elemSize());
    outFile.write(reinterpret_cast<const char*>(r_channel.data), r_channel.total() * r_channel.elemSize());

    outFile.close();
}

void save_color_channel(const Mat& channel, const string& filename, const Scalar& color)
{
    Mat color_channel = Mat::zeros(channel.size(), CV_8UC3);
    vector<Mat> channels(3, Mat::zeros(channel.size(), CV_8UC1));
    channels[color[0]] = channel;
    merge(channels, color_channel);
    imwrite(filename, color_channel);
}

void save_histogram(const Mat& image, const string& filename) {
    int histSize = 256;
    float range[] = { 0, 256 };
    const float* histRange = { range };
    bool uniform = true;
    bool accumulate = false;
    Mat hist;
    calcHist(&image, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

    int hist_w = 512; int hist_h = 400;
    int bin_w = cvRound((double)hist_w / histSize);
    Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

    normalize(hist, hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

    for (int i = 1; i < histSize; i++) {
        line(histImage, Point(bin_w * (i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
            Point(bin_w * i, hist_h - cvRound(hist.at<float>(i))),
            Scalar(255, 255, 255), 2, 8, 0);
    }
    imwrite(filename, histImage);
}

void extract_channels(const Mat& bayer, Mat& r, Mat& g, Mat& b) {
    int rows = bayer.rows;
    int cols = bayer.cols;


    r = Mat(rows / 2, cols / 2, CV_8UC1);
    Mat g1 = Mat(rows / 2, cols / 2, CV_8UC1);
    Mat g2 = Mat(rows / 2, cols / 2, CV_8UC1);
    b = Mat(rows / 2, cols / 2, CV_8UC1);
    g = Mat(rows / 2, cols, CV_8UC1);

    for (int i = 0; i < rows; i += 2) {
        for (int j = 0; j < cols; j += 2) {
            r.at<uchar>(i / 2, j / 2) = bayer.at<uchar>(i, j);
            g1.at<uchar>(i / 2, j / 2) = bayer.at<uchar>(i, j + 1);
            g2.at<uchar>(i / 2, j / 2) = bayer.at<uchar>(i + 1, j);
            b.at<uchar>(i / 2, j / 2) = bayer.at<uchar>(i + 1, j + 1);
        }
    }

    for (int i = 0; i < g1.rows; i++) {
        for (int j = 0; j < g1.cols; j++) {
            g.at<uchar>(i, 2 * j + 1) = g1.at<uchar>(i, j);
            g.at<uchar>(i, 2 * j) = g2.at<uchar>(i, j);
        }
    }
}
