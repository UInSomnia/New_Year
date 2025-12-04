#include <opencv2/opencv.hpp>
#include <iostream>
#include <format>

#include "snowflake.h"
#include "fir.h"
#include "light.h"
#include "toolbox.h"

// Добавить блеск снежинок

int main()
{
    static constexpr int width = 1280;
    static constexpr int height = 720;
    static constexpr int fps = 60;
    static constexpr double duration_sec = 60.0;
    static const int total_frames =
        static_cast<int>(duration_sec * fps);
    
    static const int type = CV_8UC3;
    
    std::vector<cv::Mat> vec_frames =
        InSomnia::prepare_frames(
            width,
            height,
            total_frames,
            type);
    
    static const std::string path_file_snowflake =
        "../../img/snow.png";
    // static constexpr int num_snowflakes = 200;
    
    std::vector<InSomnia::Interval_Snow> schedule =
    {
        { 5., 10., 200, 0, 0 }
    };
    
    InSomnia::Snowflake::generate_snow(
        path_file_snowflake,
        width,
        height,
        fps,
        schedule,
        // num_snowflakes,
        vec_frames);
    
    static const std::string path_file_fir =
        "../../img/tree.png";
    
    static constexpr float coord_fir_x = width * 0.5;
    static constexpr float coord_fir_y = height * 0.5;
    
    InSomnia::Fir fir;
    
    fir.load(
        path_file_fir,
        width,
        height);
    
    fir.generate_fir(
        fps,
        coord_fir_x,
        coord_fir_y,
        vec_frames);
    
    const cv::Mat &img_fir = fir.get_img();
    
    const std::vector<cv::Scalar> colors_lamps =
    {
        { 0, 0, 255 },   // blue
        { 0, 255, 0 },   // green
        { 255, 0, 0 },   // red
        { 0, 255, 255 }  // yellow
    };
    
    InSomnia::Light light(img_fir, colors_lamps);
    
    const uint32_t count_lamps = 50u;
    light.generate_lights_inside_tree_by_alpha(count_lamps);
    
    light.convert_tree_coords_to_frame_coords(
        coord_fir_x, coord_fir_y);
    
    // const double time_one_color = 2.;
    const std::vector<InSomnia::State_Lamps> vec_state_lamps =
    {
        { { false, false, false, true }, 1. },
        { { false, false, true, false }, 1. },
        { { false, true, false, false }, 1. },
        { { true, false, false, false }, 1. },
        
        { { false, false, true, true }, 1. },
        { { false, true, true, false }, 1. },
        { { true, true, false, false }, 1. },
        { { true, false, false, true }, 1. },
        
        { { false, true, true, true }, 0.5 },
        { { true, true, true, false }, 0.5 },
        { { true, true, false, true }, 0.5 },
        { { true, false, true, true }, 0.5 }
    };
    light.generate_light(
        fps, vec_state_lamps, vec_frames);
    
    static const std::string path_file_video =
        "result.mp4";
    
    InSomnia::write_video_to_file(
        path_file_video,
        width,
        height,
        fps,
        vec_frames);
    
    return 0;
}
