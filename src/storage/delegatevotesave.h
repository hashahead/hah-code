// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_DELEGATEVOTESAVE_H
#define STORAGE_DELEGATEVOTESAVE_H

#include <boost/filesystem.hpp>

#include "delegate.h"
#include "hnbase.h"

namespace hashahead
{
namespace storage
{

class CDelegateVoteSave
{
public:
    CDelegateVoteSave();
    ~CDelegateVoteSave();
    bool Initialize(const boost::filesystem::path& pathData);
    bool Remove();
    bool Save(const delegate::CDelegate& delegate);
    bool Load(delegate::CDelegate& delegate);

protected:
    boost::filesystem::path pathDelegateVoteFile;
};

} // namespace storage
} // namespace hashahead

#endif //STORAGE_DELEGATEVOTESAVE_H
