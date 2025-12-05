#ifndef INSOMNIA_HARE_H
#define INSOMNIA_HARE_H

#include <string>
#include <limits>

#include <opencv2/opencv.hpp>

#include "toolbox.h"

namespace InSomnia
{
    class Hare
    {
    public:
        Hare();
        
        Hare(
            const std::string &path_file_hare,
            const int width,
            const int height,
            const float scale,
            const double g_pixels_per_sec_sq,
            const double vx_per_jump,
            const double vy_initial,
            const double ground_y,
            const double start_x,
            const double jump_interval);
        
        void render(
            const uint32_t frame_idx,
            const int fps,
            cv::Mat &frame);
        
    private:
        cv::Mat img;
        
        double g_pixels_per_sec_sq; // Ускорение "гравитации"
        double vx_per_jump; // Скорость вперёд за прыжок (пикс/сек)
        double vy_initial; // Начальная скорость вверх (пикс/сек)
        double ground_y; // Уровень "земли"
        
        double current_x; // Начальная позиция X
        double current_y; // Начальная позиция Y (на земле)
        bool is_in_air; // Находится ли заяц в воздухе?
        
        double jump_duration; // Длительность одного прыжка
        double jump_interval; // Задержка между прыжками (сек)
        double jump_start_x; // Позиция начала текущего прыжка
        double last_landing_time; // Время последнего приземления
        bool waiting_for_next_jump; // Ожидаем начала следующего прыжка
    };
}

#endif
