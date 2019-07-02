#ifndef BITCOIN_TICKET_H
#define BITCOIN_TICKET_H

#include <script/script.h>
#include <pubkey.h>

CScript GenerateTicketScript(const CPubKey keyid, const int lockHeight);

bool GetPublicKeyFromScript(const CScript script, CPubKey& pubkey);
bool GetRedeemFromScript(const CScript script, CScript& redeemscript);

class CTicket {
public:
    static const int32_t VERSION = 1;

    enum CTicketState {
        IMMATURATE = 0,
        USEABLE,
        OVERDUE,
		UNKNOW
    };

    CTicket(const uint256& txid, const uint32_t n, const CScript& redeemScript, const CScript &scriptPubkey);

    CTicket() = default;
    ~CTicket() = default;

    CTicketState State(int activeHeight) const;
   
	bool LockTime(int &height)const;

    CPubKey PublicKey() const;

    bool Invalid() const;
private:
    uint256 txid;
    uint32_t n;
    CScript redeemScript;
    CScript scriptPubkey;
};

typedef std::shared_ptr<const CTicket> CTicketRef;
#endif
