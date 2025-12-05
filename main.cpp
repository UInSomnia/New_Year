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
    // Config
    
    static constexpr int width = 3840;
    static constexpr int height = 2160;
    static constexpr int fps = 60;
    static constexpr double duration_sec = 50.0;
    static const int total_frames =
        static_cast<int>(duration_sec * fps);
    static const int type = CV_8UC3;
    static const std::string dir_img =
        "../../img";
    
    // Prepare
    
    // Snow
    
    static const std::string path_file_snowflake =
        dir_img + "/snow.png";
    
    std::vector<InSomnia::Interval_Snow> schedule_snowfall =
    {
        { 5., 10., 200, 0, 0 }
    };
    
    InSomnia::Snowfall snowfall(
        path_file_snowflake,
        schedule_snowfall,
        fps,
        total_frames);
    
    // Fir
    
    static const std::string path_file_fir =
        dir_img + "/tree.png";
    
    static constexpr float coord_fir_x = width * 0.5;
    static constexpr float coord_fir_y = height * 0.5;
    
    InSomnia::Fir fir(
        path_file_fir, width, height);
    
    // Light
    
    const cv::Mat &img_fir = fir.get_img();
    
    const std::vector<cv::Scalar> colors_lamps =
    {
        { 0, 0, 255 },   // blue
        { 0, 255, 0 },   // green
        { 255, 0, 0 },   // red
        { 0, 255, 255 }  // yellow
    };
    
    const std::vector<InSomnia::State_Lamps> vec_state_lamps =
    {
        { { false, false, false, true }, 1., 0u },
        { { false, false, true, false }, 1., 0u },
        { { false, true, false, false }, 1., 0u },
        { { true, false, false, false }, 1., 0u },
        
        { { false, false, true, true }, 1., 0u },
        { { false, true, true, false }, 1., 0u },
        { { true, true, false, false }, 1., 0u },
        { { true, false, false, true }, 1., 0u },
        
        { { false, true, true, true }, 0.5, 0u },
        { { true, true, true, false }, 0.5, 0u },
        { { true, true, false, true }, 0.5, 0u },
        { { true, false, true, true }, 0.5, 0u }
    };
    
    static constexpr double scale = 0.003;
    
    InSomnia::Light light(
        img_fir,
        colors_lamps,
        vec_state_lamps,
        width,
        height,
        fps,
        scale);
    
    const uint32_t count_lamps = 50u;
    light.generate_lights_inside_tree_by_alpha(count_lamps);
    
    light.convert_tree_coords_to_frame_coords(
        coord_fir_x, coord_fir_y);
    
    // Video
    
    static const std::string path_file_video =
        "result.mp4";
    
    cv::VideoWriter video_writer(
        path_file_video,
        cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 
        fps,
        cv::Size(width, height));
    
    if (!video_writer.isOpened())
    {
        throw std::runtime_error(
            "Ошибка: не удалось открыть VideoWriter\n");
    }
    
    for (uint32_t frame_idx = 0u;
         frame_idx < total_frames;
         ++frame_idx)
    {
        // const cv::Mat &frame = vec_frames[frame_idx];
        
        cv::Mat frame =
            cv::Mat::zeros(height, width, type);
        
        
        snowfall.render(
            frame_idx,
            width,
            height,
            frame);
        
        
        fir.render(
            frame_idx,
            coord_fir_x,
            coord_fir_y,
            frame);
        
        light.render(
            frame_idx,
            frame);
        
        
        video_writer.write(frame);
        
        if ((frame_idx + 1) % fps == 0)
        {
            std::cout << std::format(
                "Записано кадров: {} из {}\n",
                frame_idx + 1, total_frames);
            std::cout.flush();
        }
    }
    
    video_writer.release();
    
    std::cout << "Видео сохранено как " << path_file_video << "\n";
    std::cout.flush();
    
    return 0;
}
