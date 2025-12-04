#ifndef INSOMNIA_SNOWFLAKE_H
#define INSOMNIA_SNOWFLAKE_H

#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include <opencv2/opencv.hpp>

#include "toolbox.h"

namespace InSomnia
{
    struct Interval_Snow
    {
        float time_start;
        float time_finish;
        uint32_t count_snowflakes;
        
        uint32_t idx_frame_start;
        uint32_t idx_frame_finish;
    };
    
    class Snowflake
    {
    public:
        Snowflake();
        
        Snowflake(
            const uint32_t width,
            const uint32_t height,
            const cv::Mat &original_img);
        
        void move();
        
        void rotate();
                
        void draw_to_frame(cv::Mat &frame);
        
        bool is_out_frame(const uint32_t height);
        
        void set_remove();
        
        bool get_remove() const;
        
        static void generate_snow(
            const std::string &path_file,
            const int width,
            const int height,
            const int fps,
            std::vector<Interval_Snow> &schedule,
            // const int num_snowflakes,
            std::vector<cv::Mat> &vec_frames);
        
    private:
        bool need_remove;
        
        cv::Point2f pos;
        cv::Point2f velocity;
        
        float scale;
        float rotation;
        float rotation_speed;
        
        cv::Mat base_img;
        cv::Mat rotated_img;
    };
}

#endif
