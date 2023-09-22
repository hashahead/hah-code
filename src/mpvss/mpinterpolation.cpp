// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mpinterpolation.h"

#include "curve25519/curve25519.h"

using namespace std;

// [-1/50, 1/50] mod 2^252+27742317777372353535851937790883648493
static const CSC25519 SCInverse[101] = {
    CSC25519({ 0x1a17ca2895da543aULL, 0xe5096a6aa2744012ULL, 0xae147ae147ae147aULL, 0x2e147ae147ae147ULL }),
    CSC25519({ 0xd6c1048e38e29734ULL, 0x703b6f3c1d4691bULL, 0xfac687d6343eb1a2ULL, 0xd6343eb1a1f58d0ULL }),
    CSC25519({ 0x8e8e2c645252d35cULL, 0x656be09b9d9ede3aULL, 0x5555555555555555ULL, 0xc55555555555555ULL }),
    CSC25519({ 0xed7c29d21ba8e31cULL, 0x624022dde9bf9fadULL, 0xe4c415c9882b9310ULL, 0x882b9310572620aULL }),
    CSC25519({ 0x683b4fb698af7860ULL, 0x8017863313c72aacULL, 0xbd37a6f4de9bd37aULL, 0xcde9bd37a6f4de9ULL }),
    CSC25519({ 0xa34117ad84d07dfdULL, 0x26865663f298f3a7ULL, 0xc16c16c16c16c16cULL, 0xc16c16c16c16c16ULL }),
    CSC25519({ 0x5374c8120dbfc4d1ULL, 0x5d90b4396c6e5ad6ULL, 0x45d1745d1745d174ULL, 0x5d1745d1745d17ULL }),
    CSC25519({ 0x61bd53ee3c7fb44eULL, 0xcf9dcac28a50b45ULL, 0x82fa0be82fa0be83ULL, 0xe82fa0be82fa0beULL }),
    CSC25519({ 0x952a433f04647874ULL, 0xfa450432a0281211ULL, 0xcf3cf3cf3cf3cf3cULL, 0x4f3cf3cf3cf3cf3ULL }),
    CSC25519({ 0x9a59a82eec1dfaa0ULL, 0xf60eb453588e963fULL, 0x1f3831f3831f3831ULL, 0x1f3831f3831f383ULL }),
    CSC25519({ 0xcca6ee3fe9cbd33fULL, 0xa8bb41f49c8d1e81ULL, 0x9999999999999999ULL, 0xb99999999999999ULL }),
    CSC25519({ 0x58034cdd58e3eaf6ULL, 0xe2ecf1c57fb64a19ULL, 0x2df2df2df2df2df2ULL, 0x2df2df2df2df2dfULL }),
    CSC25519({ 0x9ce4a4bb271a74e5ULL, 0x1c4d969773cae1f6ULL, 0x79435e50d79435e5ULL, 0xb5e50d79435e50dULL }),
    CSC25519({ 0x46be53ec5e4655b0ULL, 0x2785790fa89a5fefULL, 0xacf914c1bacf914cULL, 0x914c1bacf914c1bULL }),
    CSC25519({ 0xf60cc4d24ec72881ULL, 0x2af02d854681495bULL, 0x71c71c71c71c71c7ULL, 0xb1c71c71c71c71cULL }),
    CSC25519({ 0x1b0a8c283d0c0fb3ULL, 0x9f3f015beec4a6fcULL, 0xf8af8af8af8af8afULL, 0xf8af8af8af8af8aULL }),
    CSC25519({ 0x72263ac7ddb7bf61ULL, 0x1844410f6603d440ULL, 0xf0f0f0f0f0f0f0fULL, 0x70f0f0f0f0f0f0fULL }),
    CSC25519({ 0x6f4660181255066cULL, 0x7cc0f04c909323c8ULL, 0x7c1f07c1f07c1f0ULL, 0x7c1f07c1f07c1fULL }),
    CSC25519({ 0x7dc2df7c1e86691dULL, 0x342d70ac976b081ULL, 0x0ULL, 0x280000000000000ULL }),
    CSC25519({ 0x72e45a0b4d3a8476ULL, 0xee73e2ad70a7324cULL, 0xbdef7bdef7bdef7bULL, 0xbdef7bdef7bdef7ULL }),
    CSC25519({ 0x48d871f718bdd305ULL, 0x2f5a04a69a699f10ULL, 0x2222222222222222ULL, 0xa22222222222222ULL }),
    CSC25519({ 0xacf858ead3f4130ULL, 0x20c4c698645b07a4ULL, 0x9611a7b9611a7b96ULL, 0xb9611a7b9611a7bULL }),
    CSC25519({ 0x5fbf64de8696b4aeULL, 0x7767864bf03c1b1aULL, 0xb6db6db6db6db6dbULL, 0x76db6db6db6db6dULL }),
    CSC25519({ 0x62af6f06d5bafe0eULL, 0xd600eb729c074e96ULL, 0x97b425ed097b425eULL, 0x425ed097b425ed0ULL }),
    CSC25519({ 0x404f34c0555e071ULL, 0x54636aa83f916f26ULL, 0xc4ec4ec4ec4ec4ecULL, 0x44ec4ec4ec4ec4eULL }),
    CSC25519({ 0x342f94512bb4a874ULL, 0xca12d4d544e88024ULL, 0x5c28f5c28f5c28f5ULL, 0x5c28f5c28f5c28fULL }),
    CSC25519({ 0xc509f5ae47afd2cbULL, 0xb5f8c75898461f9eULL, 0xaaaaaaaaaaaaaaaaULL, 0x8aaaaaaaaaaaaaaULL }),
    CSC25519({ 0x78643c52d4691cd3ULL, 0xeb5012878496b882ULL, 0x7a6f4de9bd37a6f4ULL, 0x9bd37a6f4de9bd3ULL }),
    CSC25519({ 0xa6e990241b7f89a2ULL, 0xbb216872d8dcb5acULL, 0x8ba2e8ba2e8ba2e8ULL, 0xba2e8ba2e8ba2eULL }),
    CSC25519({ 0x2a54867e08c8f0e8ULL, 0xf48a086540502423ULL, 0x9e79e79e79e79e79ULL, 0x9e79e79e79e79e7ULL }),
    CSC25519({ 0x413b796576a1d291ULL, 0x3c978a0a9622a02dULL, 0x3333333333333333ULL, 0x733333333333333ULL }),
    CSC25519({ 0xe1b6e65bf13f15ddULL, 0x23bc3350449e2716ULL, 0xf286bca1af286bcaULL, 0x6bca1af286bca1aULL }),
    CSC25519({ 0x9407268a40987d15ULL, 0x4101612bea0af5e1ULL, 0xe38e38e38e38e38eULL, 0x638e38e38e38e38ULL }),
    CSC25519({ 0xe44c758fbb6f7ec2ULL, 0x3088821ecc07a880ULL, 0x1e1e1e1e1e1e1e1eULL, 0xe1e1e1e1e1e1e1eULL }),
    CSC25519({ 0xfb85bef83d0cd23aULL, 0x685ae1592ed6102ULL, 0x0ULL, 0x500000000000000ULL }),
    CSC25519({ 0x399e80d3d485d21dULL, 0x49d50f6e91dba14aULL, 0x4444444444444444ULL, 0x444444444444444ULL }),
    CSC25519({ 0xbf7ec9bd0d2d695cULL, 0xeecf0c97e0783634ULL, 0x6db6db6db6db6db6ULL, 0xedb6db6db6db6dbULL }),
    CSC25519({ 0x809e6980aabc0e2ULL, 0xa8c6d5507f22de4cULL, 0x89d89d89d89d89d8ULL, 0x89d89d89d89d89dULL }),
    CSC25519({ 0x320188423269d1a9ULL, 0x571294d28d94a267ULL, 0x5555555555555555ULL, 0x155555555555555ULL }),
    CSC25519({ 0x4dd3204836ff1344ULL, 0x7642d0e5b1b96b59ULL, 0x1745d1745d1745d1ULL, 0x1745d1745d1745dULL }),
    CSC25519({ 0x8276f2caed43a522ULL, 0x792f14152c45405aULL, 0x6666666666666666ULL, 0xe66666666666666ULL }),
    CSC25519({ 0x280e4d148130fa2aULL, 0x8202c257d415ebc3ULL, 0xc71c71c71c71c71cULL, 0xc71c71c71c71c71ULL }),
    CSC25519({ 0xf70b7df07a19a474ULL, 0xd0b5c2b25dac205ULL, 0x0ULL, 0xa00000000000000ULL }),
    CSC25519({ 0x26eb305fbd64fecbULL, 0xc8bf1f511df8cf93ULL, 0xdb6db6db6db6db6dULL, 0xdb6db6db6db6db6ULL }),
    CSC25519({ 0x6403108464d3a352ULL, 0xae2529a51b2944ceULL, 0xaaaaaaaaaaaaaaaaULL, 0x2aaaaaaaaaaaaaaULL }),
    CSC25519({ 0xacdb827b7d917657ULL, 0xdd7f2e4bb592e3deULL, 0xccccccccccccccccULL, 0xcccccccccccccccULL }),
    CSC25519({ 0x960498c6973d74fbULL, 0x537be77a8bde735ULL, 0x0ULL, 0x400000000000000ULL }),
    CSC25519({ 0xc8062108c9a746a4ULL, 0x5c4a534a3652899cULL, 0x5555555555555555ULL, 0x555555555555555ULL }),
    CSC25519({ 0x2c09318d2e7ae9f6ULL, 0xa6f7cef517bce6bULL, 0x0ULL, 0x800000000000000ULL }),
    CSC25519({ 0x5812631a5cf5d3ecULL, 0x14def9dea2f79cd6ULL, 0x0ULL, 0x1000000000000000ULL }),
    CSC25519({ 0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL }),
    CSC25519({ 0x1ULL, 0x0ULL, 0x0ULL, 0x0ULL }),
    CSC25519({ 0x2c09318d2e7ae9f7ULL, 0xa6f7cef517bce6bULL, 0x0ULL, 0x800000000000000ULL }),
    CSC25519({ 0x900c4211934e8d49ULL, 0xb894a6946ca51339ULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaULL }),
    CSC25519({ 0xc20dca53c5b85ef2ULL, 0xfa73b66fa39b5a0ULL, 0x0ULL, 0xc00000000000000ULL }),
    CSC25519({ 0xab36e09edf645d96ULL, 0x375fcb92ed64b8f7ULL, 0x3333333333333333ULL, 0x333333333333333ULL }),
    CSC25519({ 0xf40f5295f822309bULL, 0x66b9d03987ce5807ULL, 0x5555555555555555ULL, 0xd55555555555555ULL }),
    CSC25519({ 0x312732ba9f90d522ULL, 0x4c1fda8d84fecd43ULL, 0x2492492492492492ULL, 0x249249249249249ULL }),
    CSC25519({ 0x6106e529e2dc2f79ULL, 0x7d39db37d1cdad0ULL, 0x0ULL, 0x600000000000000ULL }),
    CSC25519({ 0x30041605dbc4d9c3ULL, 0x92dc3786cee1b113ULL, 0x38e38e38e38e38e3ULL, 0x38e38e38e38e38eULL }),
    CSC25519({ 0xd59b704f6fb22ecbULL, 0x9bafe5c976b25c7bULL, 0x9999999999999999ULL, 0x199999999999999ULL }),
    CSC25519({ 0xa3f42d225f6c0a9ULL, 0x9e9c28f8f13e317dULL, 0xe8ba2e8ba2e8ba2eULL, 0xe8ba2e8ba2e8ba2ULL }),
    CSC25519({ 0x2610dad82a8c0244ULL, 0xbdcc650c1562fa6fULL, 0xaaaaaaaaaaaaaaaaULL, 0xeaaaaaaaaaaaaaaULL }),
    CSC25519({ 0x50087c82524a130bULL, 0x6c18248e23d4be8aULL, 0x7627627627627627ULL, 0x762762762762762ULL }),
    CSC25519({ 0x9893995d4fc86a91ULL, 0x260fed46c27f66a1ULL, 0x9249249249249249ULL, 0x124924924924924ULL }),
    CSC25519({ 0x1e73e246887001d0ULL, 0xcb09ea70111bfb8cULL, 0xbbbbbbbbbbbbbbbbULL, 0xbbbbbbbbbbbbbbbULL }),
    CSC25519({ 0x5c8ca4221fe901b3ULL, 0xe594bc9100a3bd3ULL, 0x0ULL, 0xb00000000000000ULL }),
    CSC25519({ 0x73c5ed8aa186552bULL, 0xe45677bfd6eff455ULL, 0xe1e1e1e1e1e1e1e1ULL, 0x1e1e1e1e1e1e1e1ULL }),
    CSC25519({ 0xc40b3c901c5d56d8ULL, 0xd3dd98b2b8eca6f4ULL, 0x1c71c71c71c71c71ULL, 0x9c71c71c71c71c7ULL }),
    CSC25519({ 0x765b7cbe6bb6be10ULL, 0xf122c68e5e5975bfULL, 0xd79435e50d79435ULL, 0x9435e50d79435e5ULL }),
    CSC25519({ 0x16d6e9b4e654015cULL, 0xd8476fd40cd4fca9ULL, 0xccccccccccccccccULL, 0x8ccccccccccccccULL }),
    CSC25519({ 0x2dbddc9c542ce305ULL, 0x2054f17962a778b3ULL, 0x6186186186186186ULL, 0x618618618618618ULL }),
    CSC25519({ 0xb128d2f641764a4bULL, 0x59bd916bca1ae729ULL, 0x745d1745d1745d17ULL, 0xf45d1745d1745d1ULL }),
    CSC25519({ 0xdfae26c7888cb71aULL, 0x298ee7571e60e453ULL, 0x8590b21642c8590bULL, 0x642c8590b21642cULL }),
    CSC25519({ 0x93086d6c15460122ULL, 0x5ee632860ab17d37ULL, 0x5555555555555555ULL, 0x755555555555555ULL }),
    CSC25519({ 0x23e2cec931412b79ULL, 0x4acc25095e0f1cb2ULL, 0xa3d70a3d70a3d70aULL, 0xa3d70a3d70a3d70ULL }),
    CSC25519({ 0x540d6fce579ff37cULL, 0xc07b8f3663662db0ULL, 0x3b13b13b13b13b13ULL, 0xbb13b13b13b13b1ULL }),
    CSC25519({ 0xf562f413873ad5dfULL, 0x3ede0e6c06f04e3fULL, 0x684bda12f684bda1ULL, 0xbda12f684bda12fULL }),
    CSC25519({ 0xf852fe3bd65f1f3fULL, 0x9d777392b2bb81bbULL, 0x4924924924924924ULL, 0x892492492492492ULL }),
    CSC25519({ 0x4d42dd8bafb692bdULL, 0xf41a33463e9c9532ULL, 0x69ee58469ee58469ULL, 0x469ee58469ee584ULL }),
    CSC25519({ 0xf39f123443800e8ULL, 0xe584f538088dfdc6ULL, 0xddddddddddddddddULL, 0x5ddddddddddddddULL }),
    CSC25519({ 0xe52e090f0fbb4f77ULL, 0x266b173132506a89ULL, 0x4210842108421084ULL, 0x421084210842108ULL }),
    CSC25519({ 0xda4f839e3e6f6ad0ULL, 0x119c22d3d980ec54ULL, 0x0ULL, 0xd80000000000000ULL }),
    CSC25519({ 0xe8cc03024aa0cd81ULL, 0x981e09921264790dULL, 0xf83e0f83e0f83e0fULL, 0xf83e0f83e0f83e0ULL }),
    CSC25519({ 0xe5ec28527f3e148cULL, 0xfc9ab8cf3cf3c895ULL, 0xf0f0f0f0f0f0f0f0ULL, 0x8f0f0f0f0f0f0f0ULL }),
    CSC25519({ 0x3d07d6f21fe9c43aULL, 0x759ff882b432f5daULL, 0x750750750750750ULL, 0x75075075075075ULL }),
    CSC25519({ 0x62059e480e2eab6cULL, 0xe9eecc595c76537aULL, 0x8e38e38e38e38e38ULL, 0x4e38e38e38e38e3ULL }),
    CSC25519({ 0x11540f2dfeaf7e3dULL, 0xed5980cefa5d3ce7ULL, 0x5306eb3e45306eb3ULL, 0x6eb3e45306eb3e4ULL }),
    CSC25519({ 0xbb2dbe5f35db5f08ULL, 0xf89163472f2cbadfULL, 0x86bca1af286bca1aULL, 0x4a1af286bca1af2ULL }),
    CSC25519({ 0xf163d0411e8f7ULL, 0x31f20819234152bdULL, 0xd20d20d20d20d20dULL, 0xd20d20d20d20d20ULL }),
    CSC25519({ 0x8b6b74da732a00aeULL, 0x6c23b7ea066a7e54ULL, 0x6666666666666666ULL, 0x466666666666666ULL }),
    CSC25519({ 0xbdb8baeb70d7d94dULL, 0x1ed0458b4a690696ULL, 0xe0c7ce0c7ce0c7ceULL, 0xe0c7ce0c7ce0c7cULL }),
    CSC25519({ 0xc2e81fdb58915b79ULL, 0x1a99f5ac02cf8ac4ULL, 0x30c30c30c30c30c3ULL, 0xb0c30c30c30c30cULL }),
    CSC25519({ 0xf6550f2c20761f9fULL, 0x7e51d327a529190ULL, 0x7d05f417d05f417dULL, 0x17d05f417d05f41ULL }),
    CSC25519({ 0x49d9b084f360f1cULL, 0xb74e45a536894200ULL, 0xba2e8ba2e8ba2e8bULL, 0xfa2e8ba2e8ba2e8ULL }),
    CSC25519({ 0xb4d14b6cd82555f0ULL, 0xee58a37ab05ea92eULL, 0x3e93e93e93e93e93ULL, 0x3e93e93e93e93e9ULL }),
    CSC25519({ 0xefd71363c4465b8dULL, 0x94c773ab8f307229ULL, 0x42c8590b21642c85ULL, 0x321642c8590b216ULL }),
    CSC25519({ 0x6a963948414cf0d1ULL, 0xb29ed700b937fd28ULL, 0x1b3bea3677d46cefULL, 0x77d46cefa8d9df5ULL }),
    CSC25519({ 0xc98436b60aa30091ULL, 0xaf7319430558be9bULL, 0xaaaaaaaaaaaaaaaaULL, 0x3aaaaaaaaaaaaaaULL }),
    CSC25519({ 0x81515e8c24133cb9ULL, 0xddb42eae12333baULL, 0x5397829cbc14e5eULL, 0x29cbc14e5e0a72fULL }),
    CSC25519({ 0x3dfa98f1c71b7fb3ULL, 0x2fd58f7400835cc4ULL, 0x51eb851eb851eb85ULL, 0xd1eb851eb851eb8ULL }),
};
static const CSC25519* SCInverseZero = &SCInverse[50];

// Lagrange interpolation polynomial. Return f(0)
const uint256 MPLagrange(vector<pair<uint32_t, uint256>>& vShare)
{
    CSC25519 s0;
    for (size_t i = 0; i < vShare.size(); i++)
    {
        CSC25519 l(1);
        for (size_t j = 0; j < vShare.size(); j++)
        {
            if (i != j)
            {
                l *= *(SCInverseZero + vShare[j].first - vShare[i].first) * vShare[j].first;
            }
        }
        s0 += l * CSC25519(vShare[i].second.begin());
    }
    return uint256(s0.Data());
}

// Newton interpolation polynomial. Return f(0)
// f(x) = f(x0) + (x-x0)*f(x0,x1) + (x-x0)*(x-x1)*f(x0,x1,x2) + ... + (x-x0)*(x-x1)*...*(x - xn-1))*f(x0,x1,...,xn)
//
// vDiffSet:
//      f(x0)
//      f(x1)   f(x0, x1)
//      f(x2)   f(x1, x2)   f(x0, x1, x2)
//      f(x3)   f(x2, x3)   f(x1, x2, x3)   f(x0, x1, x2, x3)
//      ...
//      f(xn)   f(xn, xn-1) ... f(x0, x1, ..., xn)
//
// coeff:
//      f(x0, x1, ..., xn) = (f(x1, x2, ..., xn) - f(x0, x1, ..., xn-1)) / (xn - x0)
const uint256 MPNewton(vector<pair<uint32_t, uint256>>& vShare)
{
    vector<vector<CSC25519>> vDiffSet;
    vDiffSet.reserve(vShare.size());

    CSC25519 basis(1);

    CSC25519 s0;
    for (size_t i = 0; i < vShare.size(); i++)
    {
        vector<CSC25519> vDiff;
        vDiff.reserve(i + 1);
        vDiff.push_back(CSC25519(vShare[i].second.begin()));

        for (size_t j = 0; j < i; j++)
        {
            CSC25519 coeff = vDiff[j] - vDiffSet[i - 1][j];
            const CSC25519& inverse = *(SCInverseZero + vShare[i].first - vShare[i - j - 1].first);
            coeff *= inverse;
            vDiff.push_back(coeff);
        }

        s0 += basis * vDiff.back();
        basis *= vShare[i].first;
        basis.Negative();
        vDiffSet.push_back(std::move(vDiff));
    }
    return uint256(s0.Data());
}
