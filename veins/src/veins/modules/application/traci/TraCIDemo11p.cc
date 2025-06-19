#include "veins/modules/application/traci/TraCIDemo11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"

using namespace veins;

Define_Module(veins::TraCIDemo11p);

void TraCIDemo11p::initialize(int stage) {
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
    }
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa) {
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame) {
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);
    findHost()->getDisplayString().setTagArg("i", 1, "green");

    std::string edgeToAvoid = wsm->getDemoData();

    if (mobility->getRoadId()[0] != ':') {
        EV << "⚠️ Vehículo evitando vía: " << edgeToAvoid << endl;
        traciVehicle->changeRoute(edgeToAvoid.c_str(), 9999);
    }

    if (!sentMessage) {
        sentMessage = true;
        wsm->setSenderAddress(myId);
        wsm->setSerial(3);
        scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm->dup());
    }
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg) {
    if (TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg)) {
        sendDown(wsm->dup());
        wsm->setSerial(wsm->getSerial() + 1);
        if (wsm->getSerial() >= 3) {
            stopService();
            delete wsm;
        } else {
            scheduleAt(simTime() + 1, wsm);
        }
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj) {
    DemoBaseApplLayer::handlePositionUpdate(obj);

    if (mobility->getSpeed() < 1) {
        if (simTime() - lastDroveAt >= 10 && !sentMessage) {
            Coord pos = mobility->getPositionAt(simTime());

            EV << "🚨 Vehículo detenido por posible colisión. Tiempo: " << simTime()
               << ", Posición: (" << pos.x << ", " << pos.y << "), Vía: " << mobility->getRoadId() << endl;

            traciVehicle->setColor(TraCIColor(255, 0, 0, 255));

            findHost()->getDisplayString().setTagArg("t", 0, "✖ COLISIÓN");
            findHost()->getDisplayString().setTagArg("t", 1, "red");

            findHost()->getDisplayString().setTagArg("i", 1, "red");

            sentMessage = true;

            TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
            populateWSM(wsm);
            wsm->setDemoData(mobility->getRoadId().c_str());

            if (dataOnSch) {
                startService(Channel::sch2, 42, "Traffic Info Service");
                scheduleAt(computeAsynchronousSendingTime(1, ChannelType::service), wsm);
            } else {
                sendDown(wsm);
            }
        }
    } else {
        lastDroveAt = simTime();
        findHost()->getDisplayString().setTagArg("t", 0, "");
        findHost()->getDisplayString().setTagArg("i", 1, "green");
        traciVehicle->setColor(TraCIColor(0, 255, 0, 255));
    }
}

