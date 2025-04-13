// RedisSessionStorage.cpp

#include "RedisSessionStorage.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace WYXB {


void RedisSessionStorage::save(std::shared_ptr<Session> session) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("sessionId", rapidjson::Value(session->getId().c_str(), alloc), alloc);
    doc.AddMember("maxAge", session->getMaxAge(), alloc);

    rapidjson::Value data(rapidjson::kObjectType);
    for (const auto& pair : session->getAllData()) {
        data.AddMember(
            rapidjson::Value(pair.first.c_str(), alloc),
            rapidjson::Value(pair.second.c_str(), alloc),
            alloc
        );
    }
    doc.AddMember("data", data, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string key = "session:" + session->getId();
    redis_->setex(key, buffer.GetString(), session->getMaxAge());
}

std::shared_ptr<Session> RedisSessionStorage::load(const std::string& sessionId) {
    std::string key = "session:" + sessionId;
    std::string jsonStr = redis_->get(key);
    if (jsonStr.empty()) return nullptr;

    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    if (!doc.IsObject()) return nullptr;

    auto session = std::make_shared<Session>(sessionId);

    if (doc.HasMember("data")) {
        for (auto it = doc["data"].MemberBegin(); it != doc["data"].MemberEnd(); ++it) {
            session->setValue(it->name.GetString(), it->value.GetString());
        }
    }

    session->refresh(); // 设置新的过期时间
    return session;
}

void RedisSessionStorage::remove(const std::string& sessionId) {
    redis_->del("session:" + sessionId);
}

}
