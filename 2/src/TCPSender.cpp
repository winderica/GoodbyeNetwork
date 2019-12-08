#include "TCPSender.h"
#include "Global.h"
#include "color.h"

TCPSender::TCPSender(int maxSeq, int windowSize)
    : maxSeq(maxSeq),
      windowSize(windowSize),
      duplicates(vector<int>(maxSeq, 0)),
      packets(maxSeq) {}

TCPSender::~TCPSender() = default;

bool TCPSender::getWaitingState() {
    return !inWindow(nextSeqNum);
}

bool TCPSender::inWindow(int seqNum) {
    seqNum %= maxSeq;
    auto end = (base + windowSize - 1) % maxSeq;
    return base < end ? (base <= seqNum && seqNum <= end) : (base <= seqNum || seqNum <= end);
}

bool TCPSender::isCorrupted(const struct Packet &packet) {
    return pUtils->calculateCheckSum(packet) != packet.checksum;  // check if checksums match
}

Packet TCPSender::makePacket(int seqNum, const Message &message) {
    Packet packet;
    packet.ackNum = -1; // ackNum is unnecessary
    packet.seqNum = seqNum;
    copy(begin(message.data), end(message.data), begin(packet.payload));
    packet.checksum = pUtils->calculateCheckSum(packet);
    packets[seqNum] = make_pair(packet, false);
    duplicates[seqNum] = 0;
    return packet;
}

void TCPSender::udtSend(const struct Packet &packet) {
    pns->sendToNetworkLayer(RECEIVER, packet); // send packet to receiver via network layer
}

void TCPSender::startTimer(int seqNum) {
    pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);
}

void TCPSender::stopTimer(int seqNum) {
    pns->stopTimer(SENDER, seqNum);
}

void TCPSender::printLog(const string &message, const struct Packet &packet) {
    pUtils->printPacket(message.c_str(), packet);
}

string TCPSender::generateLog(const string &event, const string &color, bool sending) {
    auto arrow = sending ? " >>> " : " <<< ";
    return string() + BLUE + "Sender" + RESET_COLOR + arrow + color + event + RESET_COLOR + arrow + GREEN + "Receiver" + RESET_COLOR;
}

bool TCPSender::send(const Message &message) {
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

void TCPSender::receive(const Packet &packet) {
    bool corrupted = isCorrupted(packet);
    if (!corrupted) { // packet not corrupted
        auto ackNum = packet.ackNum;
        if (inWindow(ackNum)) {
            printLog(generateLog("ack", YELLOW, false), packet);
            for (auto i = base; i != ackNum; i = (i + 1) % maxSeq) {
                packets[i].second = true; // mark as ACKed
            }
            packets[ackNum].second = true;
            stopTimer(base);
            base = (ackNum + 1) % maxSeq; // slide window
            for (auto i = base; i != nextSeqNum; i = (i + 1) % maxSeq) {
                if (!packets[i].second) { // there are any not ACKed packets
                    startTimer(i);
                    break;
                }
            }
        } else {
            printLog(generateLog("prevAck", BRIGHT_YELLOW, false), packet);
            duplicates[ackNum]++;
            if (duplicates[ackNum] == 3) { // fast retransmission
                stopTimer(ackNum);
                auto prevPacket = packets[(ackNum + 1) % maxSeq].first;
                printLog(generateLog("prevData", BRIGHT_CYAN, true), prevPacket);
                udtSend(prevPacket); // retransmit
                startTimer(ackNum);
                duplicates[ackNum] = 0; // reset status
            }
        }
    }
}

void TCPSender::timeoutHandler(int seqNum) {
    cout << generateLog("timeout", RED, false) << endl;
    stopTimer(seqNum);
    for (auto i = base; i != nextSeqNum; i = (i + 1) % maxSeq) {
        auto packet = packets[i];
        if (!packet.second) {
            printLog(generateLog("prevData", BRIGHT_CYAN, true), packet.first);
            udtSend(packet.first); // retransmit not ACKed packet with smallest seqNum
            startTimer(i);
            break;
        }
    }
}
