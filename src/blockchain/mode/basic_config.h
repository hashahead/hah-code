// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_BASIC_CONFIG_H
#define HASHAHEAD_MODE_BASIC_CONFIG_H

#include <exception>
#include <string>

#include "hnbase.h"
#include "mode/auto_options.h"

namespace hashahead
{
/**
 * Dynamic cast hnbase::CConfig* to it's derived class.
 * T is a pointer type.
 * If occured error, throw runtime_error.
 */
template <typename T>
typename std::enable_if<std::is_pointer<T>::value, T>::type
CastConfigPtr(hnbase::CConfig* ptr)
{
    if (!ptr)
        return nullptr;

    T p = dynamic_cast<T>(ptr);
    if (!p)
    {
        throw std::runtime_error(
            std::string("bad_cast: hnbase::CConfig* to ") + hnbase::TypeName(typeid(T)));
    }
    return p;
}

/**
 * basic config on business.
 */
class CBasicConfig : virtual public hnbase::CConfig, virtual public CBasicConfigOption
{
public:
    CBasicConfig();
    virtual ~CBasicConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

    // public:
    //     unsigned int nMagicNum;
};

} // namespace hashahead

#endif // HASHAHEAD_MODE_BASIC_CONFIG_H
