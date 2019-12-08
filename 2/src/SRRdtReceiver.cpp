#include "SRRdtReceiver.h"
#include "Global.h"
#include "color.h"

SRRdtReceiver::SRRdtReceiver(int maxSeq, int windowSize)
    : packets(vector<pair<Packet, bool>>(maxSeq, make_pair(Packet(), false))),
      maxSeq(maxSeq),
      windowSize(windowSize) {}

SRRdtReceiver::~SRRdtReceiver() = default;

bool SRRdtReceiver::inWindow(int seqNum) {
    seqNum %= maxSeq;
    auto end = (base + windowSize - 1) % maxSeq;
    return base < end ? (base <= seqNum && seqNum <= end) : (base <= seqNum || seqNum <= end);
}

bool SRRdtReceiver::isCorrupted(const struct Packet &packet) {
    return pUtils->calculateCheckSum(packet) != packet.checksum; // check if checksums match
}

Message SRRdtReceiver::extract(const struct Packet &packet) {
    Message message;
    copy(begin(packet.payload), end(packet.payload), begin(message.data)); // copy data in packet to message
    return message;
}

void SRRdtReceiver::deliverData(const Message &message) {
    pns->delivertoAppLayer(RECEIVER, message); // send message to application layer
}

Packet SRRdtReceiver::makePacket(int ackNum) {
    Packet packet;
    packet.ackNum = ackNum;
    packet.seqNum = -1; // seqNum is unnecessary
    fill(begin(packet.payload), end(packet.payload), '.');
    packet.checksum = pUtils->calculateCheckSum(packet);
    return packet;
}

void SRRdtReceiver::udtSend(const struct Packet &packet) {
    pns->sendToNetworkLayer(SENDER, packet); // send packet to sender via network layer
}

void SRRdtReceiver::printLog(const string &message, const struct Packet &packet) {
    pUtils->printPacket(message.c_str(), packet);
}

string SRRdtReceiver::generateLog(const string &event, const string &color, bool sending) {
    auto arrow = sending ? " >>> " : " <<< ";
    return string() + GREEN + "Receiver" + RESET_COLOR + arrow + color + event + RESET_COLOR + arrow + BLUE + "Sender" + RESET_COLOR;
}

void SRRdtReceiver::receive(const struct Packet &packet) {
    auto corrupted = isCorrupted(packet);
    if (!corrupted) { // packet not corrupted
        auto seqNum = packet.seqNum;
        if (inWindow(seqNum)) { // base <= seqNum < base + windowSize
            printLog(generateLog("data", CYAN, false), packet);
            auto ackPacket = makePacket(seqNum); // make packet
            printLog(generateLog("ack", YELLOW, true), ackPacket);
            udtSend(ackPacket); // send packet to network layer
            if (!packets[seqNum].second) { // packet has not been cached
                packets[seqNum] = make_pair(packet, true); // store it
            }
            if (seqNum == base) { // deliver received packets
                while (packets[base].second) { // packet has been cached
                    auto data = extract(packets[base].first); // extract message
                    deliverData(data); // deliver to application layer
                    packets[base].second = false; // reset status
                    base = (base + 1) % maxSeq; // slide window
                }
            }
        } else if (inWindow(seqNum + windowSize)) { // base - windowSize <= seqNum < base
            printLog(generateLog("prevData", BRIGHT_CYAN, false), packet);
            auto ackPacket = makePacket(seqNum); // make ack packet
            printLog(generateLog("prevAck", BRIGHT_YELLOW, true), ackPacket);
            udtSend(ackPacket); // send packet to network layer
        }
    } else { // packet is corrupted
        printLog(generateLog("corrupted", RED, false), packet);
    }
}
