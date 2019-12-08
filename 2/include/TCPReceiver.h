#ifndef TCP_RECEIVER_H
#define TCP_RECEIVER_H

#include "RdtReceiver.h"

class TCPReceiver : public RdtReceiver {
private:
    const int maxSeq;
    const int windowSize;
    int base = 0;
    Packet lastAckPacket;
    vector<pair<Packet, bool>> packets;

    static bool isCorrupted(const struct Packet &packet);

    bool inWindow(int seqNum);

    static Message extract(const struct Packet &packet);

    static void deliverData(const Message &message);

    static Packet makePacket(int ackNum);

    static void udtSend(const struct Packet &packet);

    static void printLog(const string &message, const struct Packet &packet);

    static string generateLog(const string &event, const string &color, bool sending);

public:
    // callback on data from sender
    void receive(const Packet &packet) override;

    TCPReceiver(int maxSeq, int windowSize);

    ~TCPReceiver() override;
};

#endif

