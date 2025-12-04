#include "snowflake.h"

namespace InSomnia
{
    Snowflake::Snowflake()
    {
        static constexpr double nan =
            std::numeric_limits<double>::signaling_NaN();
        
        this->need_remove = false;
        
        this->scale = nan;
        this->rotation = nan;
        this->rotation_speed = nan;
    }
    
    Snowflake::Snowflake(
        const uint32_t width,
        const uint32_t height,
        const cv::Mat &original_img)
    {
        this->need_remove = false;
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        static std::uniform_real_distribution<float>
            speed_y(0.5f, 4.0f);
        static std::uniform_real_distribution<float>
            speed_x(-0.5f, 0.5f);
        
        this->velocity.y = speed_y(gen);
        this->velocity.x = speed_x(gen);
        
        static std::uniform_real_distribution<float>
            small_dist(0.01f, 0.05f);  // мелкие
        static std::uniform_real_distribution<float>
            middle_dist(0.05f, 0.12f);  // средние
        static std::uniform_real_distribution<float>
            large_dist(0.12f, 0.16f);  // большие
        static std::uniform_real_distribution<float>
            chance(0.0f, 1.0f);         // шанс
        
        const float ch = chance(gen);
        
        // 0.98 -> 1.0
        if (ch > 0.98f)
        {
            this->scale = large_dist(gen);
        }
        // 0.2 -> 0.98
        else if (ch > 0.2f)
        {
            this->scale = middle_dist(gen);
        }
        // 0.0 -> 0.2
        else
        {
            this->scale = small_dist(gen);
        }
        
        const int target_height =
            static_cast<int>(height * this->scale);
        const int target_width = static_cast<int>(
            original_img.cols * (
                (float)target_height / original_img.rows));
        
        cv::Mat resized;
        cv::resize(
            original_img,
            resized,
            cv::Size(target_width, target_height),
            0,
            0,
            cv::INTER_AREA);
        
        const float diagonal =
            std::sqrt(target_width * target_width +
                      target_height * target_height);
        
        static std::uniform_int_distribution<int>
            x_dist(0, width - 1);
        this->pos.x = static_cast<float>(x_dist(gen));
        this->pos.y = -diagonal;
        
        this->base_img = resized.clone();
        static std::uniform_real_distribution<float>
            rotation_dist(0.f, 360.f);
        this->rotation = static_cast<float>(rotation_dist(gen));
        static std::uniform_real_distribution<float>
            rotation_speed_dist(-2.f, 2.f);
        this->rotation_speed =
            static_cast<float>(rotation_speed_dist(gen));
        
    }
    
    void Snowflake::move()
    {
        this->pos.x += this->velocity.x;
        this->pos.y += this->velocity.y;
        this->rotation += this->rotation_speed;
    }
    
    void Snowflake::rotate()
    {
        cv::Point2f center(
            this->base_img.cols / 2.0f,
            this->base_img.rows / 2.0f);
        
        cv::Mat rotation_matrix =
            cv::getRotationMatrix2D(center, this->rotation, 1.0);
        
        // cv::Mat rotated_img;
        
        cv::warpAffine(
            this->base_img,
            this->rotated_img,
            rotation_matrix, 
            this->base_img.size(),
            cv::INTER_LINEAR,
            cv::BORDER_CONSTANT,
            cv::Scalar(0,0,0,0));
        
        // return rotated_img;
    }
    
    void Snowflake::draw_to_frame(cv::Mat &frame)
    {
        draw_figure_to_frame(
            this->rotated_img,
            this->pos.x,
            this->pos.y,
            frame);
    }
    
    bool Snowflake::is_out_frame(const uint32_t height)
    {
        const bool is_out =
            this->pos.y - this->rotated_img.rows / 2.0f > height;
        
        return is_out;
    }
    
    void Snowflake::set_remove()
    {
        this->need_remove = true;
    }
    
    bool Snowflake::get_remove() const
    {
        return this->need_remove;
    }
    
    void Snowflake::generate_snow(
        const std::string &path_file,
        const int width,
        const int height,
        const int fps,
        std::vector<Interval_Snow> &schedule,
        // const int num_snowflakes,
        std::vector<cv::Mat> &vec_frames)
    {
        const uint32_t total_frames = vec_frames.size();
        
        const cv::Mat snow_img =
            cv::imread(path_file, cv::IMREAD_UNCHANGED);
        
        // const cv::Mat snow_img =
        //     cv::imread("../../img/snow.png", cv::IMREAD_UNCHANGED);
        
        if (snow_img.empty())
        {
            throw std::runtime_error(
                "Ошибка: не удалось загрузить файл snow.png\n");
        }
        
        const cv::Mat snow_img_rgba =
            InSomnia::convert_to_rgba(snow_img);
        
        const cv::Mat snow_img_rgba_clear =
            InSomnia::clear_alpha(snow_img_rgba);
        
        if (schedule.empty() == true)
        {
            throw std::runtime_error(
                "Ошибка: schedule пуст\n");
        }
        
        for (Interval_Snow &interval : schedule)
        {
            interval.idx_frame_start =
                interval.time_start * fps;
            
            if (interval.idx_frame_finish < 0.)
            {
                interval.idx_frame_finish = total_frames;
            }
            else
            {
                interval.idx_frame_finish =
                    interval.time_finish * fps;
            }
        }
        
        std::vector<InSomnia::Snowflake> snowflakes;
        
        const uint32_t time_create_snowflake = 3u;
        
        bool is_active = false;
        
        uint32_t idx_schedule = 0u;
        
        for (uint32_t frame_idx = 0u; frame_idx < total_frames; ++frame_idx)
        {
            cv::Mat &frame = vec_frames[frame_idx];
            
            uint32_t num_snowflakes = 0u;
            
            if (idx_schedule < schedule.size())
            {
                if (frame_idx > schedule[idx_schedule].idx_frame_finish)
                {
                    ++idx_schedule;
                }
            }
            
            if (idx_schedule >= schedule.size())
            {
                // throw std::runtime_error(
                //     "idx_schedule >= schedule.size()");
                
                is_active = false;
                num_snowflakes = 0u;
            }
            else
            {
                // std::cout << "idx_schedule: " << idx_schedule << "\n";
                // std::cout.flush();
                
                const Interval_Snow &interval =
                    schedule[idx_schedule];
                
                if (interval.idx_frame_start == frame_idx)
                {
                    is_active = true;
                }
                if (interval.idx_frame_finish == frame_idx)
                {
                    is_active = false;
                }
                
                num_snowflakes = interval.count_snowflakes;
            }
            
            for (InSomnia::Snowflake &sf : snowflakes)
            {
                sf.move();
                
                sf.rotate();
                
                sf.draw_to_frame(frame);
                
                // Перезапуск снежинки при выходе за нижнюю границу
                if (sf.is_out_frame(height))
                {
                    if (is_active == true)
                    {
                        sf = Snowflake(
                            width, height, snow_img_rgba_clear);
                    }
                    else if (snowflakes.size() > 0)
                    {
                        sf.set_remove();
                    }
                }
            }
            
            if (is_active == false && snowflakes.size() > 0)
            {
                const std::vector<InSomnia::Snowflake>::iterator
                it_new_end = std::remove_if(
                    snowflakes.begin(),
                    snowflakes.end(),
                [](const Snowflake &s) -> bool
                {
                    return s.get_remove();
                });
                
                snowflakes.erase(it_new_end, snowflakes.end());
            }
            
            if (snowflakes.size() < num_snowflakes &&
                frame_idx % time_create_snowflake == 0 &&
                is_active == true)
            {
                snowflakes.emplace_back(width, height, snow_img_rgba_clear);
            }
            
            if ((frame_idx + 1) % fps == 0)
            {
                std::cout << std::format(
                    "Запись снежинок. Обработано кадров: {} из {}\n",
                    frame_idx + 1, total_frames);
                std::cout.flush();
            }
        }
    }
    
}
