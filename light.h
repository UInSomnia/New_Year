#ifndef INSOMNIA_LIGHT_H
#define INSOMNIA_LIGHT_H

// #include <iostream>
#include <vector>
#include <random>
#include <limits>

#include <opencv2/opencv.hpp>

namespace InSomnia
{
    struct Group_Lamps
    {
        cv::Scalar color;
        std::vector<cv::Point2f> lights;
    };
    
    struct State_Lamps
    {
        std::vector<bool> vec_state_color;
        double duration;
        uint32_t limit_frames;
    };
    
    class Light
    {
    public:
        
        Light();
        
        Light(
            const cv::Mat &tree,
            const std::vector<cv::Scalar> &colors,
            const std::vector<InSomnia::State_Lamps> &vec_state_lamps,
            const int width,
            const int height,
            const int fps,
            const double scale);
        
        void generate_lights_inside_tree_by_alpha(
            const int num_lights);
        
        void convert_tree_coords_to_frame_coords(
            const float tree_pos_x,
            const float tree_pos_y);
        
        // void generate_light(
        //     const int fps,
        //     const std::vector<State_Lamps> &vec_state_lamps,
        //     std::vector<cv::Mat> &vec_frames);
        
        void render(
            const uint32_t frame_idx,
            cv::Mat &frame);
        
    private:
        cv::Mat tree_img;
        std::vector<Group_Lamps> vec_groups_lamps;
        std::vector<InSomnia::State_Lamps> vec_state_lamps;
        
        uint32_t count_colors;
        uint32_t count_state_lamps;
        double diagonal;
        double radius_base;
        uint32_t num_mode;
    };
}

#endif
