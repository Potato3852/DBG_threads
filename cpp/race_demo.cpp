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


//std::mutex mtx;
namespace fs = std::filesystem;

int success_count = 0;
int fail_count = 0;

void process(const std::vector<fs::path> files, const std::string& output_dir) {
    for(const auto& file_path : files) {
       
        int width, height, channels;
        unsigned char* img = stbi_load(file_path.string().c_str(), &width, &height, &channels, 0);
        
        if(!img) {
            ++fail_count;
            continue;
        }
        
      
        int total_pixels = width * height * channels;
        for(int i = 0; i < total_pixels; i++) {
            img[i] = 255 - img[i];
        }

        std::string filename = file_path.stem().string();
        std::string output_path = output_dir + "/inverted_" + filename + ".png";

        //std::unique_lock<std::mutex> lock(mtx);

        //std::cout << "Proccessed: " << output_path << std::endl;

        //lock.unlock();
        if(!stbi_write_png(output_path.c_str(), width, height, channels, img, width * channels)) {
            ++fail_count;
        } else {
            ++success_count;
        }
        
        stbi_image_free(img);
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
    
    // std::vector<std::vector<fs::path>> pieces(NUM_THREADS);
    // for(size_t i = 0; i < image_files.size(); i++) {
    //     pieces[i % NUM_THREADS].push_back(image_files[i]);
    // }
    // Than brake all other logic. We are crazy
    std::vector<std::vector<fs::path>> pieces(NUM_THREADS);

    int skipped_files = 0;
    int duplicated_files = 0;

    for(size_t i = 0; i < image_files.size(); i++) {
        if (rand() % 10 == 0) {
            skipped_files++;
            continue;
        }

        int main_thread = rand() % NUM_THREADS;
        pieces[main_thread].push_back(image_files[i]);

        if(rand() % 5 == 0) {
            int dup_thread;
            do {
                dup_thread = rand() % NUM_THREADS;
            } while (dup_thread == main_thread);
            
            pieces[dup_thread].push_back(image_files[i]);
            duplicated_files++;
        }
    }

    // std::atomic<int> success_count(0);
    // std::atomic<int> fail_count(0);
    // than in global code segment
    // int success_count = 0;
    // int fail_count = 0;
    
    std::vector<std::thread> threads;
    for(int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(process, pieces[i], output_dir);             
    }

    for(auto& thread : threads) {
        thread.join();
    }

    std::cout << "Total files: " << image_files.size() << std::endl;
    std::cout << "Skipped files: " << skipped_files << std::endl;
    std::cout << "Duplicated files: " << duplicated_files << std::endl;
    int expected_total = image_files.size() + duplicated_files - skipped_files;
    std::cout << "Expected total (with duplicates): " << expected_total << std::endl;

    return 0;
}   