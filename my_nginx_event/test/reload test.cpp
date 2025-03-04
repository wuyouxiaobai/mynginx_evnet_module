#include <iostream>
#include <fstream>
#include <csignal>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>

// 全局配置
struct Config {
    std::string value;
};

std::atomic<Config*> current_config{nullptr};
std::mutex config_mutex;

// 信号处理函数
void signal_handler(int signal) {
    if (signal == SIGHUP) {
        std::cout << "Received SIGHUP, reloading config..." << std::endl;
        Config* new_config = new Config;
        std::ifstream file("config.txt");
        if (file.is_open()) {
            std::getline(file, new_config->value);
            file.close();
        } else {
            std::cerr << "Failed to open config file." << std::endl;
            delete new_config;
            return;
        }

        // 原子地更新配置
        Config* old_config = current_config.exchange(new_config);
        if (old_config) {
            delete old_config; // 释放旧配置
        }
        std::cout << "Config reloaded: " << new_config->value << std::endl;
    }
}

// 工作线程
void worker_thread(int id) {
    while (true) {
        Config* config = current_config.load();
        if (config) {
            std::cout << "Worker " << id << " using config: " << config->value << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    // 初始化配置
    Config* initial_config = new Config;
    std::ifstream file("config.txt");
    if (file.is_open()) {
        std::getline(file, initial_config->value);
        file.close();
    } else {
        std::cerr << "Failed to open config file." << std::endl;
        delete initial_config;
        return 1;
    }
    current_config.store(initial_config);

    // 注册信号处理函数
    std::signal(SIGHUP, signal_handler);

    // 启动工作线程
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back(worker_thread, i);
    }

    // 主循环
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 清理
    for (auto& worker : workers) {
        worker.join();
    }
    delete current_config.load();

    return 0;
}