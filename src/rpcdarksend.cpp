// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcredit developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activebasenode.h"
#include "amount.h"
#include "basenode.h"
#include "basenodeman.h"
#include "basenodeconfig.h"
#include <boost/lexical_cast.hpp>
#include "db.h"
#include "darksend.h"
#include "init.h"
#include "main.h"
#include "primitives/transaction.h"
#include "rpc/server.h"
#include "univalue/include/univalue.h"
#include "util.h"
#include "utilmoneystr.h"

#include <fstream>
#include <iomanip>
using namespace std;


UniValue basenodelist(const UniValue& params, bool fHelp)
{
    std::string strMode = "status";
    std::string strFilter = "";

    if (params.size() >= 1) strMode = params[0].getValStr().c_str();
    if (params.size() == 2) strFilter = params[1].getValStr().c_str();

    if (fHelp ||
            (strMode != "status" && strMode != "vin" && strMode != "pubkey" && strMode != "lastseen" && strMode != "activeseconds" && strMode != "rank"
                && strMode != "protocol" && strMode != "full" && strMode != "votes" && strMode != "donation" && strMode != "pose"))
    {
        throw runtime_error(
                "basenodelist ( \"mode\" \"filter\" )\n"
                "Get a list of basenodes in different modes\n"
                "\nArguments:\n"
                "1. \"mode\"      (string, optional/required to use filter, defaults = status) The mode to run list in\n"
                "2. \"filter\"    (string, optional) Filter results. Partial match by IP by default in all modes, additional matches in some modes\n"
                "\nAvailable modes:\n"
                "  activeseconds  - Print number of seconds basenode recognized by the network as enabled\n"
                "  donation       - Show donation settings\n"
                "  full           - Print info in format 'status protocol pubkey vin lastseen activeseconds' (can be additionally filtered, partial match)\n"
                "  lastseen       - Print timestamp of when a basenode was last seen on the network\n"
                "  pose           - Print Proof-of-Service score\n"
                "  protocol       - Print protocol of a basenode (can be additionally filtered, exact match))\n"
                "  pubkey         - Print public key associated with a basenode (can be additionally filtered, partial match)\n"
                "  rank           - Print rank of a basenode based on current block\n"
                "  status         - Print basenode status: ENABLED / EXPIRED / VIN_SPENT / REMOVE / POS_ERROR (can be additionally filtered, partial match)\n"
                "  vin            - Print vin associated with a basenode (can be additionally filtered, partial match)\n"
                "  votes          - Print all basenode votes for a Dash initiative (can be additionally filtered, partial match)\n"
                );
    }

    UniValue obj(UniValue::VOBJ);
    if (strMode == "rank") {
        std::vector<pair<int, CBasenode> > vBasenodeRanks = mnodeman.GetBasenodeRanks(chainActive.Tip()->nHeight);
        BOOST_FOREACH(PAIRTYPE(int, CBasenode)& s, vBasenodeRanks) {
            std::string strAddr = s.second.addr.ToString();
            if(strFilter !="" && strAddr.find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strAddr,       s.first));
        }
    } else {
        std::vector<CBasenode> vBasenodes = mnodeman.GetFullBasenodeVector();
        BOOST_FOREACH(CBasenode& mn, vBasenodes) {
            std::string strAddr = mn.addr.ToString();
            if (strMode == "activeseconds") {
                if(strFilter !="" && strAddr.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strAddr,       (int64_t)(mn.lastTimeSeen - mn.sigTime)));
            } else if (strMode == "full") {
                CScript pubkey;
                pubkey=GetScriptForDestination(mn.pubkey.GetID());
                CTxDestination address1;
                ExtractDestination(pubkey, address1);
                CBitcreditAddress address2(address1);

                std::ostringstream addrStream;
                addrStream << std::setw(21) << strAddr;

                std::ostringstream stringStream;
                stringStream << std::setw(10) <<
                               mn.Status() << " " <<
                               mn.protocolVersion << " " <<
                               address2.ToString() << " " <<
                               mn.vin.prevout.hash.ToString() << " " <<
                               mn.lastTimeSeen << " " << setw(8) <<
                               (mn.lastTimeSeen - mn.sigTime);
                std::string output = stringStream.str();
                stringStream << " " << strAddr;
                if(strFilter !="" && stringStream.str().find(strFilter) == string::npos &&
                        strAddr.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(addrStream.str(), output));
            } else if (strMode == "lastseen") {
                if(strFilter !="" && strAddr.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strAddr,       (int64_t)mn.lastTimeSeen));
            } else if (strMode == "protocol") {
                if(strFilter !="" && strFilter != boost::lexical_cast<std::string>(mn.protocolVersion) &&
                    strAddr.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strAddr,       (int64_t)mn.protocolVersion));
            } else if (strMode == "pubkey") {
                CScript pubkey;
                pubkey=GetScriptForDestination(mn.pubkey.GetID());
                CTxDestination address1;
                ExtractDestination(pubkey, address1);
                CBitcreditAddress address2(address1);

                if(strFilter !="" && address2.ToString().find(strFilter) == string::npos &&
                    strAddr.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strAddr,       address2.ToString().c_str()));
            } else if (strMode == "pose") {
                if(strFilter !="" && strAddr.find(strFilter) == string::npos) continue;
                std::string strOut = boost::lexical_cast<std::string>(mn.nScanningErrorCount);
                obj.push_back(Pair(strAddr,       strOut.c_str()));
            } else if(strMode == "status") {
                std::string strStatus = mn.Status();
                if(strFilter !="" && strAddr.find(strFilter) == string::npos && strStatus.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strAddr,       strStatus.c_str()));
            } else if (strMode == "vin") {
                if(strFilter !="" && mn.vin.prevout.hash.ToString().find(strFilter) == string::npos &&
                    strAddr.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strAddr,       mn.vin.prevout.hash.ToString().c_str()));
            } else if(strMode == "votes"){
                std::string strStatus = "ABSTAIN";

                //voting lasts 30 days, ignore the last vote if it was older than that
                if((GetAdjustedTime() - mn.lastVote) < (60*60*30*24))
                {
                    if(mn.nVote == -1) strStatus = "NAY";
                    if(mn.nVote == 1) strStatus = "YEA";
                }

                if(strFilter !="" && (strAddr.find(strFilter) == string::npos && strStatus.find(strFilter) == string::npos)) continue;
                obj.push_back(Pair(strAddr,       strStatus.c_str()));
            }
        }
    }
    return obj;

}


UniValue basenode(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].getValStr().c_str();

    if (fHelp  ||
        (strCommand != "start" && strCommand != "mincollateral" && strCommand != "maxbnssupply" && strCommand != "start-alias" && strCommand != "start-many" && strCommand != "stop" && strCommand != "stop-alias" && strCommand != "stop-many" && strCommand != "list" && strCommand != "list-conf" && strCommand != "count"  && strCommand != "enforce"
            && strCommand != "debug" && strCommand != "current" && strCommand != "winners" && strCommand != "genkey" && strCommand != "connect" && strCommand != "outputs" && strCommand != "vote-many" && strCommand != "vote"))
        throw runtime_error(
                "basenode \"command\"... ( \"passphrase\" )\n"
                "Set of commands to execute basenode related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  count        - Print number of all known basenodes (optional: 'enabled', 'both')\n"
                "  current      - Print info on current basenode winner\n"
                "  debug        - Print basenode status\n"
                "  genkey       - Generate new basenodeprivkey\n"
                "  mincollateral- Minimum basenode collateral to start a BN\n"
                "  maxbnssupply - Maximum basenodes collateral\n"
                "  enforce      - Enforce basenode payments\n"
                "  outputs      - Print basenode compatible outputs\n"
                "  start        - Start basenode configured in basenode.conf\n"
                "  start-alias  - Start single basenode by assigned alias configured in basenode.conf\n"
                "  start-many   - Start all basenodes configured in basenode.conf\n"
                "  stop         - Stop basenode configured in basenode.conf\n"
                "  stop-alias   - Stop single basenode by assigned alias configured in basenode.conf\n"
                "  stop-many    - Stop all basenodes configured in basenode.conf\n"
                "  list         - Print list of all known basenodes (see basenodelist for more info)\n"
                "  list-conf    - Print basenode.conf in JSON format\n"
                "  winners      - Print list of basenode winners\n"
                "  vote-many    - Vote on a BCR initiative\n"
                "  vote         - Vote on a BCR initiative\n"
                );

    if (strCommand == "mincollateral")
        {
    		return (uint64_t)getBasenodeMinimumCollateral();
        }
    if (strCommand == "maxbnssupply")
        {
    		return (uint64_t)getBasenodeMaximumCollateralSum();
        }

    if (strCommand == "stop")
    {
        if(!fBaseNode) return "you must set basenode=1 in the configuration";

        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2){
                strWalletPass = params[1].getValStr().c_str();
            } else {
                throw runtime_error(
                    "Your wallet is locked, passphrase is required\n");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                return "incorrect passphrase";
            }
        }

        std::string errorMessage;
        if(!activeBasenode.StopBaseNode(errorMessage)) {
        	return "stop failed: " + errorMessage;
        }
        pwalletMain->Lock();

        if(activeBasenode.status == BASENODE_STOPPED) return "successfully stopped basenode";
        if(activeBasenode.status == BASENODE_NOT_CAPABLE) return "not capable basenode";

        return "unknown";
    }

    if (strCommand == "stop-alias")
    {
	    if (params.size() < 2){
			throw runtime_error(
			"command needs at least 2 parameters\n");
	    }

	    std::string alias = params[1].getValStr().c_str();

    	if(pwalletMain->IsLocked()) {
    		SecureString strWalletPass;
    	    strWalletPass.reserve(100);

			if (params.size() == 3){
				strWalletPass = params[2].getValStr().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
        }

    	bool found = false;

    	UniValue statusObj(UniValue::VOBJ);
		statusObj.push_back(Pair("alias", alias));

    	BOOST_FOREACH(CBasenodeConfig::CBasenodeEntry mne, basenodeConfig.getEntries()) {
    		if(mne.getAlias() == alias) {
    			found = true;
    			std::string errorMessage;
    			bool result = activeBasenode.StopBaseNode(mne.getIp(), mne.getPrivKey(), errorMessage);

				statusObj.push_back(Pair("result", result ? "successful" : "failed"));
    			if(!result) {
   					statusObj.push_back(Pair("errorMessage", errorMessage));
   				}
    			break;
    		}
    	}

    	if(!found) {
    		statusObj.push_back(Pair("result", "failed"));
    		statusObj.push_back(Pair("errorMessage", "could not find alias in config. Verify with list-conf."));
    	}

    	pwalletMain->Lock();
    	return statusObj;
    }

    if (strCommand == "stop-many")
    {
    	if(pwalletMain->IsLocked()) {
			SecureString strWalletPass;
			strWalletPass.reserve(100);

			if (params.size() == 2){
				strWalletPass = params[1].getValStr().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
		}

		int total = 0;
		int successful = 0;
		int fail = 0;


		UniValue resultsObj(UniValue::VOBJ);

		BOOST_FOREACH(CBasenodeConfig::CBasenodeEntry mne, basenodeConfig.getEntries()) {
			total++;

			std::string errorMessage;
			bool result = activeBasenode.StopBaseNode(mne.getIp(), mne.getPrivKey(), errorMessage);

			UniValue statusObj(UniValue::VOBJ);
			statusObj.push_back(Pair("alias", mne.getAlias()));
			statusObj.push_back(Pair("result", result ? "successful" : "failed"));

			if(result) {
				successful++;
			} else {
				fail++;
				statusObj.push_back(Pair("errorMessage", errorMessage));
			}

			resultsObj.push_back(Pair("status", statusObj));
		}
		pwalletMain->Lock();

		UniValue returnObj(UniValue::VOBJ);
		returnObj.push_back(Pair("overall", "Successfully stopped " + boost::lexical_cast<std::string>(successful) + " basenodes, failed to stop " +
				boost::lexical_cast<std::string>(fail) + ", total " + boost::lexical_cast<std::string>(total)));
		returnObj.push_back(Pair("detail", resultsObj));

		return returnObj;

    }

    /*
    if (strCommand == "list")
    {
    	std::string newParams[params.size() - 1];
        std::copy(params. + 1, params.end(), newParams->begin());
        return basenodelist(newParams, fHelp);
    }
    */

    if (strCommand == "count")
    {
        if (params.size() > 2){
            throw runtime_error(
            "too many parameters\n");
        }
        if (params.size() == 2)
        {
        	std::string par = params[1].getValStr().c_str();
            if(par == "enabled") return mnodeman.CountEnabled();
            if(par == "both") return boost::lexical_cast<std::string>(mnodeman.CountEnabled()) + " / " + boost::lexical_cast<std::string>(mnodeman.size());
        }
        return mnodeman.size();
    }

    if (strCommand == "start")
    {
        if(!fBaseNode) return "you must set basenode=1 in the configuration";

        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2){
                strWalletPass = params[1].getValStr().c_str();
            } else {
                throw runtime_error(
                    "Your wallet is locked, passphrase is required\n");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                return "incorrect passphrase";
            }
        }

        if(activeBasenode.status != BASENODE_REMOTELY_ENABLED && activeBasenode.status != BASENODE_IS_CAPABLE){
            activeBasenode.status = BASENODE_NOT_PROCESSED; // TODO: consider better way
            std::string errorMessage;
            activeBasenode.ManageStatus();
            pwalletMain->Lock();
        }

        if(activeBasenode.status == BASENODE_REMOTELY_ENABLED) return "basenode started remotely";
        if(activeBasenode.status == BASENODE_INPUT_TOO_NEW) return "basenode input must have at least 15 confirmations";
        if(activeBasenode.status == BASENODE_STOPPED) return "basenode is stopped";
        if(activeBasenode.status == BASENODE_IS_CAPABLE) return "successfully started basenode";
        if(activeBasenode.status == BASENODE_NOT_CAPABLE) return "not capable basenode: " + activeBasenode.notCapableReason;
        if(activeBasenode.status == BASENODE_SYNC_IN_PROCESS) return "sync in process. Must wait until client is synced to start.";

        return "unknown";
    }

    if (strCommand == "start-alias")
    {
	    if (params.size() < 2){
			throw runtime_error(
			"command needs at least 2 parameters\n");
	    }

	    std::string alias = params[1].getValStr().c_str();

    	if(pwalletMain->IsLocked()) {
    		SecureString strWalletPass;
    	    strWalletPass.reserve(100);

			if (params.size() == 3){
				strWalletPass = params[2].getValStr().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
        }

    	bool found = false;

    	UniValue statusObj(UniValue::VOBJ);
		statusObj.push_back(Pair("alias", alias));

    	BOOST_FOREACH(CBasenodeConfig::CBasenodeEntry mne, basenodeConfig.getEntries()) {
    		if(mne.getAlias() == alias) {
    			found = true;
    			std::string errorMessage;
    			bool result = activeBasenode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage);

    			statusObj.push_back(Pair("result", result ? "successful" : "failed"));
    			if(!result) {
					statusObj.push_back(Pair("errorMessage", errorMessage));
				}
    			break;
    		}
    	}

    	if(!found) {
    		statusObj.push_back(Pair("result", "failed"));
    		statusObj.push_back(Pair("errorMessage", "could not find alias in config. Verify with list-conf."));
    	}

    	pwalletMain->Lock();
    	return statusObj;

    }

    if (strCommand == "start-many")
    {
    	if(pwalletMain->IsLocked()) {
			SecureString strWalletPass;
			strWalletPass.reserve(100);

			if (params.size() == 2){
				strWalletPass = params[1].getValStr().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
		}

		std::vector<CBasenodeConfig::CBasenodeEntry> mnEntries;
		mnEntries = basenodeConfig.getEntries();

		int total = 0;
		int successful = 0;
		int fail = 0;

		UniValue resultsObj(UniValue::VOBJ);

		BOOST_FOREACH(CBasenodeConfig::CBasenodeEntry mne, basenodeConfig.getEntries()) {
			total++;

			std::string errorMessage;

			CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            //CBasenode *pmn = mnodeman.Find(vin);

			bool result = activeBasenode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage);

			UniValue statusObj(UniValue::VOBJ);
			statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", result ? "successful" : "failed"));

			if(result) {
				successful++;
			} else {
				fail++;
				statusObj.push_back(Pair("errorMessage", errorMessage));
			}

			resultsObj.push_back(Pair("status", statusObj));
		}
		pwalletMain->Lock();

		UniValue returnObj(UniValue::VOBJ);
		returnObj.push_back(Pair("overall", "Successfully started " + boost::lexical_cast<std::string>(successful) + " basenodes, failed to start " +
				boost::lexical_cast<std::string>(fail) + ", total " + boost::lexical_cast<std::string>(total)));
		returnObj.push_back(Pair("detail", resultsObj));

		return returnObj;
    }

    if (strCommand == "debug")
    {
        if(activeBasenode.status == BASENODE_REMOTELY_ENABLED) return "basenode started remotely";
        if(activeBasenode.status == BASENODE_INPUT_TOO_NEW) return "basenode input must have at least 15 confirmations";
        if(activeBasenode.status == BASENODE_IS_CAPABLE) return "successfully started basenode";
        if(activeBasenode.status == BASENODE_STOPPED) return "basenode is stopped";
        if(activeBasenode.status == BASENODE_NOT_CAPABLE) return "not capable basenode: " + activeBasenode.notCapableReason;
        if(activeBasenode.status == BASENODE_SYNC_IN_PROCESS) return "sync in process. Must wait until client is synced to start.";

        CTxIn vin = CTxIn();
        CPubKey pubkey = CPubKey();
        CKey key;
        bool found = activeBasenode.GetBaseNodeVin(vin, pubkey, key);
        if(!found){
            return "Missing basenode input, please look at the documentation for instructions on basenode creation";
        } else {
            return "No problems were found";
        }
    }

    if (strCommand == "create")
    {

        return "Not implemented yet, please look at the documentation for instructions on basenode creation";
    }

    if (strCommand == "current")
    {
        CBasenode* winner = mnodeman.GetCurrentBaseNode(1);
        if(winner) {
        	UniValue obj(UniValue::VOBJ);
            CScript pubkey;
            pubkey=GetScriptForDestination(winner->pubkey.GetID());
            CTxDestination address1;
            ExtractDestination(pubkey, address1);
            CBitcreditAddress address2(address1);

            obj.push_back(Pair("IP:port",       winner->addr.ToString().c_str()));
            obj.push_back(Pair("protocol",      (int64_t)winner->protocolVersion));
            obj.push_back(Pair("vin",           winner->vin.prevout.hash.ToString().c_str()));
            obj.push_back(Pair("pubkey",        address2.ToString().c_str()));
            obj.push_back(Pair("lastseen",      (int64_t)winner->lastTimeSeen));
            obj.push_back(Pair("activeseconds", (int64_t)(winner->lastTimeSeen - winner->sigTime)));
            return obj;
        }

        return "unknown";
    }

    if (strCommand == "genkey")
    {
        CKey secret;
        secret.MakeNewKey(false);

        return CBitcreditSecret(secret).ToString();
    }

    if (strCommand == "winners")
    {
    	UniValue obj(UniValue::VOBJ);

        for(int nHeight = chainActive.Tip()->nHeight-10; nHeight < chainActive.Tip()->nHeight+20; nHeight++)
        {
            CScript payee;
            if(basenodePayments.GetBlockPayee(nHeight, payee)){
                CTxDestination address1;
                ExtractDestination(payee, address1);
                CBitcreditAddress address2(address1);
                obj.push_back(Pair(boost::lexical_cast<std::string>(nHeight),       address2.ToString().c_str()));
            } else {
                obj.push_back(Pair(boost::lexical_cast<std::string>(nHeight),       ""));
            }
        }

        return obj;
    }

    if(strCommand == "enforce")
    {
        return (uint64_t)enforceBasenodePaymentsTime;
    }

    if(strCommand == "connect")
    {
        std::string strAddress = "";
        if (params.size() == 2){
            strAddress = params[1].getValStr().c_str();
        } else {
            throw runtime_error(
                "Basenode address required\n");
        }

        CService addr = CService(strAddress);

        if(ConnectNode((CAddress)addr, NULL)){
            return "successfully connected";
        } else {
            return "error connecting";
        }
    }

    if(strCommand == "list-conf")
    {
    	std::vector<CBasenodeConfig::CBasenodeEntry> mnEntries;
    	mnEntries = basenodeConfig.getEntries();

    	UniValue resultObj(UniValue::VOBJ);

        BOOST_FOREACH(CBasenodeConfig::CBasenodeEntry mne, basenodeConfig.getEntries()) {
        	UniValue mnObj(UniValue::VOBJ);
    		mnObj.push_back(Pair("alias", mne.getAlias()));
    		mnObj.push_back(Pair("address", mne.getIp()));
    		mnObj.push_back(Pair("privateKey", mne.getPrivKey()));
    		mnObj.push_back(Pair("txHash", mne.getTxHash()));
    		mnObj.push_back(Pair("outputIndex", mne.getOutputIndex()));
    		resultObj.push_back(Pair("basenode", mnObj));
    	}

        return resultObj;
    }

    if (strCommand == "outputs"){
        // Find possible candidates
        vector<COutput> possibleCoins = activeBasenode.SelectCoinsBasenode();

        UniValue obj(UniValue::VOBJ);
        BOOST_FOREACH(COutput& out, possibleCoins) {
            obj.push_back(Pair(out.tx->GetHash().ToString().c_str(), boost::lexical_cast<std::string>(out.i)));
        }

        return obj;

    }

    if(strCommand == "vote-many")
    {
        std::vector<CBasenodeConfig::CBasenodeEntry> mnEntries;
        mnEntries = basenodeConfig.getEntries();

        if (params.size() != 2)
            throw runtime_error("You can only vote 'yea' or 'nay'");

        std::string vote = params[1].getValStr().c_str();
        if(vote != "yea" && vote != "nay") return "You can only vote 'yea' or 'nay'";
        int nVote = 0;
        if(vote == "yea") nVote = 1;
        if(vote == "nay") nVote = -1;


        int success = 0;
        int failed = 0;

        UniValue resultObj(UniValue::VOBJ);

        BOOST_FOREACH(CBasenodeConfig::CBasenodeEntry mne, basenodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchBaseNodeSignature;
            std::string strBaseNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyBasenode;
            CKey keyBasenode;

            if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyBasenode, pubKeyBasenode)){
                printf(" Error upon calling SetKey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            CBasenode* pmn = mnodeman.Find(pubKeyBasenode);
            if(pmn == NULL)
            {
                printf("Can't find basenode by pubkey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            std::string strMessage = pmn->vin.ToString() + boost::lexical_cast<std::string>(nVote);

            if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchBaseNodeSignature, keyBasenode)){
                printf(" Error upon calling SignMessage for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            if(!darkSendSigner.VerifyMessage(pubKeyBasenode, vchBaseNodeSignature, strMessage, errorMessage)){
                printf(" Error upon calling VerifyMessage for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            success++;

            //send to all peers
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
                pnode->PushMessage("mvote", pmn->vin, vchBaseNodeSignature, nVote);
        }

        return("Voted successfully " + boost::lexical_cast<std::string>(success) + " time(s) and failed " + boost::lexical_cast<std::string>(failed) + " time(s).");
    }

    if(strCommand == "vote")
    {
        std::vector<CBasenodeConfig::CBasenodeEntry> mnEntries;
        mnEntries = basenodeConfig.getEntries();

        if (params.size() != 2)
            throw runtime_error("You can only vote 'yea' or 'nay'");

        std::string vote = params[1].getValStr().c_str();
        if(vote != "yea" && vote != "nay") return "You can only vote 'yea' or 'nay'";
        int nVote = 0;
        if(vote == "yea") nVote = 1;
        if(vote == "nay") nVote = -1;

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;
        CPubKey pubKeyBasenode;
        CKey keyBasenode;

        std::string errorMessage;
        std::vector<unsigned char> vchBaseNodeSignature;
        std::string strMessage = activeBasenode.vin.ToString() + boost::lexical_cast<std::string>(nVote);

        if(!darkSendSigner.SetKey(strBaseNodePrivKey, errorMessage, keyBasenode, pubKeyBasenode))
            return(" Error upon calling SetKey");

        if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchBaseNodeSignature, keyBasenode))
            return(" Error upon calling SignMessage");

        if(!darkSendSigner.VerifyMessage(pubKeyBasenode, vchBaseNodeSignature, strMessage, errorMessage))
            return(" Error upon calling VerifyMessage");

        //send to all peers
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            pnode->PushMessage("mvote", activeBasenode.vin, vchBaseNodeSignature, nVote);

    }

    return UniValue::VNULL;
}


