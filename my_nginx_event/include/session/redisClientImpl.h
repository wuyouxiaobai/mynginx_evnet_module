#include <hiredis/hiredis.h>
#include <stdexcept>

namespace WYXB
{

class RedisClientImpl {
    public:
    redisContext* context;

    RedisClientImpl(const std::string& host, int port) {
        context = redisConnect(host.c_str(), port);
        if (!context || context->err) {
            throw std::runtime_error("Failed to connect to Redis");
        }
    }

    ~RedisClientImpl() {
        if (context) {
            redisFree(context);
        }
    }
};

}