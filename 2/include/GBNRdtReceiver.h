#ifndef GBN_RDT_RECEIVER_H
#define GBN_RDT_RECEIVER_H

#include "RdtReceiver.h"

class GBNRdtReceiver : public RdtReceiver {
private:
    const int maxSeq;
    const int windowSize;
    int expectedSeqNum = 0;
    Packet lastAckPacket;

    static bool isCorrupted(const struct Packet &packet);

    bool hasSeqNum(const struct Packet &packet);

    static Message extract(const struct Packet &packet);

    static void deliverData(const Message &message);

    static Packet makePacket(int ackNum);

    static void udtSend(const struct Packet &packet);

    static void printLog(const string &message, const struct Packet &packet);

    static string generateLog(const string &event, const string &color, bool sending);

public:
    // callback on data from sender
    void receive(const Packet &packet) override;

    GBNRdtReceiver(int maxSeq, int windowSize);

    ~GBNRdtReceiver() override;
};

#endif

