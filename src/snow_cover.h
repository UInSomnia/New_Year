#ifndef INSOMNIA_SNOW_COVER_H
#define INSOMNIA_SNOW_COVER_H

// #include <iostream>
// #include <format>

#include <vector>
#include <random>

#include <opencv2/opencv.hpp>

namespace InSomnia
{
    struct Snowball
    {
        float x;
        float y;
        float radius;
    };
    
    class Snow_Cover
    {
    public:
        Snow_Cover();
        
        void render(
            const uint32_t frame_idx,
            const uint32_t total_frames,
            cv::Mat &frame);
        
    private:
        std::vector<Snowball> vec_snowballs;
        
        int width;
        int height;
        float diagonal;
        
        float scale;
        float base_radius;
        uint32_t limit_snowballs;
        cv::Scalar color;
        
        float max_y_lift;
        float min_y_lift;
        float current_y_lift;
    };
}

#endif
