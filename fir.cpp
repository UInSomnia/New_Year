#include "fir.h"

namespace InSomnia
{
    
    Fir::Fir()
    {
        
    }
    
    void Fir::load(
        const std::string &path_file,
        const int width,
        const int height)
    {
        const cv::Mat tree_img =
            cv::imread(path_file, cv::IMREAD_UNCHANGED);
        
        // const cv::Mat tree_img =
        //     cv::imread("../../img/tree.png", cv::IMREAD_UNCHANGED);
        
        if (tree_img.empty())
        {
            throw std::runtime_error(
                "Ошибка: не удалось загрузить файл tree.png\n");
        }
        
        const cv::Mat tree_img_rgba =
            InSomnia::convert_to_rgba(tree_img);
        
        const cv::Mat tree_img_rgba_clear =
            InSomnia::clear_alpha(tree_img_rgba);
        
        const int target_height =
            static_cast<int>(height * 0.8);
        const int target_width = static_cast<int>(
            tree_img_rgba_clear.cols * (
                (float)target_height / tree_img_rgba_clear.rows));
        
        // cv::Mat tree_img_rgba_clear_resized;
        cv::resize(
            tree_img_rgba_clear,
            this->img,
            cv::Size(target_width, target_height),
            0,
            0,
            cv::INTER_AREA);  
    }
    
    void Fir::generate_fir(
        const int fps,
        const float x,
        const float y,
        std::vector<cv::Mat> &vec_frames) const
    {
        const uint32_t total_frames = vec_frames.size();  
        
        for (uint32_t frame_idx = 0u; frame_idx < total_frames; ++frame_idx)
        {
            cv::Mat &frame = vec_frames[frame_idx];
            
            InSomnia::draw_figure_to_frame(
                this->img,
                x,
                y,
                frame);
            
            if ((frame_idx + 1) % fps == 0)
            {
                std::cout << std::format(
                    "Запись ёлки. Обработано кадров: {} из {}\n",
                    frame_idx + 1, total_frames);
                std::cout.flush();
            }
        }
    }
    
    const cv::Mat& Fir::get_img() const
    {
        return this->img;
    }
    
}
