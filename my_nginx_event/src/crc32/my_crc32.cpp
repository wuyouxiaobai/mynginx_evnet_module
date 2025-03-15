// #include "my_crc32.h"


// namespace WYXB
// {
// void CRC32::Init_CRC32_Table()
// {
//     uint32_t ulPloynomial = 0x04c11db7;
//     for(int i = 0; i < 256; i++)
//     {
//         uint32_t crc = Reflect(i, 8) << 24;
//         for(int j = 0; j < 8; j++)
//         {
//             crc = (crc << 1) ^ (crc & (1 << 31) ? ulPloynomial : 0);
//         }
//         crc32_table[i] = Reflect(crc, 32);
//     }
// }
// uint32_t CRC32::Reflect(uint32_t ref, char ch)
// {
//     uint32_t value = 0;
//     for(int i = 1; i <= ch; i++)
//     {
//         if(ref & 1)
//         {
//             value |= (1 << (ch - i));
//         }
//         ref >>= 1;
//     }
//     return value;

// }

// uint32_t CRC32::Get_CRC(const char* buffer, uint32_t dwSize)
// {
//     uint32_t crc = 0xffffffff;
//     for(uint32_t i = 0; i < dwSize; i++)
//     {
//         crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ buffer[i]];
//     }
//     return crc ^ 0xffffffff;

// }





// }
