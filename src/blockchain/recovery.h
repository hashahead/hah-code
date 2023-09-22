// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_RECOVERY_H
#define HASHAHEAD_RECOVERY_H

#include "base.h"

namespace hashahead
{

class CRecovery : public IRecovery
{
public:
    CRecovery();
    ~CRecovery();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;

protected:
    IDispatcher* pDispatcher;
};

} // namespace hashahead
#endif // HASHAHEAD_RECOVERY_H
