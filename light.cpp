#include "light.h"

namespace InSomnia
{
    Light::Light()
    {
        
    }
    
    Light::Light(
        const cv::Mat &tree,
        const std::vector<cv::Scalar> &colors)
    {
        this->tree_img = tree.clone();
        // cv::imwrite("tree_img.png", this->tree_img);
        
        const uint32_t count_colors =
            colors.size();
        
        this->vec_groups_lamps =
            std::vector<Group_Lamps>(count_colors);
        
        for (uint32_t i = 0u; i < count_colors; ++i)
        {
            const cv::Scalar &s = colors[i];
            Group_Lamps &g = this->vec_groups_lamps[i];
            
            g.color = s;
        }
    }
    
    void Light::generate_lights_inside_tree_by_alpha(
        const int num_lights)
    {
        // std::vector<cv::Point2f> lights;
    
        if (this->tree_img.channels() != 4)
        {
            throw std::runtime_error(
                "Ошибка: изображение должно быть в формате BGRA!\n");
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> x_dist(0, this->tree_img.cols - 1);
        std::uniform_int_distribution<int> y_dist(0, this->tree_img.rows - 1);
                
        for (Group_Lamps &g : this->vec_groups_lamps)
        {
            g.lights = std::vector<cv::Point2f>(num_lights);
            
            uint32_t index_fill = 0u;
            
            for (;;)
            {
                const int x = x_dist(gen);
                const int y = y_dist(gen);
                
                // Получаем пиксель (B, G, R, A)
                const cv::Vec4b &pixel =
                    this->tree_img.at<cv::Vec4b>(y, x);
                
                // Проверяем альфа-канал: если > 0 — значит, не прозрачный (ёлка!)
                if (pixel[3] > 0)
                {
                    cv::Point2f point(x, y);
                    g.lights[index_fill] = std::move(point);
                    ++index_fill;
                }
                
                if (index_fill >= num_lights)
                {
                    break;
                }
                
            }
        }
        
        // std::cout << "Count: " << lights.size() << "\n";
        
        // for (const cv::Point2f &p : this->lights)
        // {
        //     std::cout << p.x << " -> " << p.y << "\n";
        // }
        
        // std::cout.flush();
        
    }
    
    void Light::convert_tree_coords_to_frame_coords(
        const float tree_pos_x,
        const float tree_pos_y)
    {
        static constexpr float scale = 1.f;
        
        for (Group_Lamps &g : this->vec_groups_lamps)
        {
            std::vector<cv::Point2f> &lights = g.lights;
            
            const uint32_t count = lights.size();
            
            std::vector<cv::Point2f> frame_coords(count);
            
            for (uint32_t i = 0u; i < count; ++i)
            {
                const cv::Point2f &pt = lights[i];
                
                const float frame_x =
                    pt.x * scale + tree_pos_x - (this->tree_img.cols) / 2.f;
                const float frame_y =
                    pt.y * scale + tree_pos_y - (this->tree_img.rows) / 2.f;
                
                cv::Point2f point(frame_x, frame_y);
                frame_coords[i] = std::move(point);
            }
            
            lights = std::move(frame_coords);
            
        }
        
    }
    
    void Light::generate_light(
        const int fps,
        // const double time_one_color,
        const std::vector<State_Lamps> &vec_state_lamps,
        std::vector<cv::Mat> &vec_frames)
    {
        if (vec_frames.empty() == true)
        {
            return;
        }
        
        const cv::Mat &frame_0 = vec_frames.front();
        const double diagonal =
            std::sqrt(
                frame_0.rows * frame_0.rows +
                frame_0.cols * frame_0.cols);
        
        const uint32_t radius_base =
            diagonal * 0.003;
        
        const uint32_t total_frames = vec_frames.size();
        
        const uint32_t count_colors =
            this->vec_groups_lamps.size();
        
        const uint32_t count_state_lamps =
            vec_state_lamps.size();
        
        // const uint32_t limit_frames_one_color =
        //     time_one_color * fps;
        
        // uint32_t num_color = 0u;
        
        uint32_t num_mode = 0u;
        
        for (uint32_t frame_idx = 0u; frame_idx < total_frames; ++frame_idx)
        {
            cv::Mat &frame = vec_frames[frame_idx];
            
            // Рисуем огоньки
            
            const uint32_t idx_mode = num_mode % count_state_lamps;
            const State_Lamps &mode = vec_state_lamps[idx_mode];
            
            const std::vector<bool> &vec_state_color =
                mode.vec_state_color;
            const double duration = mode.duration;
            const uint32_t limit_frames_mode =
                duration * fps;
            
            if (vec_state_color.size() != count_colors)
            {
                throw std::runtime_error(
                    "vec_state_color is incorrect");
            }
            
            const uint32_t radius =
                // radius_base + 2. * num_mode / count_state_lamps;
                radius_base * (1. + 0.5 * num_mode / count_state_lamps);
            
            for (uint32_t i = 0u; i < count_colors; ++i)
            {
                const bool b = vec_state_color[i];
                if (b)
                {
                    const Group_Lamps &g = this->vec_groups_lamps[i];
                    const std::vector<cv::Point2f> &lights = g.lights;
                    const cv::Scalar &color = g.color;
                    for (const cv::Point2f &pt : lights)
                    { 
                        cv::circle(frame, pt, radius, color, -1);
                    }
                }
            }
            
            // const uint32_t idx_color = num_color % count_colors;
            // const Group_Lamps &g = this->vec_groups_lamps[idx_color];
            // const std::vector<cv::Point2f> &lights = g.lights;
            // const cv::Scalar &color = g.color;
            // for (const cv::Point2f &pt : lights)
            // { 
            //     cv::circle(frame, pt, 3, color, -1);
            // }
            
            if ((frame_idx + 1) % limit_frames_mode == 0)
            {
                ++num_mode;
            }
            
            if ((frame_idx + 1) % fps == 0)
            {
                std::cout << std::format(
                    "Запись гирлянды. Обработано кадров: {} из {}\n",
                    frame_idx + 1, total_frames);
                std::cout.flush();
            }
        }
    }
    
}
