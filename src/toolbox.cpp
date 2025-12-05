#include "toolbox.h"

namespace InSomnia
{
    cv::Mat clear_alpha(const cv::Mat img)
    {
        cv::Mat result = img.clone();
        
        // Чистим альфа-канал: если альфа < 30, делаем полностью прозрачным
        if (result.channels() == 4)
        {
            for (uint32_t y = 0; y < result.rows; ++y)
            {
                for (uint32_t x = 0; x < result.cols; ++x)
                {
                    cv::Vec4b &pixel = result.at<cv::Vec4b>(y, x);
                    if (pixel[3] < 30)
                    {
                        pixel[3] = 0;
                    }
                }
            }
        }
        
        return result;
    }
    
    // Смешиваем пиксель с фоном по альфа-каналу (корректно для BGR)
    void blend_pixel(
        cv::Mat &frame,
        int32_t x,
        int32_t y,
        const cv::Vec4b &pixel)
    {
        if (x < 0 || x >= frame.cols || y < 0 || y >= frame.rows) {
            return;
        }
    
        cv::Vec3b bg = frame.at<cv::Vec3b>(y, x);
        float alpha = pixel[3] / 255.0f;
    
        cv::Vec3b result;
        for (int i = 0; i < 3; ++i) {
            result[i] = static_cast<uchar>(
                alpha * pixel[i] + (1 - alpha) * bg[i]
            );
        }
    
        frame.at<cv::Vec3b>(y, x) = result;
    }
    
    cv::Mat convert_to_rgba(const cv::Mat &input)
    {
        cv::Mat img_rgba;
        if (input.channels() == 3)
        {
            cv::cvtColor(input, img_rgba, cv::COLOR_BGR2BGRA);
        }
        else if (input.channels() == 4)
        {
            img_rgba = input;
        }
        else
        {
            throw std::runtime_error(
                "Неподдерживаемое количество каналов в изображении");
        }
        return img_rgba;
    }
    
    void draw_figure_to_frame(
        const cv::Mat &figure,
        const float x,
        const float y,
        cv::Mat &frame)
    {
        if (figure.empty() ||
            figure.channels() != 4 ||
            frame.empty() ||
            frame.channels() != 3)
        {
            return;
        }
        
        const uint32_t r = figure.rows;
        const uint32_t c = figure.cols;
        const uint32_t width = frame.cols;
        const uint32_t height = frame.rows;
        
        // ✅ Теперь x, y — это центр изображения
        const int x_offset = static_cast<int>(x - c / 2.0f);
        const int y_offset = static_cast<int>(y - r / 2.0f);
        
        // Ограничиваем start/end, чтобы не выйти за пределы figure
        const int start_dy = std::max(0, std::min((int)r, -y_offset));
        const int end_dy   = std::min((int)r, std::max(0, (int)height - y_offset));
        const int start_dx = std::max(0, std::min((int)c, -x_offset));
        const int end_dx   = std::min((int)c, std::max(0, (int)width - x_offset));
        
        for (int dy = start_dy; dy < end_dy; ++dy)
        {
            for (int dx = start_dx; dx < end_dx; ++dx)
            {
                const int32_t px = x_offset + dx;
                const int32_t py = y_offset + dy;
    
                if (px < 0 || px >= width || py < 0 || py >= height)
                {
                    continue;
                }
    
                // Проверка: не выходим ли за пределы figure
                if (dy >= r || dx >= c)
                {
                    continue;
                }
    
                const cv::Vec4b pixel = figure.at<cv::Vec4b>(dy, dx);
                if (pixel[3] > 0)
                {
                    blend_pixel(frame, px, py, pixel);
                }
            }
        }
    }
    
    std::vector<cv::Mat> prepare_frames(
        const int width,
        const int height,
        const int total_frames,
        const int type)
    {
        std::vector<cv::Mat> vec_frames(total_frames);
        
        for (uint32_t frame_idx = 0u;
             frame_idx < total_frames;
             ++frame_idx)
        {
            cv::Mat frame =
                cv::Mat::zeros(height, width, type /*CV_8UC3*/); // чёрный фон
            vec_frames[frame_idx] = std::move(frame);
        }
        
        return vec_frames;
    }
    
    void write_video_to_file(
        const std::string &path_file,
        const int width,
        const int height,
        const int fps,
        const std::vector<cv::Mat> &vec_frames)
    {
        const uint32_t total_frames = vec_frames.size();
        
        cv::VideoWriter video_writer(
            path_file,
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
            const cv::Mat &frame = vec_frames[frame_idx];
            
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
        
        std::cout << "Видео сохранено как " << path_file << "\n";
        std::cout.flush();
    }
    
}
