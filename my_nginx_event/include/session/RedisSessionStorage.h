#pragma once

#include "RedisSessionStorage.h"
#include "sessionStorage.h"
#include "session.h"
#include "redisClient.h"  // 你需要封装的 redis 操作类
namespace WYXB
{


class RedisSessionStorage : public SessionStorage {
public:
    explicit RedisSessionStorage(std::shared_ptr<RedisClient> client, std::weak_ptr<SessionManager> sessionManager)
    : redis_(std::move(client)), sessionManager_(sessionManager) {}

    void save(std::shared_ptr<Session> session) override;
    std::shared_ptr<Session> load(const std::string& sessionId) override;
    void remove(const std::string& sessionId) override;

private:
    std::shared_ptr<RedisClient> redis_;
    std::weak_ptr<SessionManager> sessionManager_;
};

}

