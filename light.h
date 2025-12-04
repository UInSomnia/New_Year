#ifndef INSOMNIA_LIGHT_H
#define INSOMNIA_LIGHT_H

// #include <iostream>
#include <vector>
#include <random>

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
    };
    
    class Light
    {
    public:
        
        Light();
        
        Light(
            const cv::Mat &tree,
            const std::vector<cv::Scalar> &colors);
        
        void generate_lights_inside_tree_by_alpha(
            const int num_lights);
        
        void convert_tree_coords_to_frame_coords(
            const float tree_pos_x,
            const float tree_pos_y);
        
        void generate_light(
            const int fps,
            // const double time_one_color,
            const std::vector<State_Lamps> &vec_state_lamps,
            std::vector<cv::Mat> &vec_frames);
        
    private:
        cv::Mat tree_img;
        std::vector<Group_Lamps> vec_groups_lamps;
    };
}

#endif
