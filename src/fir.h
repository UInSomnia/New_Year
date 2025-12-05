#ifndef INSOMNIA_FIR_H
#define INSOMNIA_FIR_H

#include <opencv2/opencv.hpp>

#include "toolbox.h"

namespace InSomnia
{
    class Fir
    {
    public:
        Fir();
        
        Fir(
            const std::string &path_file,
            const int width,
            const int height);
        
        // void generate_fir(
        //     const int fps,
        //     const float x,
        //     const float y,
        //     std::vector<cv::Mat> &vec_frames) const;
        
        void render(
            const uint32_t frame_idx,
            const float x,
            const float y,
            cv::Mat &frame);
        
        const cv::Mat& get_img() const;
        
    private:
        cv::Mat img;
        
        void load(
            const std::string &path_file,
            const int width,
            const int height);
    };
}

#endif
