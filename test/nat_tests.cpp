// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include "base_tests.h"
#include "natpmp.h"
#include "natportmapping.h"
#include "test_big.h"

using namespace std;
using namespace hashahead;

//./build-release/test/test_big --log_level=all --run_test=nat_tests/pmptest
//./build-release/test/test_big --log_level=all --run_test=nat_tests/pmptest2

BOOST_FIXTURE_TEST_SUITE(nat_tests, BasicUtfSetup)

/* List of IP address blocks which are private / reserved and therefore not suitable for public external IP addresses */
#define IP(a, b, c, d) (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))
#define MSK(m) (32 - (m))
static const struct
{
    uint32_t address;
    uint32_t rmask;
} reserved[] = {
    { IP(0, 0, 0, 0), MSK(8) },       /* RFC1122 "This host on this network" */
    { IP(10, 0, 0, 0), MSK(8) },      /* RFC1918 Private-Use */
    { IP(100, 64, 0, 0), MSK(10) },   /* RFC6598 Shared Address Space */
    { IP(127, 0, 0, 0), MSK(8) },     /* RFC1122 Loopback */
    { IP(169, 254, 0, 0), MSK(16) },  /* RFC3927 Link-Local */
    { IP(172, 16, 0, 0), MSK(12) },   /* RFC1918 Private-Use */
    { IP(192, 0, 0, 0), MSK(24) },    /* RFC6890 IETF Protocol Assignments */
    { IP(192, 0, 2, 0), MSK(24) },    /* RFC5737 Documentation (TEST-NET-1) */
    { IP(192, 31, 196, 0), MSK(24) }, /* RFC7535 AS112-v4 */
    { IP(192, 52, 193, 0), MSK(24) }, /* RFC7450 AMT */
    { IP(192, 88, 99, 0), MSK(24) },  /* RFC7526 6to4 Relay Anycast */
    { IP(192, 168, 0, 0), MSK(16) },  /* RFC1918 Private-Use */
    { IP(192, 175, 48, 0), MSK(24) }, /* RFC7534 Direct Delegation AS112 Service */
    { IP(198, 18, 0, 0), MSK(15) },   /* RFC2544 Benchmarking */
    { IP(198, 51, 100, 0), MSK(24) }, /* RFC5737 Documentation (TEST-NET-2) */
    { IP(203, 0, 113, 0), MSK(24) },  /* RFC5737 Documentation (TEST-NET-3) */
    { IP(224, 0, 0, 0), MSK(4) },     /* RFC1112 Multicast */
    { IP(240, 0, 0, 0), MSK(4) },     /* RFC1112 Reserved for Future Use + RFC919 Limited Broadcast */
};
#undef IP
#undef MSK

static int addr_is_reserved(struct in_addr* addr)
{
    uint32_t address = ntohl(addr->s_addr);
    size_t i;

    for (i = 0; i < sizeof(reserved) / sizeof(reserved[0]); ++i)
    {
        if ((address >> reserved[i].rmask) == (reserved[i].address >> reserved[i].rmask))
            return 1;
    }

    return 0;
}

static void handlenatpmpbadreplytype(natpmp_t* pnatpmp, const natpmpresp_t* presponse, int* preturncode)
{
    char retry = (pnatpmp->try_number <= 9);
    printf("readnatpmpresponseorretry received unexpected reply type %hu , %s...\n", presponse->type, (retry == 1 ? "retrying" : "no more retry"));
    if (retry)
    {
        *preturncode = NATPMP_TRYAGAIN;
        pnatpmp->has_pending_request = 1;
    }
}

BOOST_AUTO_TEST_CASE(pmptest)
{
    natpmp_t natpmp;
    natpmpresp_t response;
    int r;
    int sav_errno;
    struct timeval timeout;
    fd_set fds;
    int i;
    int protocol = 0;
    uint16_t privateport = 0;
    uint16_t publicport = 0;
    uint32_t lifetime = 3600;
    int command = 0;
    int forcegw = 0;
    in_addr_t gateway = 0;
    struct in_addr gateway_in_use;

    protocol = NATPMP_PROTOCOL_TCP;
    privateport = 8500;
    publicport = 8500;

    r = initnatpmp(&natpmp, forcegw, gateway);
    printf("initnatpmp() returned %d (%s)\n", r, r ? "FAILED" : "SUCCESS");
    if (r < 0)
    {
        return;
    }

    gateway_in_use.s_addr = natpmp.gateway;
    printf("using gateway : %s\n", inet_ntoa(gateway_in_use));

    r = sendpublicaddressrequest(&natpmp);
    printf("sendpublicaddressrequest returned %d (%s)\n", r, r == 2 ? "SUCCESS" : "FAILED");
    if (r < 0)
    {
        return;
    }

    do
    {
        FD_ZERO(&fds);
        FD_SET(natpmp.s, &fds);
        getnatpmprequesttimeout(&natpmp, &timeout);
        r = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
        if (r < 0)
        {
            fprintf(stderr, "select()");
            return;
        }
        r = readnatpmpresponseorretry(&natpmp, &response);
        sav_errno = errno;
        printf("readnatpmpresponseorretry returned %d (%s)\n",
               r, r == 0 ? "OK" : (r == NATPMP_TRYAGAIN ? "TRY AGAIN" : "FAILED"));
        if (r < 0 && r != NATPMP_TRYAGAIN)
        {
#ifdef ENABLE_STRNATPMPERR
            fprintf(stderr, "readnatpmpresponseorretry() failed : %s\n",
                    strnatpmperr(r));
#endif
            fprintf(stderr, "  errno=%d '%s'\n", sav_errno, strerror(sav_errno));
        }
        if (r >= 0 && response.type != 0)
        {
            handlenatpmpbadreplytype(&natpmp, &response, &r);
        }
    } while (r == NATPMP_TRYAGAIN);
    if (r < 0)
    {
        return;
    }

    if (response.type != NATPMP_RESPTYPE_PUBLICADDRESS)
    {
        fprintf(stderr, "readnatpmpresponseorretry() failed : invalid response type %u\n", response.type);
        return;
    }

    if (addr_is_reserved(&response.pnu.publicaddress.addr))
    {
        fprintf(stderr, "readnatpmpresponseorretry() failed : invalid Public IP address %s\n", inet_ntoa(response.pnu.publicaddress.addr));
        return;
    }

    printf("Public IP address : %s\n", inet_ntoa(response.pnu.publicaddress.addr));
    printf("epoch = %u\n", response.epoch);

    if (command == 'a')
    {
        r = sendnewportmappingrequest(&natpmp, protocol,
                                      privateport, publicport,
                                      lifetime);
        printf("sendnewportmappingrequest returned %d (%s)\n",
               r, r == 12 ? "SUCCESS" : "FAILED");
        if (r < 0)
        {
            return;
        }

        do
        {
            FD_ZERO(&fds);
            FD_SET(natpmp.s, &fds);
            getnatpmprequesttimeout(&natpmp, &timeout);
            select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
            r = readnatpmpresponseorretry(&natpmp, &response);
            printf("readnatpmpresponseorretry returned %d (%s)\n",
                   r, r == 0 ? "OK" : (r == NATPMP_TRYAGAIN ? "TRY AGAIN" : "FAILED"));
            if (r >= 0 && ((protocol == NATPMP_PROTOCOL_TCP && response.type != NATPMP_RESPTYPE_TCPPORTMAPPING) || (protocol == NATPMP_PROTOCOL_UDP && response.type != NATPMP_RESPTYPE_UDPPORTMAPPING)))
            {
                handlenatpmpbadreplytype(&natpmp, &response, &r);
            }
        } while (r == NATPMP_TRYAGAIN);
        if (r < 0)
        {
#ifdef ENABLE_STRNATPMPERR
            fprintf(stderr, "readnatpmpresponseorretry() failed : %s\n",
                    strnatpmperr(r));
#endif
            return;
        }

        printf("Mapped public port %hu protocol %s to local port %hu "
               "lifetime %u\n",
               response.pnu.newportmapping.mappedpublicport,
               response.type == NATPMP_RESPTYPE_UDPPORTMAPPING ? "UDP" : (response.type == NATPMP_RESPTYPE_TCPPORTMAPPING ? "TCP" : "UNKNOWN"),
               response.pnu.newportmapping.privateport,
               response.pnu.newportmapping.lifetime);
        printf("epoch = %u\n", response.epoch);
    }

    r = closenatpmp(&natpmp);
    printf("closenatpmp() returned %d (%s)\n", r, r == 0 ? "SUCCESS" : "FAILED");
}

BOOST_AUTO_TEST_CASE(pmptest2)
{
    std::string strExtIp;
    if (!CNatPortMapping::GetPmpPublicAddress(strExtIp))
    {
        printf("Get pmp public address failed\n");
    }
    else
    {
        printf("Get pmp public address success, public address: %s\n", strExtIp.c_str());
    }
    if (!CNatPortMapping::PmpPortMapping("192.168.3.26", 8500, 8500))
    {
        printf("Mapping pmp port failed\n");
    }
    else
    {
        printf("Mapping pmp port success\n");
    }
}

BOOST_AUTO_TEST_SUITE_END()
