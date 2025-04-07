#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <curl/curl.h>

// 获取文件大小
std::size_t get_file_size(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}

// libcurl 回调：写入数据到文件
size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    std::ofstream* ofs = static_cast<std::ofstream*>(userdata);
    ofs->write(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

bool download_with_resume(const std::string& url, const std::string& output_path) {
    const std::string tmp_path = output_path + ".tmp";
    // 1. 获取已有大小
    std::size_t offset = get_file_size(tmp_path);

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::ofstream ofs(tmp_path, std::ios::binary | std::ios::app);
    if (!ofs.is_open()) {
        curl_easy_cleanup(curl);
        return false;
    }

    // 2. 设置 URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    // 3. 设置断点续传的起始位置
    if (offset > 0) {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(offset));
        std::cout << "Resuming from byte " << offset << std::endl;
    }
    // 4. 设置写数据回调
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
    // 5. 启用进度条（可选）
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    // 执行下载
    CURLcode res = curl_easy_perform(curl);
    ofs.close();
    curl_easy_cleanup(curl);

    if (res == CURLE_OK) {
        // 6. 下载成功，重命名
        std::rename(tmp_path.c_str(), output_path.c_str());
        std::cout << "Download completed: " << output_path << std::endl;
        return true;
    } else {
        std::cerr << "Download error: " << curl_easy_strerror(res) << std::endl;
        // 保留 tmp 文件，下次继续续传
        return false;
    }
}

int main() {
    std::string url = "https://example.com/largefile.zip";
    std::string output = "largefile.zip";
    if (!download_with_resume(url, output)) {
        std::cerr << "Failed to download." << std::endl;
        return 1;
    }
    return 0;
}
