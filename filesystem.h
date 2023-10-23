#pragma once

#include <stdio.h>

#define BASEFILESYSTEM_INTERFACE_VERSION "VBaseFileSystem011"

typedef void* FileHandle_t;
enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD	= SEEK_SET,
	FILESYSTEM_SEEK_CURRENT = SEEK_CUR,
	FILESYSTEM_SEEK_TAIL	= SEEK_END,
};

class IBaseFileSystem
{
public:
	virtual int				Read(void* pOutput, int size, FileHandle_t file) = 0;
	virtual int				Write(void const* pInput, int size, FileHandle_t file) = 0;
	virtual FileHandle_t	Open(const char* pFileName, const char* pOptions, const char* pathID = 0) = 0;
	virtual void			Close(FileHandle_t file) = 0;
	virtual void			Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) = 0;
	virtual unsigned int	Tell(FileHandle_t file) = 0;
	virtual unsigned int	Size(FileHandle_t file) = 0;
	virtual unsigned int	Size(const char* pFileName, const char* pPathID = 0) = 0;
	virtual void			Flush(FileHandle_t file) = 0;
	virtual bool			Precache(const char* pFileName, const char* pPathID = 0) = 0;
	virtual bool			FileExists(const char* pFileName, const char* pPathID = 0) = 0;
};