
// Copyright (c) 2009-2012 The Darkcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SPORK_H
#define SPORK_H

//#include "bignum.h"
#include "arith_uint256.h"
#include "uint256.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "utilstrencodings.h"
#include "script/script.h"
#include "base58.h"
#include "main.h"
#include "protocol.h"
#include "basenode.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

// Don't ever reuse these IDs for other sporks
#define SPORK_1_BASENODE_PAYMENTS_ENFORCEMENT               10000
#define SPORK_2_MAX_VALUE                                     10002
#define SPORK_3_REPLAY_BLOCKS                                 10003
#define SPORK_4_NOTUSED                                       10004
#define SPORK_5_BASENODE_SCANNING                           10005

#define SPORK_1_BASENODE_PAYMENTS_ENFORCEMENT_DEFAULT       1427803200  //2015-3-5
#define SPORK_2_MAX_VALUE_DEFAULT                             50000        //50000 BCR :NOTE TO BE CHANGED ?!?!
#define SPORK_3_REPLAY_BLOCKS_DEFAULT                         0
#define SPORK_4_RECONVERGE_DEFAULT                            1420070400  //2047-1-1
#define SPORK_5_BASENODE_SCANNING_DEFAULT                   978307200   //2001-1-1

class CSporkMessage;
class CSporkManager;

extern std::map<uint256, CSporkMessage> mapSporks;
extern std::map<int, CSporkMessage> mapSporksActive;
extern CSporkManager sporkManager;

void ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
int GetSporkValue(int nSporkID);
bool IsSporkActive(int nSporkID);
void ExecuteSpork(int nSporkID, int nValue);

//
// Spork Class
// Keeps track of all of the network spork settings
//

class CSporkMessage
{
public:
    std::vector<unsigned char> vchSig;
    int nSporkID;
    int64_t nValue;
    int64_t nTimeSigned;

    uint256 GetHash(){
        uint256 n = Hash(BEGIN(nSporkID), END(nTimeSigned));
        return n;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nSporkID);
        READWRITE(nValue);
        READWRITE(nTimeSigned);
        READWRITE(vchSig);
	}
};


class CSporkManager
{
private:
    std::vector<unsigned char> vchSig;

    std::string strMasterPrivKey;
    std::string strTestPubKey;
    std::string strMainPubKey;

public:

    CSporkManager() {
        strMainPubKey = "0438690e610b546c8cdb145dcd2ef587729a7471857d91eb88d694d6ac1b076499367aeb4898d0fd5cfea5f85ad560b94325c5365dea936dde2175ccd28fa7f407";
        strTestPubKey = "046dde3fcb0e940f9a6efdff6bd3fc0d28e23245ddbcbde1ae60b1302120d0116c43e9b83248cbde3c8f2aa7ce130b8b12cba8fe799020eaa34cfe8a392941909f";
    }

    std::string GetSporkNameByID(int id);
    int GetSporkIDByName(std::string strName);
    bool UpdateSpork(int nSporkID, int64_t nValue);
    bool SetPrivKey(std::string strPrivKey);
    bool CheckSignature(CSporkMessage& spork);
    bool Sign(CSporkMessage& spork);
    void Relay(CSporkMessage& msg);

};

#endif
