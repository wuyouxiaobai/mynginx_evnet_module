// redisClient.h
#pragma once
#include <string>
#include "redisClientImpl.h"
#include <memory>

namespace WYXB
{



class RedisClient {
public:
    RedisClient(const std::string& host, int port);
    bool setex(const std::string& key, const std::string& value, int ttl);
    std::string get(const std::string& key);
    void del(const std::string& key);

private:
    std::unique_ptr<RedisClientImpl> impl_;
};

}