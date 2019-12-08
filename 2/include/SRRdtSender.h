#ifndef SR_RDT_SENDER_H
#define SR_RDT_SENDER_H

#include "RdtSender.h"

class SRRdtSender : public RdtSender {
private:
    const int maxSeq;
    const int windowSize;
    int base = 0;
    int nextSeqNum = 0;
    vector<pair<Packet, bool>> packets;

    static bool isCorrupted(const struct Packet &packet);

    Packet makePacket(int seqNum, const Message &message);

    static void udtSend(const struct Packet &packet);

    static void startTimer(int seqNum);

    static void stopTimer(int seqNum);

    static void printLog(const string &message, const struct Packet &packet);

    static string generateLog(const string &event, const string &color, bool sending);

    bool inWindow(int seqNum);

public:

    // whether is waiting for ack
    bool getWaitingState() override;

    // callback on message from application layer
    bool send(const Message &message) override;

    // callback on ack from receiver
    void receive(const Packet &packet) override;

    // callback on timeout
    void timeoutHandler(int seqNum) override;

    SRRdtSender(int maxSeq, int windowSize);

    ~SRRdtSender() override;
};

#endif

