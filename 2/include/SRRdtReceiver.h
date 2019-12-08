#ifndef SR_RDT_RECEIVER_H
#define SR_RDT_RECEIVER_H

#include "RdtReceiver.h"

class SRRdtReceiver : public RdtReceiver {
private:
    const int maxSeq;
    const int windowSize;
    int base = 0;
    vector<pair<Packet, bool>> packets;

    static bool isCorrupted(const struct Packet &packet);

    static Message extract(const struct Packet &packet);

    static void deliverData(const Message &message);

    static Packet makePacket(int ackNum);

    static void udtSend(const struct Packet &packet);

    static void printLog(const string &message, const struct Packet &packet);

    static string generateLog(const string &event, const string &color, bool sending);

    bool inWindow(int seqNum);

public:
    // callback on data from sender
    void receive(const Packet &packet) override;

    SRRdtReceiver(int maxSeq, int windowSize);

    ~SRRdtReceiver() override;
};

#endif

