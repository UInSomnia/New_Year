#ifndef INSOMNIA_TOOLBOX_H
#define INSOMNIA_TOOLBOX_H

#include <opencv2/opencv.hpp>

namespace InSomnia
{
    cv::Mat clear_alpha(const cv::Mat img);
    
    void blend_pixel(
        cv::Mat &frame,
        int32_t x,
        int32_t y,
        const cv::Vec4b &pixel);
    
    cv::Mat convert_to_rgba(const cv::Mat &input);
    
    // Можно оптимизировать
    void draw_figure_to_frame(
        const cv::Mat &figure,
        const float x,
        const float y,
        cv::Mat &frame);
    
    std::vector<cv::Mat> prepare_frames(
        const int width,
        const int height,
        const int total_frames,
        const int type);
    
    void write_video_to_file(
        const std::string &path_file,
        const int width,
        const int height,
        const int fps,
        const std::vector<cv::Mat> &vec_frames);
}

#endif
