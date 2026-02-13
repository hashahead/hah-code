// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "timeseries.h"

using namespace std;
using namespace hnbase;

namespace fs = boost::filesystem;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CTimeSeriesBase

CTimeSeriesBase::CTimeSeriesBase()
{
    nLastFile = 0;
}

CTimeSeriesBase::~CTimeSeriesBase()
{
}

bool CTimeSeriesBase::Initialize(const fs::path& pathLocationIn, const string& strPrefixIn)
{
    if (!fs::exists(pathLocationIn))
    {
        fs::create_directories(pathLocationIn);
    }

    if (!fs::is_directory(pathLocationIn))
    {
        return false;
    }

    pathLocation = pathLocationIn;
    strPrefix = strPrefixIn;
    nLastFile = 1;

    for (;;)
    {
        fs::path last = pathLocation / FileName(nLastFile + 1);
        if (!fs::exists(last))
        {
            break;
        }
        nLastFile++;
    }
    return CheckDiskSpace();
}

void CTimeSeriesBase::Deinitialize()
{
}

bool CTimeSeriesBase::CheckDiskSpace()
{
    // 15M
    return (fs::space(pathLocation).available > 15000000);
}

const std::string CTimeSeriesBase::FileName(uint32 nFile)
{
    ostringstream oss;
    oss << strPrefix << "_" << setfill('0') << setw(6) << nFile << ".dat";
    return oss.str();
}

bool CTimeSeriesBase::GetFilePath(uint32 nFile, string& strPath)
{
    fs::path current = pathLocation / FileName(nFile);
    if (fs::exists(current) && fs::is_regular_file(current))
    {
        strPath = current.string();
        return true;
    }
    return false;
}

bool CTimeSeriesBase::GetLastFilePath(uint32& nFile, std::string& strPath, const uint32 nWriteDataSize)
{
    for (;;)
    {
        fs::path last = pathLocation / FileName(nLastFile);
        if (!fs::exists(last))
        {
            FILE* fp = fopen(last.string().c_str(), "w+");
            if (fp == nullptr)
            {
                break;
            }
            fclose(fp);
        }
        if (fs::is_regular_file(last) && fs::file_size(last) + nWriteDataSize + 8 <= MAX_FILE_SIZE)
        {
            nFile = nLastFile;
            strPath = last.string();
            return true;
        }
        nLastFile++;
    }
    return false;
}

bool CTimeSeriesBase::RemoveFollowUpFile(uint32 nBeginFile)
{
    std::string pathFile;
    while (GetFilePath(nBeginFile, pathFile))
    {
        if (!fs::remove(fs::path(pathFile)))
        {
            hnbase::StdError("TimeSeriesBase", "RemoveFollowUpFile: remove fail fail, file: %s", pathFile.c_str());
            return false;
        }
        ++nBeginFile;
    }
    return true;
}

bool CTimeSeriesBase::TruncateFile(const string& pathFile, uint32 nOffset)
{
    string strTempFilePath = pathFile + ".temp";
    fs::rename(fs::path(pathFile), fs::path(strTempFilePath));

    FILE* pReadFd = nullptr;
    FILE* pWriteFd = nullptr;
    pReadFd = fopen(strTempFilePath.c_str(), "rb");
    if (pReadFd == nullptr)
    {
        hnbase::StdError("TimeSeriesBase", "TruncateFile: fopen fail1, file: %s", strTempFilePath.c_str());
        return false;
    }
    pWriteFd = fopen(pathFile.c_str(), "wb");
    if (pWriteFd == nullptr)
    {
        hnbase::StdError("TimeSeriesBase", "TruncateFile: fopen fail2, file: %s", pathFile.c_str());
        fclose(pReadFd);
        return false;
    }
    uint32 nReadLen = 0;
    uint8 uReadBuf[4096] = { 0 };
    while (nReadLen < nOffset && !feof(pReadFd))
    {
        size_t nNeedReadLen = nOffset - nReadLen;
        if (nNeedReadLen > sizeof(uReadBuf))
        {
            nNeedReadLen = sizeof(uReadBuf);
        }
        size_t nLen = fread(uReadBuf, 1, nNeedReadLen, pReadFd);
        if (nLen > 0)
        {
            fwrite(uReadBuf, 1, nLen, pWriteFd);
            nReadLen += nLen;
        }
        if (nLen != nNeedReadLen && ferror(pReadFd))
        {
            break;
        }
    }
    fclose(pReadFd);
    fclose(pWriteFd);

    fs::remove(fs::path(strTempFilePath));
    return true;
}

bool CTimeSeriesBase::RepairFile(uint32 nFile, uint32 nOffset)
{
    if (nOffset > 0)
    {
        std::string pathFile;
        if (!GetFilePath(nFile, pathFile))
        {
            hnbase::StdError("TimeSeriesBase", "RepairFile: GetFilePath fail");
            return false;
        }
        if (!TruncateFile(pathFile, nOffset))
        {
            hnbase::StdError("TimeSeriesBase", "RepairFile: TruncateFile fail");
            return false;
        }
        return RemoveFollowUpFile(nFile + 1);
    }
    else
    {
        return RemoveFollowUpFile(nFile);
    }
}

//////////////////////////////
// CTimeSeriesCached

const uint32 CTimeSeriesCached::nMagicNum = 0x8A5CA1E8;

CTimeSeriesCached::CTimeSeriesCached()
  : cacheStream(FILE_CACHE_SIZE)
{
}

CTimeSeriesCached::~CTimeSeriesCached()
{
}

bool CTimeSeriesCached::Initialize(const path& pathLocationIn, const string& strPrefixIn)
{

    if (!CTimeSeriesBase::Initialize(pathLocationIn, strPrefixIn))
    {
        return false;
    }

    {
        boost::unique_lock<boost::mutex> lock(mtxCache);

        ResetCache();
    }
    return true;
}

void CTimeSeriesCached::Deinitialize()
{
    boost::unique_lock<boost::mutex> lock(mtxCache);

    ResetCache();
}

void CTimeSeriesCached::ResetCache()
{
    cacheStream.Clear();
    mapCachePos.clear();
}

bool CTimeSeriesCached::VacateCache(uint32 nNeeded)
{
    const size_t nHdrSize = 12;

    while (cacheStream.GetBufFreeSpace() < nNeeded + nHdrSize)
    {
        CDiskPos diskpos;
        uint32 nSize = 0;

        try
        {
            cacheStream.Rewind();
            cacheStream >> diskpos >> nSize;
        }
        catch (exception& e)
        {
            StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }

        mapCachePos.erase(diskpos);
        cacheStream.Consume(nSize + nHdrSize);
    }
    return true;
}

//////////////////////////////
// CTimeSeriesChunk

const uint32 CTimeSeriesChunk::nMagicNum = 0x8A5CA1E8;

CTimeSeriesChunk::CTimeSeriesChunk()
{
}

CTimeSeriesChunk::~CTimeSeriesChunk()
{
}

} // namespace storage
} // namespace hashahead
