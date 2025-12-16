#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <set>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

// Pakostim
// std::mutex mtx;
std::mutex mutex1, mutex2, mutex3;
namespace fs = std::filesystem;


void process(const std::vector<fs::path> files, const std::string& output_dir, std::atomic<int>& count_of_success, std::atomic<int>& count_of_failed, int thread_id) {
    for(const auto& file_path : files) {

        // Prodolshaem pakostit`
        if(thread_id % 2 == 0) {
            mutex1.lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            mutex2.lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            mutex3.lock();  // Поток 0: 1 → 2 → 3
        } else {
            mutex3.lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            mutex2.lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            mutex1.lock();  // Поток 1: 3 → 2 → 1 (ДЕДЛОК!)
        }
       
        int width, height, channels;
        unsigned char* img = stbi_load(file_path.string().c_str(), &width, &height, &channels, 0);
        
        if(!img) {
            ++count_of_failed;
            continue;
        }
        
      
        int total_pixels = width * height * channels;
        for(int i = 0; i < total_pixels; i++) {
            img[i] = 255 - img[i];
        }

        std::string filename = file_path.stem().string();
        std::string output_path = output_dir + "/inverted_" + filename + ".png";

        //std::unique_lock<std::mutex> lock(mtx);

        std::cout << "Proccessed: " << output_path << std::endl;

        //lock.unlock();
        if(!stbi_write_png(output_path.c_str(), width, height, channels, img, width * channels)) {
            ++count_of_failed;
        } else {
            ++count_of_success;
        }
        
        stbi_image_free(img);

        // New
        if(thread_id % 2 == 0) {
            mutex3.unlock();
            mutex2.unlock();
            mutex1.unlock();
        } else {
            mutex1.unlock();
            mutex2.unlock();
            mutex3.unlock();
        }
    }
}

int main(int argc, char** argv) {
    int NUM_THREADS = 4;
    if(argc > 1) {
        NUM_THREADS = std::atoi(argv[1]);
    }

    std::string input_dir = "./dataset";
    std::string output_dir = "./results/images";

    if(!fs::exists(input_dir)) {
        std::cerr << "Error: Directory '" << input_dir << "' doesn't exist!\n";
        return 1;
    }

    if(!fs::exists(output_dir)) {
        fs::create_directory(output_dir);
    }

    std::set<std::string> extensions = {".png", ".jpeg", ".jpg"};

    //put all files in one vector
    std::vector<fs::path> image_files;

    for(const auto& file : fs::directory_iterator(input_dir)) {
        if(file.is_regular_file()) {
            std::string ext = file.path().extension().string();
            
             //check the correctness of extension
            bool supported = find(extensions.begin(), extensions.end(), ext) != extensions.end();
            if(!supported) continue;

            image_files.push_back(file.path());
        }
    }

    if(image_files.empty()) {
        std::cerr << "Error: No image files found in dataset!" << std::endl;
        return 1;
    }
    
    std::vector<std::vector<fs::path>> pieces(NUM_THREADS);
    for(size_t i = 0; i < image_files.size(); i++) {
        pieces[i % NUM_THREADS].push_back(image_files[i]);
    }

    std::atomic<int> success_count(0);
    std::atomic<int> fail_count(0);
    std::vector<std::thread> threads;


    for(int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(process, pieces[i], output_dir, std::ref(success_count), std::ref(fail_count), i);             
    }

    for(auto& thread : threads) {
        thread.join();
    }

    return 0;
}   