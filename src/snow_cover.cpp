#include "snow_cover.h"

namespace InSomnia
{

    Snow_Cover::Snow_Cover()
    {
        this->width = 3840;
        this->height = 2160;
        this->diagonal =
            std::sqrt(this->width * this->width +
                      this->height * this->height);
        
        this->scale = 0.0015;
        this->base_radius =
            this->diagonal * this->scale;
        this->limit_snowballs = 15'000u;
        this->color = cv::Scalar(200, 200, 200);
        
        this->max_y_lift = this->height * 1.f / 3.f;
        this->min_y_lift = this->height * 0.95f;
        this->current_y_lift = this->min_y_lift;
    }
    
    void Snow_Cover::render(
        const uint32_t frame_idx,
        const uint32_t total_frames,
        cv::Mat &frame)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        std::uniform_real_distribution<float>
            dist_pos_x(0.f, this->width);
        
        this->current_y_lift =
            this->min_y_lift -
            (static_cast<float>(frame_idx) / (total_frames - 1)) *
            (this->min_y_lift - this->max_y_lift);
        
        // std::cout << std::format(
        //     "max_y_lift: {} "
        //     "min_y_lift: {} "
        //     "current_y_lift: {}\n",
        //     this->max_y_lift, this->min_y_lift, this->current_y_lift);
        // std::cout.flush();
        
        std::uniform_real_distribution<float>
            dist_pos_y(this->current_y_lift, this->height);
        
        const uint32_t count_need =
            this->limit_snowballs *
                (static_cast<float>(frame_idx) / (total_frames - 1));
        
        const uint32_t count_has =
            this->vec_snowballs.size();
        
        const uint32_t count_gen =
            count_need - count_has;
        
        // std::cout << std::format(
        //     "count_need: {}\n"
        //     "count_has: {}\n"
        //     "count_gen: {}\n\n",
        //     count_need, count_has, count_gen);
        // std::cout.flush();
        
        for (uint32_t i = 0u; i < count_gen; ++i)
        {
            Snowball snowball;
            
            snowball.x = dist_pos_x(gen);
            snowball.y = dist_pos_y(gen);
            snowball.radius = this->base_radius;
            
            this->vec_snowballs.push_back(
                std::move(snowball));
        }
        
        for (const Snowball &sb : this->vec_snowballs)
        {
            const float x = sb.x;
            const float y = sb.y;
            const float radius = sb.radius;
            
            cv::circle(
                frame,
                cv::Point2f(x, y),
                radius,
                this->color,
                -1);
        }
        
    }
    
}
