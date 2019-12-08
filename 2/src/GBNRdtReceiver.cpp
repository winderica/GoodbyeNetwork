#include "GBNRdtReceiver.h"
#include "Global.h"
#include "color.h"

GBNRdtReceiver::GBNRdtReceiver(int maxSeq, int windowSize) : maxSeq(maxSeq), windowSize(windowSize), lastAckPacket(makePacket(-1)) {}

GBNRdtReceiver::~GBNRdtReceiver() = default;

bool GBNRdtReceiver::isCorrupted(const struct Packet &packet) {
    return pUtils->calculateCheckSum(packet) != packet.checksum;
}

bool GBNRdtReceiver::hasSeqNum(const struct Packet &packet) {
    return expectedSeqNum == packet.seqNum;
}

Message GBNRdtReceiver::extract(const struct Packet &packet) {
    Message message;
    copy(begin(packet.payload), end(packet.payload), begin(message.data)); // copy data in packet to message
    return message;
}

void GBNRdtReceiver::deliverData(const Message &message) {
    pns->delivertoAppLayer(RECEIVER, message); // send message to application layer
}

Packet GBNRdtReceiver::makePacket(int ackNum) {
    Packet packet;
    packet.ackNum = ackNum;
    packet.seqNum = -1; // seqNum is unnecessary
    fill(begin(packet.payload), end(packet.payload), '.');
    packet.checksum = pUtils->calculateCheckSum(packet);
    return packet;
}

void GBNRdtReceiver::udtSend(const struct Packet &packet) {
    pns->sendToNetworkLayer(SENDER, packet); // send packet to sender via network layer
}

void GBNRdtReceiver::printLog(const string &message, const struct Packet &packet) {
    pUtils->printPacket(message.c_str(), packet);
}

string GBNRdtReceiver::generateLog(const string &event, const string &color, bool sending) {
    auto arrow = sending ? " >>> " : " <<< ";
    return string() + GREEN + "Receiver" + RESET_COLOR + arrow + color + event + RESET_COLOR + arrow + BLUE + "Sender" + RESET_COLOR;
}

void GBNRdtReceiver::receive(const struct Packet &packet) {
    bool corrupted = isCorrupted(packet);
    if (!corrupted && hasSeqNum(packet)) { // packet not corrupted and sequence number is expected
        printLog(generateLog("data", CYAN, false), packet);
        auto data = extract(packet); // extract message
        deliverData(data); // deliver to application layer
        lastAckPacket = makePacket(expectedSeqNum); // make packet
        printLog(generateLog("ack", YELLOW, true), lastAckPacket);
        udtSend(lastAckPacket); // send packet to network layer
        expectedSeqNum = (expectedSeqNum + 1) % maxSeq; // increase sequence number
    } else { // packet is corrupted or unexpected
        if (corrupted) {
            printLog(generateLog("corrupted", RED, false), packet);
        } else {
            printLog(generateLog("unexpected", RED, false), packet);
        }
        printLog(generateLog("prevAck", BRIGHT_YELLOW, true), lastAckPacket);
        udtSend(lastAckPacket); // send previous packet
    }
}