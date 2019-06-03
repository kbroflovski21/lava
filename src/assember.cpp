#include "assember.h"
#include <boost/bind.hpp>
#include "chainparams.h"
#include "logging.h"
#include "miner.h"
#include "poc.h"
#include "util/time.h"
#include "validation.h"

CPOCBlockAssember::CPOCBlockAssember()
    : scheduler(std::make_shared<CScheduler>())
{
    setNull();
    auto f = boost::bind(&CScheduler::serviceQueue, scheduler.get());
    thread = std::make_shared<boost::thread>(f);
    const auto interval = 200;
    scheduler->scheduleEvery(std::bind(&CPOCBlockAssember::checkDeadline, this), interval);
}

CPOCBlockAssember::~CPOCBlockAssember()
{
    thread->interrupt();
}

bool CPOCBlockAssember::UpdateDeadline(const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script)
{
    auto params = Params();
    if (deadline / chainActive.Tip()->nBaseTarget > params.GetTargetDeadline()) {
        LogPrintf("Invalid deadline %uul\n", deadline);
        return false;
    }

    if (this->deadline != 0 && deadline >= this->deadline) {
        LogPrintf("Invalid deadline %uul\n", deadline);
        return false;
    }
    auto block = chainActive.Tip()->GetBlockHeader();
    auto generationSignature = CalcGenerationSignature(block.genSign, block.nPlotID);
    if (CalcDeadline(generationSignature, chainActive.Tip()->nHeight + 1, plotID, nonce) != deadline) {
        //TODO... log
        LogPrintf("Deadline inconformity %uul\n", deadline);
        return false;
    }
    auto ts = (deadline / chainActive.Tip()->nBaseTarget);
    LogPrintf("Update new deadline: %u, now: %u\n", ts, GetTimeMillis() / 1000);
    this->genSig = genSig;
    this->plotID = plotID;
    this->deadline = deadline;
    this->nonce = nonce;
    this->scriptPubKeyIn = script;
    return true;
}

void CPOCBlockAssember::CreateNewBlock(const CScript& scriptPubKeyIn)
{
    LogPrintf("CPOCBlockAssember CreateNewBlock, plotid: %u nonce:%u deadline:%u\n", plotID, nonce, deadline);
    auto params = Params();
    auto blk = BlockAssembler(params).CreateNewBlock(scriptPubKeyIn, nonce, plotID, deadline);
    if (blk) {
        uint32_t extraNonce = 0;
        IncrementExtraNonce(&blk->block, chainActive.Tip(), extraNonce);
        auto pblk = std::make_shared<CBlock>(blk->block);
        if (ProcessNewBlock(params, pblk, true, NULL) == false) {
            LogPrintf("ProcessNewBlock failed\n");
        }
    } else {
        LogPrintf("CreateNewBlock failed\n");
    }
}

void CPOCBlockAssember::checkDeadline()
{
    LOCK(cs_main);
    if (deadline == 0)
        return;
    auto lastBlockTime = chainActive.Tip()->GetBlockHeader().GetBlockTime();
    auto ts = ((deadline / chainActive.Tip()->nBaseTarget) * 1000);
    auto dl = (lastBlockTime * 1000) + ts;
    if (GetTimeMillis() >= dl) {
        CreateNewBlock(scriptPubKeyIn);
        setNull();
    }
}

void CPOCBlockAssember::setNull()
{
    plotID = 0;
    nonce = 0;
    deadline = 0;
    genSig.SetNull();
    scriptPubKeyIn.clear();
}

void CPOCBlockAssember::Interrupt()
{
    thread->interrupt();
}