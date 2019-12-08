#include "GBNRdtSender.h"
#include "Global.h"
#include "color.h"

GBNRdtSender::GBNRdtSender(int maxSeq, int windowSize) : maxSeq(maxSeq), windowSize(windowSize), packets(vector<Packet>(maxSeq)) {}

GBNRdtSender::~GBNRdtSender() = default;

bool GBNRdtSender::getWaitingState() {
    return !inWindow(nextSeqNum);
}

bool GBNRdtSender::inWindow(int seqNum) {
    seqNum %= maxSeq;
    auto end = (base + windowSize - 1) % maxSeq;
    return base < end ? (base <= seqNum && seqNum <= end) : (base <= seqNum || seqNum <= end);
}

bool GBNRdtSender::isCorrupted(const struct Packet &packet) {
    return pUtils->calculateCheckSum(packet) != packet.checksum;  // check if checksums match
}

Packet GBNRdtSender::makePacket(int seqNum, const Message &message) {
    Packet packet;
    packet.ackNum = -1; // ackNum is unnecessary
    packet.seqNum = seqNum;
    copy(begin(message.data), end(message.data), begin(packet.payload));
    packet.checksum = pUtils->calculateCheckSum(packet);
    packets[seqNum] = packet;
    return packet;
}

void GBNRdtSender::udtSend(const struct Packet &packet) {
    pns->sendToNetworkLayer(RECEIVER, packet); // send packet to receiver via network layer
}

void GBNRdtSender::startTimer(int seqNum) {
    pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);
}

void GBNRdtSender::stopTimer(int seqNum) {
    pns->stopTimer(SENDER, seqNum);
}

void GBNRdtSender::printLog(const string &message, const struct Packet &packet) {
    pUtils->printPacket(message.c_str(), packet);
}

string GBNRdtSender::generateLog(const string &event, const string &color, bool sending) {
    auto arrow = sending ? " >>> " : " <<< ";
    return string() + BLUE + "Sender" + RESET_COLOR + arrow + color + event + RESET_COLOR + arrow + GREEN + "Receiver" + RESET_COLOR;
}

bool GBNRdtSender::send(const Message &message) {
    if (getWaitingState()) {
        return false;
    }
    auto packet = makePacket(nextSeqNum, message);
    printLog(generateLog("data", CYAN, true), packet);
    udtSend(packet);
    if (base == nextSeqNum) {
        startTimer(base);
    }
    nextSeqNum = (nextSeqNum + 1) % maxSeq;
    return true;
}

void GBNRdtSender::receive(const Packet &packet) {
    bool corrupted = isCorrupted(packet);
    if (!corrupted) { // packet not corrupted
        auto prevBase = base;
        base = (packet.ackNum + 1) % maxSeq; // slide window
        if (base == nextSeqNum) {
            printLog(generateLog("ack", YELLOW, false), packet);
            stopTimer(prevBase);
        } else {
            printLog(generateLog("prevAck", BRIGHT_YELLOW, false), packet);
            stopTimer(prevBase);
            startTimer(base);
        }
    }
}

void GBNRdtSender::timeoutHandler(int seqNum) {
    cout << generateLog("timeout", RED, false) << endl;
    stopTimer(seqNum);
    for (auto i = base; i != nextSeqNum; i = (i + 1) % maxSeq) {
        auto packet = packets[i];
        printLog(generateLog("prevData", BRIGHT_CYAN, true), packet);
        udtSend(packet); // retransmit all packets that has been sent
    }
    startTimer(seqNum);
}
