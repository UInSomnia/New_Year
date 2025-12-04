#include <opencv2/opencv.hpp>
#include <vector>
#include <random>
#include <iostream>

struct Snowflake {
    cv::Point2f pos;
    cv::Point2f velocity;
    float scale;
    float rotation;
    float rotation_speed;
    cv::Mat rotated_img;

    Snowflake(int width, int height, const cv::Mat& original_img) {
        pos.x = static_cast<float>(rand() % width);
        pos.y = -50.0f;

        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> speed_y(1.0f, 3.0f);
        static std::uniform_real_distribution<float> speed_x(-0.5f, 0.5f);

        velocity.y = speed_y(gen);
        velocity.x = speed_x(gen);

        // Масштаб: от 5% до 20% от высоты кадра
        static std::uniform_real_distribution<float> scale_dist(0.05f, 0.10f);
        scale = scale_dist(gen);

        int target_height = static_cast<int>(height * scale);
        int target_width = static_cast<int>(original_img.cols * ((float)target_height / original_img.rows));

        cv::Mat resized;
        cv::resize(original_img, resized, cv::Size(target_width, target_height), 0, 0, cv::INTER_AREA);

        // Чистим альфа-канал: если альфа < 30, делаем полностью прозрачным
        if (resized.channels() == 4) {
            for (int y = 0; y < resized.rows; ++y) {
                for (int x = 0; x < resized.cols; ++x) {
                    cv::Vec4b& pixel = resized.at<cv::Vec4b>(y, x);
                    if (pixel[3] < 30) {
                        pixel[3] = 0;
                    }
                }
            }
        }

        rotated_img = resized.clone();
        rotation = static_cast<float>(rand() % 360);
        rotation_speed = static_cast<float>(rand() % 4 - 2); // -2..+2
    }
};

// Смешиваем пиксель с фоном по альфа-каналу (корректно для BGR)
void blendPixel(cv::Mat& frame, int x, int y, const cv::Vec4b& overlay_pixel) {
    if (overlay_pixel[3] == 0) return;

    cv::Vec3b bg = frame.at<cv::Vec3b>(y, x);
    float alpha = overlay_pixel[3] / 255.0f;

    cv::Vec3b result;
    for (int i = 0; i < 3; ++i) {
        result[i] = static_cast<uchar>(
            alpha * overlay_pixel[i] + (1 - alpha) * bg[i]
        );
    }

    frame.at<cv::Vec3b>(y, x) = result;
}

int main()
{
    // Загружаем с прозрачностью
    cv::Mat snow_img = cv::imread("../../img/snow.png", cv::IMREAD_UNCHANGED);
    if (snow_img.empty())
    {
        std::cerr << "Ошибка: не удалось загрузить файл snow.png" << std::endl;
        return -1;
    }

    cv::Mat img_rgba;
    if (snow_img.channels() == 3) {
        cv::cvtColor(snow_img, img_rgba, cv::COLOR_BGR2BGRA);
    } else if (snow_img.channels() == 4) {
        img_rgba = snow_img;
    } else {
        std::cerr << "Неподдерживаемое количество каналов в изображении" << std::endl;
        return -1;
    }

    const int width = 1280;
    const int height = 720;
    const int num_snowflakes = 700;
    const int fps = 60;
    const double duration_sec = 20.0;
    const int total_frames = static_cast<int>(duration_sec * fps);
    
    cv::VideoWriter video_writer("snowfall_blue.mp4", 
                                 cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 
                                 fps, cv::Size(width, height));

    if (!video_writer.isOpened()) {
        std::cerr << "Ошибка: не удалось открыть VideoWriter" << std::endl;
        return -1;
    }

    std::vector<Snowflake> snowflakes;
    for (int i = 0; i < num_snowflakes; ++i)
    {
        snowflakes.emplace_back(width, height, img_rgba);
    }

    for (int frame_idx = 0; frame_idx < total_frames; ++frame_idx) {
        cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC3); // чёрный фон

        for (auto& sf : snowflakes)
        {
            sf.pos.x += sf.velocity.x;
            sf.pos.y += sf.velocity.y;
            sf.rotation += sf.rotation_speed;

            // Поворачиваем изображение
            cv::Point2f center(sf.rotated_img.cols / 2.0f, sf.rotated_img.rows / 2.0f);
            cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, sf.rotation, 1.0);
            cv::Mat rotated_img;
            cv::warpAffine(sf.rotated_img, rotated_img, rotation_matrix, 
                          sf.rotated_img.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0,0,0,0));

            int x = static_cast<int>(sf.pos.x - rotated_img.cols / 2.0f);
            int y = static_cast<int>(sf.pos.y - rotated_img.rows / 2.0f);

            if (x + rotated_img.cols > 0 && x < width && 
                y + rotated_img.rows > 0 && y < height)
            {

                for (int dy = 0; dy < rotated_img.rows; ++dy)
                {
                    for (int dx = 0; dx < rotated_img.cols; ++dx)
                    {
                        int px = x + dx;
                        int py = y + dy;

                        if (px < 0 || px >= width || py < 0 || py >= height) continue;

                        cv::Vec4b pixel = rotated_img.at<cv::Vec4b>(dy, dx);
                        if (pixel[3] > 0) {
                            blendPixel(frame, px, py, pixel);
                        }
                    }
                }
            }

            // Перезапуск снежинки при выходе за нижнюю границу
            if (sf.pos.y - sf.rotated_img.rows / 2.0f > height)
            {
                sf.pos.y = -50.0f;
                sf.pos.x = static_cast<float>(rand() % width);

                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::uniform_real_distribution<float> speed_y(1.0f, 3.0f);
                static std::uniform_real_distribution<float> speed_x(-0.5f, 0.5f);
                static std::uniform_real_distribution<float> scale_dist(0.05f, 0.10f);

                sf.velocity.y = speed_y(gen);
                sf.velocity.x = speed_x(gen);
                sf.scale = scale_dist(gen);

                int target_height = static_cast<int>(height * sf.scale);
                int target_width = static_cast<int>(img_rgba.cols * ((float)target_height / img_rgba.rows));
                cv::Mat resized;
                cv::resize(img_rgba, resized, cv::Size(target_width, target_height), 0, 0, cv::INTER_AREA);

                if (resized.channels() == 4) {
                    for (int y = 0; y < resized.rows; ++y) {
                        for (int x = 0; x < resized.cols; ++x) {
                            cv::Vec4b& pixel = resized.at<cv::Vec4b>(y, x);
                            if (pixel[3] < 30) pixel[3] = 0;
                        }
                    }
                }

                sf.rotated_img = resized.clone();
            }
        }

        video_writer.write(frame);

        if (frame_idx % fps == 0)
        {
            std::cout << "Обработано кадров: " << frame_idx << " из " << total_frames << std::endl;
        }
    }

    video_writer.release();
    std::cout << "Видео сохранено как snowfall_blue.mp4" << std::endl;

    return 0;
}







// #include <opencv2/opencv.hpp>
// #include <vector>
// #include <random>
// #include <iostream>

// struct Snowflake
// {
//     cv::Point2f pos;
//     cv::Point2f velocity;
//     float scale;
//     float rotation;
//     float rotation_speed;
//     cv::Mat rotated_img;

//     Snowflake(int width, int height, const cv::Mat& original_img) {
//         pos.x = static_cast<float>(rand() % width);
//         pos.y = -50.0f;

//         static std::random_device rd;
//         static std::mt19937 gen(rd());
//         static std::uniform_real_distribution<float> speed_y(1.0f, 3.0f);
//         static std::uniform_real_distribution<float> speed_x(-0.5f, 0.5f);

//         velocity.y = speed_y(gen);
//         velocity.x = speed_x(gen);

//         // Масштаб: от 5% до 20% от высоты кадра
//         static std::uniform_real_distribution<float> scale_dist(0.01f, 0.05f);
//         scale = scale_dist(gen);

//         int target_height = static_cast<int>(height * scale);
//         int target_width = static_cast<int>(original_img.cols * ((float)target_height / original_img.rows));

//         cv::Mat resized;
//         cv::resize(original_img, resized, cv::Size(target_width, target_height), 0, 0, cv::INTER_AREA);

//         // Чистим альфа-канал: если альфа < 30, делаем полностью прозрачным
//         if (resized.channels() == 4) {
//             for (int y = 0; y < resized.rows; ++y) {
//                 for (int x = 0; x < resized.cols; ++x) {
//                     cv::Vec4b& pixel = resized.at<cv::Vec4b>(y, x);
//                     if (pixel[3] < 30) { // порог прозрачности
//                         pixel[3] = 0;
//                     }
//                 }
//             }
//         }

//         rotated_img = resized.clone();
//         rotation = static_cast<float>(rand() % 360);
//         rotation_speed = static_cast<float>(rand() % 4 - 2); // -2..+2
//     }
// };

// // Функция для смешивания пикселя с фоном с учётом альфа
// void blendPixel(cv::Mat& frame, int x, int y, const cv::Vec4b& overlay_pixel)
// {
//     if (overlay_pixel[3] == 0) return; // полностью прозрачный

//     cv::Vec3b bg = frame.at<cv::Vec3b>(y, x);
//     float alpha = overlay_pixel[3] / 255.0f;

//     // Смешиваем цвета: result = alpha * overlay + (1 - alpha) * background
//     cv::Vec3b result;
//     for (int i = 0; i < 3; ++i) {
//         result[i] = static_cast<uchar>(
//             alpha * overlay_pixel[2-i] + (1 - alpha) * bg[i]
//         );
//     }

//     frame.at<cv::Vec3b>(y, x) = result;
// }

// int main()
// {
//     cv::Mat snow_img = cv::imread("../../img/snow.png", cv::IMREAD_UNCHANGED);
//     if (snow_img.empty())
//     {
//         std::cerr << "Ошибка: не удалось загрузить файл snow.png" << std::endl;
//         return -1;
//     }
    
//     cv::Mat img_rgba;
//     if (snow_img.channels() == 3) {
//         cv::cvtColor(snow_img, img_rgba, cv::COLOR_BGR2BGRA);
//     } else if (snow_img.channels() == 4) {
//         img_rgba = snow_img;
//     } else {
//         std::cerr << "Неподдерживаемое количество каналов в изображении" << std::endl;
//         return -1;
//     }

//     const int width = 1280;
//     const int height = 720;
//     const int num_snowflakes = 200;
//     const int fps = 30;
//     const double duration_sec = 10.0;
//     const int total_frames = static_cast<int>(duration_sec * fps);
    
//     cv::VideoWriter video_writer("snowfall_clean.mp4", 
//                                  cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 
//                                  fps, cv::Size(width, height));

//     if (!video_writer.isOpened())
//     {
//         std::cerr << "Ошибка: не удалось открыть VideoWriter" << std::endl;
//         return -1;
//     }

//     std::vector<Snowflake> snowflakes;
//     for (int i = 0; i < num_snowflakes; ++i)
//     {
//         snowflakes.emplace_back(width, height, img_rgba);
//     }

//     for (int frame_idx = 0; frame_idx < total_frames; ++frame_idx)
//     {
//         cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC3);

//         for (auto& sf : snowflakes)
//         {
//             sf.pos.x += sf.velocity.x;
//             sf.pos.y += sf.velocity.y;
//             sf.rotation += sf.rotation_speed;

//             // Поворачиваем изображение
//             cv::Point2f center(sf.rotated_img.cols / 2.0f, sf.rotated_img.rows / 2.0f);
//             cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, sf.rotation, 1.0);
//             cv::Mat rotated_img;
//             cv::warpAffine(sf.rotated_img, rotated_img, rotation_matrix, 
//                           sf.rotated_img.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0,0,0,0));

//             int x = static_cast<int>(sf.pos.x - rotated_img.cols / 2.0f);
//             int y = static_cast<int>(sf.pos.y - rotated_img.rows / 2.0f);

//             if (x + rotated_img.cols > 0 && x < width && 
//                 y + rotated_img.rows > 0 && y < height) {

//                 for (int dy = 0; dy < rotated_img.rows; ++dy) {
//                     for (int dx = 0; dx < rotated_img.cols; ++dx) {
//                         int px = x + dx;
//                         int py = y + dy;

//                         if (px < 0 || px >= width || py < 0 || py >= height) continue;

//                         cv::Vec4b pixel = rotated_img.at<cv::Vec4b>(dy, dx);
//                         if (pixel[3] > 0) {
//                             blendPixel(frame, px, py, pixel);
//                         }
//                     }
//                 }
//             }

//             // Перезапуск снежинки
//             if (sf.pos.y - sf.rotated_img.rows / 2.0f > height) {
//                 sf.pos.y = -50.0f;
//                 sf.pos.x = static_cast<float>(rand() % width);

//                 static std::random_device rd;
//                 static std::mt19937 gen(rd());
//                 static std::uniform_real_distribution<float> speed_y(1.0f, 3.0f);
//                 static std::uniform_real_distribution<float> speed_x(-0.5f, 0.5f);
//                 static std::uniform_real_distribution<float> scale_dist(0.01f, 0.05f);
                
//                 sf.velocity.y = speed_y(gen);
//                 sf.velocity.x = speed_x(gen);
//                 sf.scale = scale_dist(gen);

//                 int target_height = static_cast<int>(height * sf.scale);
//                 int target_width = static_cast<int>(img_rgba.cols * ((float)target_height / img_rgba.rows));
//                 cv::Mat resized;
//                 cv::resize(img_rgba, resized, cv::Size(target_width, target_height), 0, 0, cv::INTER_AREA);

//                 // Чистим альфа-канал
//                 if (resized.channels() == 4) {
//                     for (int y = 0; y < resized.rows; ++y) {
//                         for (int x = 0; x < resized.cols; ++x) {
//                             cv::Vec4b& pixel = resized.at<cv::Vec4b>(y, x);
//                             if (pixel[3] < 30) pixel[3] = 0;
//                         }
//                     }
//                 }

//                 sf.rotated_img = resized.clone();
//             }
//         }

//         video_writer.write(frame);

//         if (frame_idx % fps == 0) {
//             std::cout << "Обработано кадров: " << frame_idx << " из " << total_frames << std::endl;
//         }
//     }

//     video_writer.release();
//     std::cout << "Видео сохранено как snowfall_clean.mp4" << std::endl;

//     return 0;
// }





// // #include <opencv2/opencv.hpp>
// // #include <vector>
// // #include <random>
// // #include <iostream>

// // struct Snowflake
// // {
// //     cv::Point2f pos;
// //     cv::Point2f velocity;
// //     float scale;          // относительный масштаб (0.0 - 1.0)
// //     float rotation;
// //     float rotation_speed;
// //     cv::Mat rotated_img;

// //     Snowflake(int width, int height, const cv::Mat& original_img) {
// //         // Начальная позиция сверху
// //         pos.x = static_cast<float>(rand() % width);
// //         pos.y = -50.0f;

// //         // Скорость падения и горизонтальное движение
// //         static std::random_device rd;
// //         static std::mt19937 gen(rd());
// //         static std::uniform_real_distribution<float> speed_y(1.0f, 3.0f);
// //         static std::uniform_real_distribution<float> speed_x(-0.5f, 0.5f);

// //         velocity.y = speed_y(gen);
// //         velocity.x = speed_x(gen);

// //         // Масштаб: от 5% до 20% от высоты кадра
// //         static std::uniform_real_distribution<float> scale_dist(0.05f, 0.20f);
// //         scale = scale_dist(gen);

// //         // Вычисляем целевой размер по высоте
// //         int target_height = static_cast<int>(height * scale);
// //         int target_width = static_cast<int>(original_img.cols * ((float)target_height / original_img.rows));

// //         // Масштабируем изображение
// //         cv::resize(original_img, rotated_img, cv::Size(target_width, target_height), 0, 0, cv::INTER_AREA);

// //         // Случайное вращение
// //         static std::uniform_real_distribution<float> angle_dist(0.0f, 360.0f);
// //         static std::uniform_real_distribution<float> rotation_speed_dist(-2.0f, 2.0f);
// //         rotation = angle_dist(gen);
// //         rotation_speed = rotation_speed_dist(gen);
// //     }
// // };

// // int main()
// // {
// //     cv::Mat snow_img = cv::imread("../../img/snow.png", cv::IMREAD_UNCHANGED);
// //     if (snow_img.empty()) {
// //         std::cerr << "Ошибка: не удалось загрузить файл snow.png" << std::endl;
// //         return -1;
// //     }

// //     cv::Mat img_rgba;
// //     if (snow_img.channels() == 3)
// //     {
// //         cv::cvtColor(snow_img, img_rgba, cv::COLOR_BGR2BGRA);
// //     }
// //     else if (snow_img.channels() == 4)
// //     {
// //         img_rgba = snow_img;
// //     }
// //     else
// //     {
// //         std::cerr << "Неподдерживаемое количество каналов в изображении" << std::endl;
// //         return -1;
// //     }
    
// //     const int width = 1280;
// //     const int height = 720;
// //     const int num_snowflakes = 50;
// //     const int fps = 30;
// //     const double duration_sec = 10.0;
// //     const int total_frames = static_cast<int>(duration_sec * fps);

// //     cv::VideoWriter video_writer("snowfall.mp4", 
// //                                  cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 
// //                                  fps, cv::Size(width, height));

// //     if (!video_writer.isOpened()) {
// //         std::cerr << "Ошибка: не удалось открыть VideoWriter" << std::endl;
// //         return -1;
// //     }

// //     std::vector<Snowflake> snowflakes;
// //     for (int i = 0; i < num_snowflakes; ++i)
// //     {
// //         snowflakes.emplace_back(width, height, img_rgba);
// //     }
    
// //     for (int frame_idx = 0; frame_idx < total_frames; ++frame_idx)
// //     {
// //         cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC3);

// //         for (auto& sf : snowflakes) {
// //             sf.pos.x += sf.velocity.x;
// //             sf.pos.y += sf.velocity.y;
// //             sf.rotation += sf.rotation_speed;

// //             // Поворот изображения
// //             cv::Point2f center(sf.rotated_img.cols / 2.0f, sf.rotated_img.rows / 2.0f);
// //             cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, sf.rotation, 1.0);
// //             cv::Mat rotated_img;
// //             cv::warpAffine(sf.rotated_img, rotated_img, rotation_matrix, 
// //                           sf.rotated_img.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);

// //             int x = static_cast<int>(sf.pos.x - rotated_img.cols / 2.0f);
// //             int y = static_cast<int>(sf.pos.y - rotated_img.rows / 2.0f);

// //             if (x + rotated_img.cols > 0 && x < width && 
// //                 y + rotated_img.rows > 0 && y < height) {

// //                 for (int dy = 0; dy < rotated_img.rows; ++dy) {
// //                     for (int dx = 0; dx < rotated_img.cols; ++dx) {
// //                         int px = x + dx;
// //                         int py = y + dy;

// //                         if (px < 0 || px >= width || py < 0 || py >= height) continue;

// //                         cv::Vec4b pixel = rotated_img.at<cv::Vec4b>(dy, dx);
// //                         if (pixel[3] > 0) { // если не прозрачный
// //                             cv::Vec3b color = {pixel[2], pixel[1], pixel[0]};
// //                             frame.at<cv::Vec3b>(py, px) = color;
// //                         }
// //                     }
// //                 }
// //             }

// //             // Перезапуск снежинки, когда она уходит за нижнюю границу
// //             if (sf.pos.y - sf.rotated_img.rows / 2.0f > height) {
// //                 sf.pos.y = -50.0f;
// //                 sf.pos.x = static_cast<float>(rand() % width);

// //                 // Перегенерация скорости и масштаба для разнообразия
// //                 static std::random_device rd;
// //                 static std::mt19937 gen(rd());
// //                 static std::uniform_real_distribution<float> speed_y(1.0f, 3.0f);
// //                 static std::uniform_real_distribution<float> speed_x(-0.5f, 0.5f);
// //                 static std::uniform_real_distribution<float> scale_dist(0.05f, 0.20f);

// //                 sf.velocity.y = speed_y(gen);
// //                 sf.velocity.x = speed_x(gen);
// //                 sf.scale = scale_dist(gen);

// //                 int target_height = static_cast<int>(height * sf.scale);
// //                 int target_width = static_cast<int>(img_rgba.cols * ((float)target_height / img_rgba.rows));
// //                 cv::resize(img_rgba, sf.rotated_img, cv::Size(target_width, target_height), 0, 0, cv::INTER_AREA);
// //             }
// //         }

// //         video_writer.write(frame);

// //         if (frame_idx % fps == 0) {
// //             std::cout << "Обработано кадров: " << frame_idx << " из " << total_frames << std::endl;
// //         }
// //     }

// //     video_writer.release();
// //     std::cout << "Видео сохранено как snowfall.mp4" << std::endl;

// //     return 0;
// // }






// // #include <opencv2/opencv.hpp>
// // #include <vector>
// // #include <random>
// // #include <iostream>

// // // Структура для хранения информации о снежинке
// // struct Snowflake
// // {
// //     cv::Point2f pos;      // текущая позиция (x, y)
// //     cv::Point2f velocity;  // скорость (vx, vy)
// //     float scale;          // масштаб (для разных размеров)
// //     float rotation;       // текущий угол поворота
// //     float rotation_speed; // скорость вращения
// //     cv::Mat rotated_img;  // повернутое изображение снежинки

// //     Snowflake(int width, int height, const cv::Mat& original_img, 
// //               float min_scale = 0.3f, float max_scale = 1.0f) {
// //         // Случайная позиция сверху
// //         pos.x = static_cast<float>(rand() % width);
// //         pos.y = -50.0f; // начинаем немного выше экрана

// //         // Случайная скорость падения и горизонтальное движение
// //         static std::random_device rd;
// //         static std::mt19937 gen(rd());
// //         static std::uniform_real_distribution<float> speed_y(1.0f, 3.0f);
// //         static std::uniform_real_distribution<float> speed_x(-0.5f, 0.5f);

// //         velocity.y = speed_y(gen);
// //         velocity.x = speed_x(gen);

// //         // Случайный масштаб
// //         static std::uniform_real_distribution<float> scale_dist(min_scale, max_scale);
// //         scale = scale_dist(gen);

// //         // Случайное вращение
// //         static std::uniform_real_distribution<float> angle_dist(0.0f, 360.0f);
// //         static std::uniform_real_distribution<float> rotation_speed_dist(-2.0f, 2.0f);
// //         rotation = angle_dist(gen);
// //         rotation_speed = rotation_speed_dist(gen);

// //         // Масштабируем изображение снежинки
// //         cv::resize(original_img, rotated_img, cv::Size(), scale, scale, cv::INTER_LINEAR);
// //     }
// // };

// // int main()
// // {
// //     // Загружаем изображение снежинки с прозрачностью (если есть)
// //     cv::Mat snow_img = cv::imread(
// //         "../../img/snow.png", cv::IMREAD_UNCHANGED);
// //     if (snow_img.empty()) {
// //         std::cerr << "Ошибка: не удалось загрузить файл snow.png" << std::endl;
// //         return -1;
// //     }
    
// //     // Если изображение не в формате BGRA, конвертируем его для работы с альфа-каналом
// //     cv::Mat img_rgba;
// //     if (snow_img.channels() == 3) {
// //         cv::cvtColor(snow_img, img_rgba, cv::COLOR_BGR2BGRA);
// //     } else if (snow_img.channels() == 4) {
// //         img_rgba = snow_img;
// //     } else {
// //         std::cerr << "Неподдерживаемое количество каналов в изображении" << std::endl;
// //         return -1;
// //     }

// //     const int width = 1280; // ширина кадра
// //     const int height = 720; // высота кадра
// //     const int num_snowflakes = 50; // количество снежинок
// //     const int fps = 30; // кадры в секунду
// //     const double duration_sec = 10.0; // длительность видео в секундах
// //     const int total_frames = static_cast<int>(duration_sec * fps);
    
// //     // Подготовка VideoWriter
// //     cv::VideoWriter video_writer("snowfall.mp4", 
// //                                  cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 
// //                                  fps, cv::Size(width, height));

// //     if (!video_writer.isOpened())
// //     {
// //         std::cerr << "Ошибка: не удалось открыть VideoWriter" << std::endl;
// //         return -1;
// //     }

// //     // Создаем начальные снежинки
// //     std::vector<Snowflake> snowflakes;
// //     for (int i = 0; i < num_snowflakes; ++i)
// //     {
// //         snowflakes.emplace_back(width, height, img_rgba);
// //     }

// //     // Генерация кадров
// //     for (int frame_idx = 0; frame_idx < total_frames; ++frame_idx) {
// //         // Создаем пустой кадр (черный фон)
// //         cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC3);

// //         for (auto& sf : snowflakes) {
// //             // Обновляем позицию
// //             sf.pos.x += sf.velocity.x;
// //             sf.pos.y += sf.velocity.y;

// //             // Обновляем угол поворота
// //             sf.rotation += sf.rotation_speed;

// //             // Поворачиваем изображение снежинки
// //             cv::Point2f center(sf.rotated_img.cols / 2.0f, sf.rotated_img.rows / 2.0f);
// //             cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, sf.rotation, 1.0);
// //             cv::Mat rotated_img;
// //             cv::warpAffine(sf.rotated_img, rotated_img, rotation_matrix, 
// //                           sf.rotated_img.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);

// //             // Рассчитываем позицию для вставки на основной кадр
// //             int x = static_cast<int>(sf.pos.x - rotated_img.cols / 2.0f);
// //             int y = static_cast<int>(sf.pos.y - rotated_img.rows / 2.0f);

// //             // Проверяем, что снежинка в пределах кадра
// //             if (x + rotated_img.cols > 0 && x < width && 
// //                 y + rotated_img.rows > 0 && y < height) {

// //                 // Накладываем снежинку на кадр с учетом альфа-канала
// //                 for (int dy = 0; dy < rotated_img.rows; ++dy) {
// //                     for (int dx = 0; dx < rotated_img.cols; ++dx) {
// //                         int px = x + dx;
// //                         int py = y + dy;

// //                         if (px < 0 || px >= width || py < 0 || py >= height) continue;

// //                         cv::Vec4b pixel = rotated_img.at<cv::Vec4b>(dy, dx);
// //                         if (pixel[3] > 0) { // если пиксель не прозрачный
// //                             cv::Vec3b color = {pixel[2], pixel[1], pixel[0]}; // BGRA -> BGR
// //                             frame.at<cv::Vec3b>(py, px) = color;
// //                         }
// //                     }
// //                 }
// //             }

// //             // Если снежинка упала за нижнюю границу, возвращаем ее наверх
// //             if (sf.pos.y - sf.rotated_img.rows / 2.0f > height) {
// //                 sf.pos.y = -50.0f;
// //                 sf.pos.x = static_cast<float>(rand() % width);
// //                 // Можно также обновить скорость и масштаб для разнообразия
// //                 static std::random_device rd;
// //                 static std::mt19937 gen(rd());
// //                 static std::uniform_real_distribution<float> speed_y(1.0f, 3.0f);
// //                 static std::uniform_real_distribution<float> speed_x(-0.5f, 0.5f);
// //                 sf.velocity.y = speed_y(gen);
// //                 sf.velocity.x = speed_x(gen);
// //             }
// //         }

// //         // Записываем кадр в видео
// //         video_writer.write(frame);

// //         // Выводим прогресс
// //         if (frame_idx % fps == 0)
// //         {
// //             std::cout << "Обработано кадров: " << frame_idx << " из " << total_frames << std::endl;
// //         }
// //     }

// //     video_writer.release();
// //     std::cout << "Видео сохранено как snowfall.mp4" << std::endl;

// //     return 0;
// // }








// // #include <iostream>
// // #include <random>

// // #include <opencv2/opencv.hpp>

// // // sudo apt update
// // // sudo apt install libopencv-dev

// // // Глобальная снежинка (загружается один раз)
// // cv::Mat snowflake;

// // void overlayImage(
// //     cv::Mat& background, cv::Mat& foreground, cv::Point2i location);

// // struct Snowflake
// // {
// //     cv::Point pos;      // текущая позиция
// //     double speed;       // скорость падения
// //     double drift;       // горизонтальное смещение (ветер)
// //     int size;           // масштаб (для разного размера)
// //     bool active;        // активна ли снежинка

// //     Snowflake(int width, int height) : active(true)
// //     {
// //         pos.x = rand() % width;
// //         pos.y = -50; // начинаем выше экрана
// //         speed = 1.0 + static_cast<double>(rand()) / RAND_MAX * 2.0; // 1.0–3.0
// //         drift = -0.5 + static_cast<double>(rand()) / RAND_MAX * 1.0; // -0.5–0.5
// //         size = 1 + rand() % 3; // 1, 2 или 3 — для разных масштабов
// //     }

// //     void update(int frame_height)
// //     {
// //         if (!active) return;
// //         pos.y += static_cast<int>(speed);
// //         pos.x += static_cast<int>(drift);
// //         if (pos.y > frame_height + 50) { // исчезаем ниже экрана
// //             active = false;
// //         }
// //     }

// //     void draw(cv::Mat& frame)
// //     {
// //         if (!active) return;

// //         // Масштабируем снежинку
// //         cv::Mat scaled;
// //         cv::resize(snowflake, scaled, cv::Size(), size, size);

// //         // Позиция верхнего левого угла
// //         int x = pos.x - scaled.cols / 2;
// //         int y = pos.y - scaled.rows / 2;

// //         // Если снежинка видима на экране
// //         if (x < frame.cols && y < frame.rows && x + scaled.cols > 0 && y + scaled.rows > 0) {
// //             // Наложение с учётом alpha-канала
// //             overlayImage(frame, scaled, cv::Point(x, y));
// //         }
// //     }
// // };

// // // Реализация overlayImage
// // void overlayImage(cv::Mat& background, const cv::Mat& foreground, cv::Point2i location)
// // {
// //     // Обрезаем область
// //     cv::Rect roi_rect(location, foreground.size());
// //     if (roi_rect.x < 0 || roi_rect.y < 0 ||
// //         roi_rect.x + roi_rect.width > background.cols ||
// //         roi_rect.y + roi_rect.height > background.rows) {
// //         return;
// //     }

// //     // ROI фона (целочисленный)
// //     cv::Mat bg_roi = background(roi_rect);

// //     // Если foreground без альфа-канала — добавим его
// //     cv::Mat fg_with_alpha = (foreground.channels() == 4) ? foreground : cv::Mat();
// //     if (foreground.channels() == 3) {
// //         std::vector<cv::Mat> with_alpha = {foreground, cv::Mat::ones(foreground.size(), CV_8UC1) * 255};
// //         cv::merge(with_alpha, fg_with_alpha);
// //     } else {
// //         fg_with_alpha = foreground;
// //     }

// //     // Разделяем на B,G,R,A
// //     std::vector<cv::Mat> fg_channels(4);
// //     cv::split(fg_with_alpha, fg_channels);
// //     cv::Mat alpha = fg_channels[3];

// //     // Конвертируем альфу в [0,1] float
// //     cv::Mat alpha_f;
// //     alpha.convertTo(alpha_f, CV_32F, 1.0 / 255.0);

// //     // Конвертируем фон и передний план в float
// //     std::vector<cv::Mat> bg_channels;
// //     cv::split(bg_roi, bg_channels);
// //     for (int i = 0; i < 3; ++i) {
// //         bg_channels[i].convertTo(bg_channels[i], CV_32F);
// //         fg_channels[i].convertTo(fg_channels[i], CV_32F);
// //     }

// //     // Смешиваем: blended = fg * alpha + bg * (1 - alpha)
// //     std::vector<cv::Mat> result_channels(3);
// //     for (int i = 0; i < 3; ++i) {
// //         result_channels[i] = fg_channels[i].mul(alpha_f) + bg_channels[i].mul(1.0 - alpha_f);
// //         result_channels[i].convertTo(result_channels[i], CV_8U); // обратно в uint8
// //     }

// //     cv::merge(result_channels, bg_roi);
// // }

// // int main()
// // {
// //     const int width = 3840;
// //     const int height = 2160;
// //     const double fps = 100;
// //     const int total_frames = 500; // 8 секунд при 15 FPS

// //     // Загрузка снежинки
// //     snowflake = cv::imread("../../img/snow.png", cv::IMREAD_UNCHANGED);
// //     if (snowflake.empty())
// //     {
// //         std::cerr << "❌ Не удалось загрузить снежинку\n";
// //         return -1;
// //     }

// //     cv::VideoWriter writer("snowfall.mp4",
// //         cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
// //         fps,
// //         cv::Size(width, height),
// //         true);

// //     std::vector<Snowflake> snowflakes;
// //     int max_snowflakes = 100;

// //     for (int frame_id = 0; frame_id < total_frames; ++frame_id)
// //     {
// //         cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC3);

// //         // Добавляем новые снежинки
// //         if (snowflakes.size() < max_snowflakes && rand() % 10 == 0)
// //         {
// //             snowflakes.emplace_back(width, height);
// //         }

// //         // Обновляем и рисуем все снежинки
// //         for (auto& flake : snowflakes)
// //         {
// //             flake.update(height);
// //             flake.draw(frame);
// //         }

// //         // Удаляем неактивные снежинки
// //         snowflakes.erase(
// //             std::remove_if(snowflakes.begin(), snowflakes.end(),
// //                            [](const Snowflake& f) { return !f.active; }),
// //             snowflakes.end()
// //         );

// //         writer.write(frame);
// //     }

// //     return 0;
// // }


// // // #include <iostream>
// // // #include <random>

// // // #include <opencv2/opencv.hpp>

// // // // sudo apt update
// // // // sudo apt install libopencv-dev

// // // void overlayImage(
// // //     cv::Mat& background, cv::Mat& foreground, cv::Point2i location);

// // // // Структура снежинки
// // // struct Snowflake
// // // {
// // //     cv::Point pos;      // текущая позиция
// // //     double speed;       // скорость падения
// // //     double drift;       // горизонтальное смещение (ветер)
// // //     int size;           // масштаб (для разного размера)
// // //     bool active;        // активна ли снежинка
// // //     cv::Mat snowflake_img; // изображение снежинки

// // //     Snowflake(int width, int height, const cv::Mat& snowflake_template) : active(true)
// // //     {
// // //         pos.x = rand() % width;
// // //         pos.y = -50; // начинаем выше экрана
// // //         speed = 1.0 + static_cast<double>(rand()) / RAND_MAX * 2.0; // 1.0–3.0
// // //         drift = -0.5 + static_cast<double>(rand()) / RAND_MAX * 1.0; // -0.5–0.5
// // //         size = 1 + rand() % 3; // 1, 2 или 3 — для разных масштабов
        
// // //         // Масштабируем изображение снежинки
// // //         cv::resize(snowflake_template, snowflake_img, 
// // //                   cv::Size(snowflake_template.cols * size, snowflake_template.rows * size));
        
// // //         // Если изображение не имеет альфа-канала, добавляем его
// // //         if (snowflake_img.channels() == 3) {
// // //             cv::Mat alpha(snowflake_img.size(), CV_8UC1, cv::Scalar(255));
// // //             std::vector<cv::Mat> channels;
// // //             cv::split(snowflake_img, channels);
// // //             channels.push_back(alpha);
// // //             cv::merge(channels, snowflake_img);
// // //         }
// // //     }

// // //     void update(int frame_height)
// // //     {
// // //         if (!active) return;
// // //         pos.y += static_cast<int>(speed);
// // //         pos.x += static_cast<int>(drift);
        
// // //         // Плавное изменение горизонтального смещения (имитация ветра)
// // //         drift += (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.1;
// // //         drift = std::max(-2.0, std::min(2.0, drift)); // Ограничиваем скорость дрейфа
        
// // //         if (pos.y > frame_height + 50) { // исчезаем ниже экрана
// // //             active = false;
// // //         }
// // //     }

// // //     void draw(cv::Mat& frame)
// // //     {
// // //         if (!active) return;

// // //         // Позиция верхнего левого угла
// // //         int x = pos.x - snowflake_img.cols / 2;
// // //         int y = pos.y - snowflake_img.rows / 2;

// // //         // Если снежинка видима на экране
// // //         if (x < frame.cols && y < frame.rows && 
// // //             x + snowflake_img.cols > 0 && y + snowflake_img.rows > 0) {
// // //             // Наложение с учётом alpha-канала
// // //             overlayImage(frame, snowflake_img, cv::Point(x, y));
// // //         }
// // //     }
// // // };

// // // void draw_snowman(cv::Mat &frame, int x, int y, double scale = 1.0);

// // // void draw_house(cv::Mat &frame, int x, int y, double scale = 1.0);

// // // void draw_christmas_tree(cv::Mat &frame, int x, int y, double scale = 1.0);

// // // int main()
// // // {
// // //     const int width = 3840;
// // //     const int height = 2160;
// // //     const double fps = 100;
// // //     const int total_frames = 500; // 8 секунд при 15 FPS
    
// // //     cv::Mat snowflake_template = cv::imread("../../img/snow.png", cv::IMREAD_UNCHANGED);
// // //     if (snowflake_template.empty())
// // //     {
// // //         std::cerr << "❌ Не удалось загрузить снежинку\n";
// // //         return -1;
// // //     }
    
// // //     // Инициализация генератора случайных чисел
// // //     srand(static_cast<unsigned int>(time(nullptr)));
    
// // //     // Создаём изображение снежинки
// // //     // cv::Mat snowflake_template = createSnowflakeImage(32);
    
// // //     // Создаём видео writer
// // //     cv::VideoWriter writer("snowfall.mp4",
// // //         cv::VideoWriter::fourcc('m', 'p', '4', 'v'), // Кодек для MP4
// // //         fps,
// // //         cv::Size(width, height),
// // //         true);
    
// // //     if (!writer.isOpened()) {
// // //         std::cerr << "❌ Не удалось создать VideoWriter\n";
// // //         return -1;
// // //     }
    
// // //     std::cout << "🎥 Начало записи видео...\n";
    
// // //     std::vector<Snowflake> snowflakes;
// // //     int max_snowflakes = 150;
    
// // //     for (int frame_id = 0; frame_id < total_frames; ++frame_id)
// // //     {
// // //         // Создаём фон (тёмно-синее небо)
// // //         cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC3);
// // //         frame.setTo(cv::Scalar(30, 30, 70)); // Тёмно-синий цвет
        
// // //         // Рисуем землю (снег)
// // //         cv::rectangle(frame, 
// // //                      cv::Point(0, height - 100), 
// // //                      cv::Point(width, height), 
// // //                      cv::Scalar(240, 240, 255), -1);
        
// // //         // Рисуем ёлки
// // //         draw_christmas_tree(frame, width/3, height - 100, 3.0);
// // //         draw_christmas_tree(frame, width/2, height - 100, 4.0);
// // //         draw_christmas_tree(frame, 2*width/3, height - 100, 3.5);
        
// // //         // Добавляем новые снежинки
// // //         if (snowflakes.size() < max_snowflakes && rand() % 3 == 0) {
// // //             snowflakes.emplace_back(width, height, snowflake_template);
// // //         }
        
// // //         // Добавляем больше снежинок в начале анимации
// // //         if (frame_id < 100 && snowflakes.size() < max_snowflakes/2 && rand() % 2 == 0) {
// // //             snowflakes.emplace_back(width, height, snowflake_template);
// // //         }
        
// // //         // Обновляем и рисуем все снежинки
// // //         for (auto& flake : snowflakes) {
// // //             flake.update(height);
// // //             flake.draw(frame);
// // //         }
        
// // //         // Удаляем неактивные снежинки
// // //         snowflakes.erase(
// // //             std::remove_if(snowflakes.begin(), snowflakes.end(),
// // //                            [](const Snowflake& f) { return !f.active; }),
// // //             snowflakes.end()
// // //         );
        
// // //         // Добавляем текст
// // //         cv::putText(frame, "Winter Scene with Snowfall", 
// // //                    cv::Point(width/2 - 200, 50),
// // //                    cv::FONT_HERSHEY_DUPLEX, 1.5, 
// // //                    cv::Scalar(200, 200, 255), 2);
        
// // //         // Показываем количество снежинок
// // //         cv::putText(frame, "Snowflakes: " + std::to_string(snowflakes.size()), 
// // //                    cv::Point(50, 50),
// // //                    cv::FONT_HERSHEY_SIMPLEX, 0.7, 
// // //                    cv::Scalar(200, 200, 200), 1);
        
// // //         // Записываем кадр
// // //         writer.write(frame);
        
// // //         // Показываем прогресс
// // //         if (frame_id % 30 == 0) {
// // //             std::cout << "📹 Записано " << frame_id << " кадров (" 
// // //                       << (frame_id * 100 / total_frames) << "%)\n";
// // //         }
        
// // //         // Опционально: показываем текущий кадр
// // //         // cv::imshow("Snowfall", frame);
// // //         // if (cv::waitKey(1) == 27) break; // ESC для выхода
// // //     }
    
// // //     writer.release();
// // //     std::cout << "✅ Видео сохранено в snowfall.mp4\n";
    
// // //     // // Выбираем кодек. Для MP4 обычно 'avc1' (H.264) или 'mp4v'
// // //     // cv::VideoWriter writer("output.mp4", 
// // //     //     cv::VideoWriter::fourcc('a', 'v', 'c', '1'), 
// // //     //     fps, 
// // //     //     cv::Size(width, height), 
// // //     //     true); // true = цветное изображение

// // //     // if (!writer.isOpened())
// // //     // {
// // //     //     std::cerr << "❌ Не удалось создать VideoWriter. Проверьте, установлен ли FFmpeg.\n";
// // //     //     return -1;
// // //     // }

// // //     // std::cout << "🎥 Запись анимации...\n";
// // //     // std::cout.flush();
    
// // //     // for (int frame_id = 0; frame_id < total_frames; ++frame_id)
// // //     // {
// // //     //     // Создаём чёрный кадр
// // //     //     cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC3);

// // //     //     // Вычисляем позицию (движение по горизонтали)
// // //     //     const int x =
// // //     //         width / 2 +
// // //     //         static_cast<int>(200 * std::sin(frame_id * 0.1));
        
// // //     //     const int y = 1000;
        
// // //     //     // draw_snowman(frame, x, y, 5.);
        
// // //     //     // draw_house(frame, x + 800, 1500, 7.);
        
// // //     //     draw_christmas_tree(frame, x + 800, 1500, 5.);
        
// // //     //     // Записываем кадр
// // //     //     writer.write(frame);

// // //     //     // (опционально) показываем прогресс
// // //     //     if (frame_id % 30 == 0)
// // //     //     {
// // //     //         std::cout << "   Записано " << frame_id << " кадров\n";
// // //     //         std::cout.flush();
// // //     //     }
// // //     // }

// // //     // writer.release();
    
// // //     // std::cout << "✅ Видео сохранено в output.mp4\n";
// // //     // std::cout.flush();
    
// // //     return 0;
// // // }

// // // void draw_snowman(cv::Mat &frame, int x, int y, double scale)
// // // {
// // //     // Базовые размеры при scale = 1.0
// // //     const double base_head_radius    = 30.0;
// // //     const double base_body_radius    = 45.0;
// // //     const double base_base_radius    = 60.0;
// // //     const double base_head_offset    = 60.0;  // расстояние от туловища до головы
// // //     const double base_base_offset    = 80.0;  // расстояние от туловища до низа

// // //     // Масштабированные значения
// // //     int r_head = static_cast<int>(base_head_radius * scale);
// // //     int r_body = static_cast<int>(base_body_radius * scale);
// // //     int r_base = static_cast<int>(base_base_radius * scale);
// // //     int head_offset = static_cast<int>(base_head_offset * scale);
// // //     int base_offset = static_cast<int>(base_base_offset * scale);

// // //     // Туловище (центральная часть — базовая точка y)
// // //     cv::circle(frame, cv::Point(x, y), r_body, cv::Scalar(255, 255, 255), -1);

// // //     // Голова
// // //     cv::circle(frame, cv::Point(x, y - head_offset), r_head, cv::Scalar(255, 255, 255), -1);

// // //     // Нижняя часть (ноги/основание)
// // //     cv::circle(frame, cv::Point(x, y + base_offset), r_base, cv::Scalar(255, 255, 255), -1);

// // //     // Глаза
// // //     int eye_offset_x = static_cast<int>(10 * scale);
// // //     int eye_offset_y = static_cast<int>(5 * scale);
// // //     int eye_radius   = static_cast<int>(5 * scale);
// // //     cv::circle(frame, cv::Point(x - eye_offset_x, y - head_offset - eye_offset_y), eye_radius, cv::Scalar(0, 0, 0), -1);
// // //     cv::circle(frame, cv::Point(x + eye_offset_x, y - head_offset - eye_offset_y), eye_radius, cv::Scalar(0, 0, 0), -1);

// // //     // Рот (улыбка — дуга)
// // //     int mouth_width  = static_cast<int>(15 * scale);
// // //     int mouth_height = static_cast<int>(10 * scale);
// // //     int mouth_y      = y - static_cast<int>(55 * scale);
// // //     cv::ellipse(frame, cv::Point(x, mouth_y), cv::Size(mouth_width, mouth_height), 0, 0, 180, cv::Scalar(0, 0, 0), static_cast<int>(2 * scale));

// // //     // Нос-морковка
// // //     int nose_radius = static_cast<int>(5 * scale);
// // //     int nose_y      = y - static_cast<int>(58 * scale);
// // //     cv::circle(frame, cv::Point(x, nose_y), nose_radius, cv::Scalar(0, 140, 255), -1);

// // //     // Шарф
// // //     int scarf_width  = static_cast<int>(40 * scale);
// // //     int scarf_height = static_cast<int>(8 * scale);
// // //     int scarf_y      = y - static_cast<int>(30 * scale);
// // //     cv::ellipse(frame, cv::Point(x, scarf_y), cv::Size(scarf_width, scarf_height), 0, 0, 360, cv::Scalar(0, 0, 255), -1);

// // //     // --- Руки-палки (ветки) ---
// // //     int arm_length = static_cast<int>(70 * scale);  // длина руки
// // //     int arm_y_offset = static_cast<int>(10 * scale); // чуть ниже центра туловища
    
// // //     // Левая рука: от центра туловища влево-вверх
// // //     cv::Point left_shoulder(x, y + arm_y_offset);
// // //     cv::Point left_hand(x - arm_length, y + arm_y_offset - static_cast<int>(20 * scale));
// // //     cv::line(frame, left_shoulder, left_hand, cv::Scalar(0, 100, 0), static_cast<int>(4 * scale));
    
// // //     // Правая рука: от центра туловища вправо-вверх
// // //     cv::Point right_shoulder(x, y + arm_y_offset);
// // //     cv::Point right_hand(x + arm_length, y + arm_y_offset - static_cast<int>(20 * scale));
// // //     cv::line(frame, right_shoulder, right_hand, cv::Scalar(0, 100, 0), static_cast<int>(4 * scale));

// // //     // --- Ведро на голову (серая трапеция) ---
// // //     int bucket_height   = static_cast<int>(25 * scale);
// // //     int top_width       = static_cast<int>(30 * scale);
// // //     int bottom_width    = static_cast<int>(40 * scale);
// // //     int bucket_y_top    = y - head_offset - r_head - bucket_height; // верх ведра — над головой
// // //     int bucket_y_bottom = y - head_offset - r_head;                // низ ведра — на уровне макушки
    
// // //     // Четыре угла трапеции (по часовой стрелке)
// // //     std::vector<cv::Point> bucket = {
// // //         cv::Point(x - bottom_width / 2, bucket_y_bottom),   // нижний левый
// // //         cv::Point(x - top_width / 2,    bucket_y_top),      // верхний левый
// // //         cv::Point(x + top_width / 2,    bucket_y_top),      // верхний правый
// // //         cv::Point(x + bottom_width / 2, bucket_y_bottom)    // нижний правый
// // //     };
    
// // //     // Серый цвет (BGR): (100, 100, 100) — нейтральный серый
// // //     cv::fillConvexPoly(frame, bucket, cv::Scalar(100, 100, 100));
// // // }

// // // void draw_house(cv::Mat &frame, int x, int y, double scale)
// // // {
// // //     // Базовые размеры при scale = 1.0
// // //     const double base_width      = 160.0;
// // //     const double base_height     = 120.0;
// // //     const double roof_height     = 60.0;
// // //     const double door_width      = 30.0;
// // //     const double door_height     = 50.0;
// // //     const double window_size     = 25.0;
// // //     const double chimney_width   = 10.0;
// // //     const double chimney_height  = 30.0;

// // //     // Масштабированные значения
// // //     int w = static_cast<int>(base_width * scale);
// // //     int h = static_cast<int>(base_height * scale);
// // //     int rh = static_cast<int>(roof_height * scale);
// // //     int dw = static_cast<int>(door_width * scale);
// // //     int dh = static_cast<int>(door_height * scale);
// // //     int ws = static_cast<int>(window_size * scale);
// // //     int cw = static_cast<int>(chimney_width * scale);
// // //     int ch = static_cast<int>(chimney_height * scale);

// // //     // Координаты углов дома (стены)
// // //     cv::Point top_left(x - w / 2, y - h);
// // //     cv::Point bottom_right(x + w / 2, y);

// // //     // Стены — заполненный прямоугольник (коричневый)
// // //     cv::rectangle(frame, top_left, bottom_right, cv::Scalar(139, 69, 19), -1); // коричневый (BGR)

// // //     // Крыша — треугольник
// // //     std::vector<cv::Point> roof = {
// // //         cv::Point(x - w / 2 - static_cast<int>(10 * scale), y - h),      // левый край крыши (чуть шире)
// // //         cv::Point(x, y - h - rh),                                        // вершина крыши
// // //         cv::Point(x + w / 2 + static_cast<int>(10 * scale), y - h)       // правый край
// // //     };
// // //     cv::fillConvexPoly(frame, roof, cv::Scalar(139, 0, 0)); // тёмно-красный

// // //     // Дверь (в центре внизу)
// // //     cv::Point door_top(x - dw / 2, y - dh);
// // //     cv::Point door_bottom(x + dw / 2, y);
// // //     cv::rectangle(frame, door_top, door_bottom, cv::Scalar(100, 100, 100), -1); // серая дверь
// // //     // Ручка двери
// // //     cv::circle(frame, cv::Point(x + dw / 2 - static_cast<int>(5 * scale), y - dh / 2), 
// // //                static_cast<int>(3 * scale), cv::Scalar(0, 0, 0), -1);

// // //     // Окно (справа от центра)
// // //     cv::Point win_top(x + static_cast<int>(20 * scale), y - h + static_cast<int>(20 * scale));
// // //     cv::Point win_bottom(x + static_cast<int>(20 * scale) + ws, y - h + static_cast<int>(20 * scale) + ws);
// // //     cv::rectangle(frame, win_top, win_bottom, cv::Scalar(255, 255, 200), -1); // светло-жёлтое окно
// // //     // Крест в окне
// // //     int win_center_x = (win_top.x + win_bottom.x) / 2;
// // //     int win_center_y = (win_top.y + win_bottom.y) / 2;
// // //     cv::line(frame, cv::Point(win_top.x, win_center_y), cv::Point(win_bottom.x, win_center_y), cv::Scalar(0, 0, 0), 1);
// // //     cv::line(frame, cv::Point(win_center_x, win_top.y), cv::Point(win_center_x, win_bottom.y), cv::Scalar(0, 0, 0), 1);

// // //     // Труба (слева на крыше)
// // //     cv::Point chimney_top(x - static_cast<int>(40 * scale), y - h - ch);
// // //     cv::Point chimney_bottom(x - static_cast<int>(40 * scale) + cw, y - h);
// // //     cv::rectangle(frame, chimney_top, chimney_bottom, cv::Scalar(80, 80, 80), -1); // тёмно-серая труба

// // //     // Дым из трубы (три полупрозрачных круга — но OpenCV не поддерживает прозрачность напрямую,
// // //     // поэтому просто рисуем светло-серый дым)
// // //     int smoke_y = y - h - ch - static_cast<int>(10 * scale);
// // //     int smoke_offset = static_cast<int>(15 * scale);
// // //     cv::circle(frame, cv::Point(x - static_cast<int>(35 * scale), smoke_y), static_cast<int>(8 * scale), cv::Scalar(200, 200, 200), -1);
// // //     cv::circle(frame, cv::Point(x - static_cast<int>(30 * scale), smoke_y - smoke_offset), static_cast<int>(10 * scale), cv::Scalar(180, 180, 180), -1);
// // //     cv::circle(frame, cv::Point(x - static_cast<int>(25 * scale), smoke_y - 2 * smoke_offset), static_cast<int>(12 * scale), cv::Scalar(160, 160, 160), -1);
// // // }

// // // // OK вид как будь то сверху
// // // void draw_christmas_tree(cv::Mat &frame, int x, int y, double scale)
// // // {
// // //     // Инициализация генератора случайных чисел
// // //     std::random_device rd;
    
// // //     static std::mt19937 rng(rd());
// // //     std::uniform_real_distribution<double> dist_angle(0.0, 2 * M_PI);
// // //     std::uniform_real_distribution<double> dist_length(0.0, 1.0);

// // //     // === Ствол ===
// // //     cv::Scalar brown(139, 69, 19); // коричневый (BGR)
// // //     int trunk_height = static_cast<int>(30 * scale);
// // //     int trunk_width = static_cast<int>(8 * scale);
// // //     cv::rectangle(frame,
// // //         cv::Point(x - trunk_width/2, y),
// // //         cv::Point(x + trunk_width/2, y - trunk_height),  // ⬆️ Ствол растёт вверх!
// // //         brown, -1);

// // //     // === Крона — конус из случайных иголок ===
// // //     cv::Scalar green(20, 100, 20); // тёмно-зелёный (BGR)
// // //     int crown_radius_base = static_cast<int>(60 * scale); // радиус основания кроны
// // //     int crown_height = static_cast<int>(100 * scale);     // высота кроны

// // //     // Центральная ось кроны
// // //     cv::Point crown_top(x, y - trunk_height); // верх ствола
// // //     cv::Point crown_bottom(x, y - trunk_height - crown_height); // низ кроны

// // //     // Количество "слоёв" (по высоте)
// // //     int layers = 15;
// // //     for (int layer = 0; layer < layers; ++layer) {
// // //         double t = static_cast<double>(layer) / (layers - 1); // 0..1
// // //         int layer_y = static_cast<int>(crown_top.y + t * (crown_bottom.y - crown_top.y));
// // //         int layer_radius = static_cast<int>(crown_radius_base * (1 - t)); // уменьшается к верху

// // //         // Количество иголок на слое
// // //         int needles_per_layer = static_cast<int>(20 * (1 - t) * scale);
// // //         for (int i = 0; i < needles_per_layer; ++i) {
// // //             double angle = dist_angle(rng); // случайный угол
// // //             double length = dist_length(rng) * 8 * scale; // длина иголки

// // //             // Точка начала иголки — на окружности слоя
// // //             cv::Point start(
// // //                 static_cast<int>(x + layer_radius * std::cos(angle)),
// // //                 static_cast<int>(layer_y + layer_radius * std::sin(angle))
// // //             );

// // //             // Точка конца — немного наружу (перпендикулярно радиусу)
// // //             cv::Point end(
// // //                 static_cast<int>(start.x + length * std::cos(angle)),
// // //                 static_cast<int>(start.y + length * std::sin(angle))
// // //             );

// // //             cv::line(frame, start, end, green, 1);
// // //         }
// // //     }

// // //     // === Звезда на макушке ===
// // //     int star_size = static_cast<int>(12 * scale);
// // //     cv::Point star_center(x, crown_bottom.y - star_size);
// // //     std::vector<cv::Point> star;
// // //     for (int i = 0; i < 10; ++i) {
// // //         double angle = i * M_PI / 5.0;
// // //         double r = (i % 2 == 0) ? star_size : star_size * 0.4;
// // //         star.push_back(cv::Point(
// // //             static_cast<int>(star_center.x + r * std::cos(angle - M_PI/2)),
// // //             static_cast<int>(star_center.y + r * std::sin(angle - M_PI/2))
// // //         ));
// // //     }
// // //     cv::fillConvexPoly(frame, star, cv::Scalar(0, 255, 255)); // жёлтая звезда
// // // }

// // // void overlayImage(cv::Mat& background, cv::Mat& foreground, cv::Point2i location)
// // // {
// // //     if (foreground.channels() != 4)
// // //     {
// // //         cv::Mat foreground_with_alpha;
// // //         cv::cvtColor(foreground, foreground_with_alpha, cv::COLOR_BGR2BGRA);
// // //         foreground = foreground_with_alpha;
// // //     }

// // //     for (int y = std::max(location.y, 0); y < background.rows; ++y)
// // //     {
// // //         int fY = y - location.y;
// // //         if (fY >= foreground.rows) break;

// // //         for (int x = std::max(location.x, 0); x < background.cols; ++x)
// // //         {
// // //             int fX = x - location.x;
// // //             if (fX >= foreground.cols) break;

// // //             double opacity = foreground.at<cv::Vec4b>(fY, fX)[3] / 255.0;
            
// // //             for (int c = 0; c < 3; ++c)
// // //             {
// // //                 background.at<cv::Vec3b>(y, x)[c] = cv::saturate_cast<uchar>(
// // //                     background.at<cv::Vec3b>(y, x)[c] * (1 - opacity) + 
// // //                     foreground.at<cv::Vec4b>(fY, fX)[c] * opacity
// // //                 );
// // //             }
// // //         }
// // //     }
// // // }

// // // // void overlayImage(cv::Mat& background, cv::Mat& foreground, cv::Point2i location)
// // // // {
// // // //     cv::Mat temp_background;
// // // //     background.copyTo(temp_background);

// // // //     // Обрезаем область наложения
// // // //     cv::Rect roi_rect(location, foreground.size());
// // // //     if (roi_rect.x < 0 || roi_rect.y < 0 ||
// // // //         roi_rect.x + roi_rect.width > background.cols ||
// // // //         roi_rect.y + roi_rect.height > background.rows) {
// // // //         return;
// // // //     }

// // // //     // Выбираем только видимую часть переднего плана
// // // //     cv::Mat fg_roi = foreground(
// // // //         cv::Range(0, roi_rect.height),
// // // //         cv::Range(0, roi_rect.width)
// // // //     );

// // // //     // Альфа-канал
// // // //     cv::Mat alpha;
// // // //     if (foreground.channels() == 4) {
// // // //         cv::extractChannel(fg_roi, alpha, 3);
// // // //     } else {
// // // //         alpha = cv::Mat::ones(fg_roi.size(), CV_8UC1) * 255;
// // // //     }

// // // //     // Нормализация alpha
// // // //     cv::Mat alpha_norm;
// // // //     alpha.convertTo(alpha_norm, CV_32F, 1.0/255.0);

// // // //     // Разделение каналов
// // // //     std::vector<cv::Mat> channels;
// // // //     cv::split(fg_roi, channels);

// // // //     // Наложение
// // // //     for (int c = 0; c < 3; ++c)
// // // //     {
// // // //         cv::Mat bg_channel = temp_background(
// // // //             cv::Range(roi_rect.y, roi_rect.y + roi_rect.height),
// // // //             cv::Range(roi_rect.x, roi_rect.x + roi_rect.width)
// // // //         );
// // // //         cv::Mat fg_channel = channels[c];
// // // //         cv::Mat blended;
// // // //         cv::multiply(fg_channel, alpha_norm, blended);
// // // //         cv::multiply(bg_channel, cv::Scalar(1.0) - alpha_norm, bg_channel);
// // // //         bg_channel += blended;
// // // //         bg_channel.copyTo(temp_background(
// // // //             cv::Range(roi_rect.y, roi_rect.y + roi_rect.height),
// // // //             cv::Range(roi_rect.x, roi_rect.x + roi_rect.width)
// // // //         ));
// // // //     }

// // // //     temp_background.copyTo(background);
// // // // }
