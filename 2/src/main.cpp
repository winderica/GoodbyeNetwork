#include "Global.h"
#include "StopWaitRdtSender.h"
#include "StopWaitRdtReceiver.h"
#include "GBNRdtSender.h"
#include "GBNRdtReceiver.h"
#include "SRRdtSender.h"
#include "SRRdtReceiver.h"
#include "TCPSender.h"
#include "TCPReceiver.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <Type>" << endl
             << "Type:" << endl
             << "\t0: Stop-wait" << endl
             << "\t1: GBN" << endl
             << "\t2: SR" << endl
             << "\t3: TCP" << endl;
        return 1;
    }
    NetworkServiceConfiguration::PROB_OF_PACKET_LOSSED = 0.1;
    NetworkServiceConfiguration::PROB_OF_PACKET_CORRUPTED = 0.1;
    string type = argv[1];
    RdtSender *ps;
    RdtReceiver *pr;
    if (type == "0") {
        ps = new StopWaitRdtSender();
        pr = new StopWaitRdtReceiver();
    } else if (type == "1") {
        ps = new GBNRdtSender(8, 4);
        pr = new GBNRdtReceiver(8, 4);
    } else if (type == "2") {
        ps = new SRRdtSender(8, 4);
        pr = new SRRdtReceiver(8, 4);
    } else if (type == "3") {
        ps = new TCPSender(8, 4);
        pr = new TCPReceiver(8, 4);
    } else {
        return 1;
    }
    pns->setRunMode(0); // verbose mode
    pns->init();
    pns->setRtdSender(ps);
    pns->setRtdReceiver(pr);
    pns->setInputFile("../test/input.txt");
    pns->setOutputFile("../test/output.txt");
    pns->start();

    delete pUtils; // free instance of Tool
    delete pns; // free instance of NetworkService
    delete ps; // free instance of RdtSender
    delete pr; // free instance of RdtReceiver

    return 0;
}

