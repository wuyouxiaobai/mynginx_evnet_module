#pragma once
#include <cstdint>

namespace WYXB
{
class CRC32
{
private:
    CRC32(){Init_CRC32_Table();}
public:
    ~CRC32(){}

    static CRC32& getInstance()
    {
        static CRC32 instance;
        return instance;
    }

    // CRC32(CRC32 const&) = delete;
    // CRC32& operator=(CRC32 const&) = delete;

public:
    void Init_CRC32_Table();
    uint32_t Reflect(uint32_t ref, char ch);

    uint32_t Get_CRC(const char* buffer, uint32_t dwSize);

public:
    uint32_t crc32_table[256];





};







}