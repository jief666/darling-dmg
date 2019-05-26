#ifndef FILEREADER_H
#define FILEREADER_H
#include "Reader.h"
#include <string>

#ifdef _MSC_VER
#include "Windows.h"
#endif

class FileReader : public Reader
{
public:
	FileReader(const std::string& path);
	~FileReader();
	
	int32_t read(void* buf, int32_t count, uint64_t offset) override; // Cannot return partial read
	uint64_t length() override { return len; }

private:

	int32_t read_(void* buf, int32_t count, uint64_t offset); // can return partial read


	int m_fd;
	uint64_t len;
#ifdef _MSC_VER
	HANDLE m_fdw;
	bool m_isPhys;
#endif

#ifdef DEBUG
    std::string m_path;
#endif
};

#endif
