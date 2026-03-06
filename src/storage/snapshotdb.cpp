// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "snapshotdb.h"

#include "dbstruct.h"
#include "timeseries.h"

using namespace std;
using namespace hnbase;

namespace fs = boost::filesystem;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CSnapshotDB

CSnapshotDB::CSnapshotDB()
{
}

CSnapshotDB::~CSnapshotDB()
{
}

bool CSnapshotDB::Initialize(const fs::path& pathData)
{
    pathDataLocation = pathData;
    pathSnapshot = pathDataLocation / "snapshot";
    if (!fs::exists(pathSnapshot))
    {
        fs::create_directories(pathSnapshot);
    }
    if (!fs::is_directory(pathSnapshot))
    {
        return false;
    }
    return true;
}

void CSnapshotDB::Deinitialize()
{
}

bool CSnapshotDB::Remove()
{
    return true;
}

bool CSnapshotDB::IsSnapshotBlock(const uint256& hashBlock)
{
    set<uint256, CustomBlockHashCompare> setBlockHash;
    GetAllSnapshotBlock(setBlockHash);
    return (setBlockHash.find(hashBlock) != setBlockHash.end());
}

bool CSnapshotDB::StartSnapshot(const uint256& hashLastBlock, const uint32 nMaxSnapshots)
{
    fs::path pathLastBlock = pathSnapshot / hashLastBlock.ToString();
    if (fs::exists(pathLastBlock))
    {
        fs::remove_all(pathLastBlock);
    }

    RemoveHeightSnapshot(CBlock::GetBlockHeightByHash(hashLastBlock));
    RemoveRedundantSnapshot(nMaxSnapshots);

    fs::create_directories(pathLastBlock);
    return true;
}

bool CSnapshotDB::SaveBlockFile(const uint256& hashLastBlock, const uint32 nFile, const uint32 nOffset)
{
    CTimeSeriesCached tsBlock;
    if (!tsBlock.Initialize(pathDataLocation / "block", BLOCKFILE_PREFIX))
    {
        StdLog("CSnapshotDB", "Save block file: Initialize failed, last block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }
    const uint32 nBlockSize = tsBlock.GetBlockSize(nFile, nOffset);
    if (nBlockSize == 0)
    {
        StdLog("CSnapshotDB", "Save block file: Get last block size failed, last block: %s", hashLastBlock.GetBhString().c_str());
        tsBlock.Deinitialize();
        return false;
    }
    const uint32 nCopyFileSize = nOffset + nBlockSize;

    fs::path pathLastBlock = pathSnapshot / hashLastBlock.ToString();
    if (!fs::exists(pathLastBlock))
    {
        StdLog("CSnapshotDB", "Save block file: Snapshot directory not exist, dir: %s, last block: %s", pathLastBlock.string().c_str(), hashLastBlock.GetBhString().c_str());
        tsBlock.Deinitialize();
        return false;
    }

    if (!tsBlock.CopyToPath(nFile, nCopyFileSize, pathLastBlock))
    {
        StdLog("CSnapshotDB", "Save block file: Copy file failed, last block: %s", hashLastBlock.GetBhString().c_str());
        tsBlock.Deinitialize();
        return false;
    }

    tsBlock.Deinitialize();
    return true;
}

bool CSnapshotDB::SaveSnapshotData(const uint256& hashLastBlock, const uint8 nDataType, const char* pSnapData, const uint32 nSnapDataSize)
{
    CTimeSeriesSnapshot tss;
    if (!tss.Initialize(pathSnapshot / hashLastBlock.ToString(), SNAPSHOTFILE_PREFIX))
    {
        StdLog("CSnapshotDB", "Save snapshot data: Initialize failed, last block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }
    CDiskPos pos;
    if (!tss.Write(nDataType, pSnapData, nSnapDataSize, pos))
    {
        StdLog("CSnapshotDB", "Save snapshot data: Write data failed, data size: %u, last block: %s", nSnapDataSize, hashLastBlock.GetBhString().c_str());
        tss.Deinitialize();
        return false;
    }
    tss.Deinitialize();
    return true;
}

//------------------------------------------------------------------
bool CSnapshotDB::GetSnapshotFileList(const uint256& hashSnapBlock, std::vector<CSnapshotFileInfo>& vSnapFilelist)
{
    try
    {
        fs::path pathSnapBlock = pathSnapshot / hashSnapBlock.ToString();
        if (fs::exists(pathSnapBlock) && fs::is_directory(pathSnapBlock))
        {
            for (auto& entry : fs::directory_iterator(pathSnapBlock))
            {
                fs::path file_path = entry.path();
                if (fs::is_regular_file(file_path))
                {
                    const uint64 nFileSize = fs::file_size(file_path);
                    const string strFileName = file_path.filename().string();
                    CSnapshotFileInfo snapFileInfo;
                    snapFileInfo.strFileName = strFileName;
                    snapFileInfo.nFileSize = nFileSize;
                    vSnapFilelist.push_back(snapFileInfo);
                }
            }
        }
        else
        {
            StdLog("CSnapshotDB", "Get snapshot file list: Directory does not exist, snapshot block: %s", hashSnapBlock.ToString().c_str());
            return false;
        }
    }
    catch (const fs::filesystem_error& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace hashahead
