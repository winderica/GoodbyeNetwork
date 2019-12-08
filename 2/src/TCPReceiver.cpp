#include "TCPReceiver.h"
#include "Global.h"
#include "color.h"

TCPReceiver::TCPReceiver(int maxSeq, int windowSize)
    : packets(vector<pair<Packet, bool>>(maxSeq, make_pair(Packet(), false))),
      maxSeq(maxSeq),
      windowSize(windowSize),
      lastAckPacket(makePacket(-1)) {}

TCPReceiver::~TCPReceiver() = default;

bool TCPReceiver::isCorrupted(const struct Packet &packet) {
    return pUtils->calculateCheckSum(packet) != packet.checksum;
}

bool TCPReceiver::inWindow(int seqNum) {
    seqNum %= maxSeq;
    auto end = (base + windowSize - 1) % maxSeq;
    return base < end ? (base <= seqNum && seqNum <= end) : (base <= seqNum || seqNum <= end);
}

Message TCPReceiver::extract(const struct Packet &packet) {
    Message message;
    copy(begin(packet.payload), end(packet.payload), begin(message.data)); // copy data in packet to message
    return message;
}

void TCPReceiver::deliverData(const Message &message) {
    pns->delivertoAppLayer(RECEIVER, message); // send message to application layer
}

Packet TCPReceiver::makePacket(int ackNum) {
    Packet packet;
    packet.ackNum = ackNum;
    packet.seqNum = -1; // seqNum is unnecessary
    fill(begin(packet.payload), end(packet.payload), '.');
    packet.checksum = pUtils->calculateCheckSum(packet);
    return packet;
}

void TCPReceiver::udtSend(const struct Packet &packet) {
    pns->sendToNetworkLayer(SENDER, packet); // send packet to sender via network layer
}

void TCPReceiver::printLog(const string &message, const struct Packet &packet) {
    pUtils->printPacket(message.c_str(), packet);
}

string TCPReceiver::generateLog(const string &event, const string &color, bool sending) {
    auto arrow = sending ? " >>> " : " <<< ";
    return string() + GREEN + "Receiver" + RESET_COLOR + arrow + color + event + RESET_COLOR + arrow + BLUE + "Sender" + RESET_COLOR;
}

void TCPReceiver::receive(const struct Packet &packet) {
    bool corrupted = isCorrupted(packet);
    if (!corrupted) { // packet not corrupted and sequence number is expected
        auto seqNum = packet.seqNum;
        if (inWindow(seqNum)) { // base <= seqNum < base + windowSize
            if (!packets[seqNum].second) { // packet has not been cached
                packets[seqNum] = make_pair(packet, true); // store it
            }
            if (seqNum == base) { // deliver received packets
                printLog(generateLog("data", CYAN, false), packet);
                auto prevBase = base;
                while (packets[base].second) { // packet has been cached
                    auto data = extract(packets[base].first); // extract message
                    deliverData(data); // deliver to application layer
                    packets[base].second = false; // reset status
                    prevBase = base;
                    base = (base + 1) % maxSeq; // slide window
                }
                lastAckPacket = makePacket(prevBase); // make packet
                printLog(generateLog("ack", YELLOW, true), lastAckPacket);
                udtSend(lastAckPacket); // send packet to network layer
            } else {
                printLog(generateLog("prevData", BRIGHT_CYAN, false), packet);
                printLog(generateLog("prevAck", BRIGHT_YELLOW, true), lastAckPacket);
                udtSend(lastAckPacket); // send packet to network layer
            }
        }
    } else { // packet is corrupted
        printLog(generateLog("corrupted", RED, false), packet);
        printLog(generateLog("prevAck", BRIGHT_YELLOW, true), lastAckPacket);
        udtSend(lastAckPacket); // send previous packet
    }
}