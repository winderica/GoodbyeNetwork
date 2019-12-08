#include "SRRdtSender.h"
#include "Global.h"
#include "color.h"

SRRdtSender::SRRdtSender(int maxSeq, int windowSize) : packets(maxSeq), maxSeq(maxSeq), windowSize(windowSize) {}

SRRdtSender::~SRRdtSender() = default;

bool SRRdtSender::getWaitingState() {
    return !inWindow(nextSeqNum);
}

bool SRRdtSender::inWindow(int seqNum) {
    seqNum %= maxSeq;
    auto end = (base + windowSize - 1) % maxSeq;
    return base < end ? (base <= seqNum && seqNum <= end) : (base <= seqNum || seqNum <= end);
}

bool SRRdtSender::isCorrupted(const struct Packet &packet) {
    return pUtils->calculateCheckSum(packet) != packet.checksum; // check if checksums match
}

Packet SRRdtSender::makePacket(int seqNum, const Message &message) {
    Packet packet;
    packet.ackNum = -1; // ackNum is unnecessary
    packet.seqNum = seqNum;
    copy(begin(message.data), end(message.data), begin(packet.payload));
    packet.checksum = pUtils->calculateCheckSum(packet);
    packets[seqNum] = make_pair(packet, false);
    return packet;
}

void SRRdtSender::udtSend(const struct Packet &packet) {
    pns->sendToNetworkLayer(RECEIVER, packet); // send packet to receiver via network layer
}

void SRRdtSender::startTimer(int seqNum) {
    pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);
}

void SRRdtSender::stopTimer(int seqNum) {
    pns->stopTimer(SENDER, seqNum);
}

void SRRdtSender::printLog(const string &message, const struct Packet &packet) {
    pUtils->printPacket(message.c_str(), packet);
}

string SRRdtSender::generateLog(const string &event, const string &color, bool sending) {
    auto arrow = sending ? " >>> " : " <<< ";
    return string() + BLUE + "Sender" + RESET_COLOR + arrow + color + event + RESET_COLOR + arrow + GREEN + "Receiver" + RESET_COLOR;
}

bool SRRdtSender::send(const Message &message) {
    if (getWaitingState()) {
        return false;
    }
    auto packet = makePacket(nextSeqNum, message); // make packet
    printLog(generateLog("data", CYAN, true), packet);
    udtSend(packet); // send packet to network layer
    startTimer(nextSeqNum);
    nextSeqNum = (nextSeqNum + 1) % maxSeq;
    return true;
}

void SRRdtSender::receive(const Packet &packet) {
    auto corrupted = isCorrupted(packet);
    if (!corrupted) { // packet not corrupted
        auto ackNum = packet.ackNum;
        if (inWindow(ackNum)) { // base <= ackNum < base + windowSize
            printLog(generateLog("ack", YELLOW, false), packet);
            packets[ackNum].second = true; // mark it as ACKed
            stopTimer(ackNum);
            if (ackNum == base) {
                while (packets[base].second) { // packet has been ACKed
                    packets[base].second = false; // reset status
                    base = (base + 1) % maxSeq; // slide window
                }
            }
        } else {
            printLog(generateLog("prevAck", BRIGHT_YELLOW, false), packet);
        }
    }
}

void SRRdtSender::timeoutHandler(int seqNum) {
    auto packet = packets[seqNum].first;
    printLog(generateLog("timeout", RED, false), packet);
    stopTimer(seqNum);
    printLog(generateLog("prevData", BRIGHT_CYAN, true), packet);
    udtSend(packet); // resend packet
    startTimer(seqNum);
}
