// redisClient.cpp

#include "redisClient.h"
#include "redisClientImpl.h"

namespace WYXB
{




RedisClient::RedisClient(const std::string& host, int port)
    : impl_(std::make_unique<RedisClientImpl>(host, port)) {}

bool RedisClient::setex(const std::string& key, const std::string& value, int ttl) {
    redisReply* reply = (redisReply*)redisCommand(
        impl_->context, "SETEX %s %d %s", key.c_str(), ttl, value.c_str());

    if (!reply) return false;
    bool success = (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    freeReplyObject(reply);
    return success;
}

std::string RedisClient::get(const std::string& key) {
    redisReply* reply = (redisReply*)redisCommand(
        impl_->context, "GET %s", key.c_str());

    std::string result;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        result = reply->str;
    }

    if (reply) freeReplyObject(reply);
    return result;
}

void RedisClient::del(const std::string& key) {
    redisReply* reply = (redisReply*)redisCommand(
        impl_->context, "DEL %s", key.c_str());
    if (reply) freeReplyObject(reply);
}


}