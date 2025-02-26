#pragma once

namespace WYXB
{

class CMemory
{

private:
	CMemory(){}  

public:
	~CMemory(){}


public:
    static CMemory& getInstance()
    {
        static CMemory instance;
        return instance;
    }
	void *AllocMemory(int memCount,bool ifmemset);
	void FreeMemory(void *point);

};
}