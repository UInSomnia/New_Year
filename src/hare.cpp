#include "hare.h"

namespace InSomnia
{

    Hare::Hare()
    {
        static constexpr double nan =
            std::numeric_limits<double>::signaling_NaN();
        
        // Физические параметры (в пикселях и кадрах)
        this->g_pixels_per_sec_sq = nan; // Ускорение "гравитации"
        this->vx_per_jump = nan; // Скорость вперёд за прыжок (пикс/сек)
        this->vy_initial = nan; // Начальная скорость вверх (пикс/сек)
        this->ground_y = nan; // Уровень "земли"
        
        // Начальные параметры
        this->current_x = nan; // Начальная позиция X
        this->current_y = nan; // Начальная позиция Y (на земле)
        this->is_in_air = false; // Находится ли заяц в воздухе?
        
        // Параметры прыжков
        this->jump_duration = nan; // Длительность одного прыжка
        this->jump_interval = nan; // Задержка между прыжками (сек)
        this->jump_start_x = nan; // Позиция начала текущего прыжка
        this->last_landing_time = nan; // Время последнего приземления
        this->waiting_for_next_jump = false; // Ожидаем начала следующего прыжка
    }
    
    Hare::Hare(
        const std::string &path_file_hare,
        const int width,
        const int height,
        const float scale,
        const double g_pixels_per_sec_sq,
        const double vx_per_jump,
        const double vy_initial,
        const double ground_y,
        const double start_x,
        const double jump_interval)
    {
        // Физические параметры (в пикселях и кадрах)
        this->g_pixels_per_sec_sq = g_pixels_per_sec_sq; // Ускорение "гравитации"
        this->vx_per_jump = vx_per_jump; // Скорость вперёд за прыжок (пикс/сек)
        this->vy_initial = vy_initial; // Начальная скорость вверх (пикс/сек)
        this->ground_y = ground_y; // Уровень "земли"
        
        // Начальные параметры
        this->current_x = start_x; // Начальная позиция X
        this->current_y = ground_y; // Начальная позиция Y (на земле)
        this->is_in_air = false; // Находится ли заяц в воздухе?
        
        // Параметры прыжков
        this->jump_duration = 2.0 * (-vy_initial) / g_pixels_per_sec_sq; // Длительность одного прыжка
        this->jump_interval = jump_interval; // Задержка между прыжками (сек)
        this->jump_start_x = current_x; // Позиция начала текущего прыжка
        this->last_landing_time = 0.0; // Время последнего приземления
        this->waiting_for_next_jump = false; // Ожидаем начала следующего прыжка
        
        // const std::string path_file_hare =
        //     dir_img + "/hare.png";
        
        const cv::Mat hare_img =
            cv::imread(path_file_hare, cv::IMREAD_UNCHANGED);
        
        if (hare_img.empty())
        {
            throw std::runtime_error(
                "Ошибка: не удалось загрузить файл hare.png\n");
        }
        
        const cv::Mat hare_img_rgba =
            InSomnia::convert_to_rgba(hare_img);
        
        const cv::Mat hare_img_rgba_clear =
            InSomnia::clear_alpha(hare_img_rgba);
        
        const int target_height =
            static_cast<int>(height * scale); // 0.2
        const int target_width = static_cast<int>(
            hare_img_rgba_clear.cols * (
                (float)target_height / hare_img_rgba_clear.rows));
        
        // cv::Mat hare_img_ready;
        
        cv::resize(
            hare_img_rgba_clear,
            this->img,
            cv::Size(target_width, target_height),
            0,
            0,
            cv::INTER_AREA);
    }
    
    void Hare::render(
        const uint32_t frame_idx,
        const int fps,
        cv::Mat &frame)
    {
        const double t_seconds =
            static_cast<double>(frame_idx) / fps;
        
        // Определяем, должен ли начаться новый прыжок
        if (!(this->is_in_air) &&
            !(this->waiting_for_next_jump))
        {
            // Первый прыжок начинаем сразу
            this->is_in_air = true;
            this->waiting_for_next_jump = false;
            this->jump_start_x = this->current_x; // Начинаем прыжок с текущей позиции
            this->last_landing_time = t_seconds; // Запоминаем время начала прыжка
        }
        else if (!(this->is_in_air) && this->waiting_for_next_jump)
        {
            // Ожидаем задержку между прыжками
            if (t_seconds - this->last_landing_time >= this->jump_interval)
            {
                // Задержка прошла, начинаем новый прыжок
                this->is_in_air = true;
                this->waiting_for_next_jump = false;
                this->jump_start_x = this->current_x; // Начинаем прыжок с текущей позиции
                this->last_landing_time = t_seconds; // Обновляем время начала прыжка
            }
        }
        
        if (this->is_in_air)
        {
            // Время от начала прыжка
            double jump_time = t_seconds - this->last_landing_time;
            
            // Ограничиваем время прыжка
            if (jump_time > this->jump_duration)
            {
                // Прыжок закончен, приземляемся
                jump_time = this->jump_duration;
                
                // Вычисляем финальную позицию после прыжка
                const double x =
                    this->jump_start_x + this->vx_per_jump * this->jump_duration;
                double y =
                    this->ground_y +
                    this->vy_initial * this->jump_duration +
                    0.5 * this->g_pixels_per_sec_sq *
                    this->jump_duration * this->jump_duration;
                
                // Корректируем позицию приземления
                if (y > this->ground_y)
                {
                    y = this->ground_y;
                }
                
                // Обновляем текущую позицию
                this->current_x = x;
                this->current_y = y;
                
                // Переходим в состояние ожидания следующего прыжка
                this->is_in_air = false;
                this->waiting_for_next_jump = true;
                this->last_landing_time = t_seconds; // Запоминаем время приземления для отсчёта задержки
                
                // Рисуем зайца в точке приземления
                InSomnia::draw_figure_to_frame(
                    this->img, this->current_x, this->current_y, frame);
            }
            else
            {
                // Прыжок в процессе
                // Вычисляем текущую позицию
                const double x =
                    this->jump_start_x + this->vx_per_jump * jump_time;
                double y =
                    this->ground_y +
                    this->vy_initial * jump_time +
                    0.5 * this->g_pixels_per_sec_sq * jump_time * jump_time;
                
                // Корректируем позицию, если ниже земли
                if (y > this->ground_y)
                {
                    y = this->ground_y;
                }
                
                // Рисуем зайца в текущей позиции прыжка
                InSomnia::draw_figure_to_frame(
                    this->img, x, y, frame);
            }
        }
        else
        {
            // Заяц на земле, ожидает начала следующего прыжка
            InSomnia::draw_figure_to_frame(
                this->img, this->current_x, this->current_y, frame);
        }
        
    }
    
}
