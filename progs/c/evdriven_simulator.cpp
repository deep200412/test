//Simple event driven simulator

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <cstdint>
using namespace std;

enum class PinDir : uint8_t { INPUT, OUTPUT };
enum class InstKind : uint8_t { COMB, FLOP, PORT_DRV, UNKNOWN };
enum class LogicValue : uint8_t { ZERO = 0, ONE = 1, X = 2 };

static inline const char* toString(LogicValue v) {
    switch (v) {
        case LogicValue::ZERO: return "0";
        case LogicValue::ONE:  return "1";
        default:               return "X";
    }
}

struct Port {
    string name;
    PinDir dir;
    int netId = -1;
};

struct PinRef {
    int id = -1;
    string name;
    PinDir dir = PinDir::INPUT;
    int instId = -1;
    int netId = -1;
};

struct Instance {
    int id = -1;
    string name;
    string cellType;
    InstKind kind = InstKind::UNKNOWN;
    vector<int> inputPins;
    vector<int> outputPins;
};

struct Net {
    int id = -1;
    string name;
    vector<int> drivers;
    vector<int> loads;
    vector<int> fanoutGates;
    LogicValue value = LogicValue::X;
};

class DesignDB {
public:
    vector<Instance> instances;
    vector<PinRef> pins;
    vector<Net> nets;

    unordered_map<string, Port> ports;
    unordered_map<string, int> instNameToId;
    unordered_map<string, int> netNameToId;

    vector<uint8_t> gateScheduled;
    queue<int> activeGates;

    int createNet(const string& netName) {
        int id = (int)nets.size();
        nets.push_back(Net{id, netName});
        netNameToId[netName] = id;
        return id;
    }

    int createInstance(const string& instName, const string& cellType, InstKind kind) {
        int id = (int)instances.size();
        instances.push_back(Instance{id, instName, cellType, kind});
        instNameToId[instName] = id;
        return id;
    }

    int addPin(int instId, const string& pinName, PinDir dir) {
        int id = (int)pins.size();
        pins.push_back(PinRef{id, pinName, dir, instId, -1});
        if (dir == PinDir::INPUT) instances[instId].inputPins.push_back(id);
        else instances[instId].outputPins.push_back(id);
        return id;
    }

    void connectPinToNet(int pinId, int netId) {
        pins[pinId].netId = netId;
        if (pins[pinId].dir == PinDir::OUTPUT) nets[netId].drivers.push_back(pinId);
        else nets[netId].loads.push_back(pinId);
    }

    void addPort(const string& portName, PinDir dir, int netId) {
        ports[portName] = Port{portName, dir, netId};
    }

    void finalizeConnectivity() {
        for (auto& net : nets) {
            net.fanoutGates.clear();
            for (int loadPin : net.loads) {
                int instId = pins[loadPin].instId;
                if (instances[instId].kind == InstKind::COMB) {
                    net.fanoutGates.push_back(instId);
                }
            }
        }
        gateScheduled.assign(instances.size(), 0);
    }

    void resetNetValues(LogicValue init = LogicValue::X) {
        for (auto& net : nets) net.value = init;
    }

    static inline LogicValue inv(LogicValue a) {
        if (a == LogicValue::ZERO) return LogicValue::ONE;
        if (a == LogicValue::ONE)  return LogicValue::ZERO;
        return LogicValue::X;
    }

    static inline LogicValue evalAND(const vector<LogicValue>& in) {
        bool hasX = false;
        for (auto v : in) {
            if (v == LogicValue::ZERO) return LogicValue::ZERO;
            if (v == LogicValue::X) hasX = true;
        }
        return hasX ? LogicValue::X : LogicValue::ONE;
    }

    static inline LogicValue evalOR(const vector<LogicValue>& in) {
        bool hasX = false;
        for (auto v : in) {
            if (v == LogicValue::ONE) return LogicValue::ONE;
            if (v == LogicValue::X) hasX = true;
        }
        return hasX ? LogicValue::X : LogicValue::ZERO;
    }

    static inline LogicValue evalXOR(const vector<LogicValue>& in) {
        for (auto v : in) {
            if (v == LogicValue::X) return LogicValue::X;
        }
        int ones = 0;
        for (auto v : in) {
            if (v == LogicValue::ONE) ones++;
        }
        return (ones & 1) ? LogicValue::ONE : LogicValue::ZERO;
    }

    LogicValue evalGateOutput(int instId) const {
        const Instance& inst = instances[instId];
        vector<LogicValue> in;
        in.reserve(inst.inputPins.size());

        for (int pinId : inst.inputPins) {
            int netId = pins[pinId].netId;
            in.push_back(netId >= 0 ? nets[netId].value : LogicValue::X);
        }

        const string& t = inst.cellType;

        if (t == "BUF" || t.find("BUF") != string::npos) return in.empty() ? LogicValue::X : in[0];
        if (t == "NOT" || t.find("INV") != string::npos) return in.empty() ? LogicValue::X : inv(in[0]);
        if (t.find("AND") != string::npos && t.find("NAND") == string::npos) return evalAND(in);
        if (t.find("NAND") != string::npos) return inv(evalAND(in));
        if (t.find("OR") != string::npos && t.find("NOR") == string::npos) return evalOR(in);
        if (t.find("NOR") != string::npos) return inv(evalOR(in));
        if (t.find("XOR") != string::npos && t.find("XNOR") == string::npos) return evalXOR(in);
        if (t.find("XNOR") != string::npos) return inv(evalXOR(in));

        return LogicValue::X;
    }

    inline void scheduleGate(int instId) {
        if (!gateScheduled[instId]) {
            gateScheduled[instId] = 1;
            activeGates.push(instId);
        }
    }

    inline void scheduleFanoutOfNet(int netId) {
        for (int instId : nets[netId].fanoutGates) {
            scheduleGate(instId);
        }
    }

    bool assignNetIfChanged(int netId, LogicValue newVal) {
        if (nets[netId].value == newVal) return false;
        nets[netId].value = newVal;
        scheduleFanoutOfNet(netId);
        return true;
    }

    void applyStimulus(
        const unordered_map<string, LogicValue>& primaryInputs,
        const unordered_map<string, LogicValue>& flopOutputs
    ) {
        while (!activeGates.empty()) activeGates.pop();
        fill(gateScheduled.begin(), gateScheduled.end(), 0);

        for (const auto& [portName, val] : primaryInputs) {
            auto it = ports.find(portName);
            if (it != ports.end() && it->second.dir == PinDir::INPUT) {
                assignNetIfChanged(it->second.netId, val);
            }
        }

        for (const auto& [netName, val] : flopOutputs) {
            auto it = netNameToId.find(netName);
            if (it != netNameToId.end()) {
                assignNetIfChanged(it->second, val);
            }
        }
    }

    void runEventDriven() {
        while (!activeGates.empty()) {
            int instId = activeGates.front();
            activeGates.pop();
            gateScheduled[instId] = 0;

            const Instance& inst = instances[instId];
            if (inst.kind != InstKind::COMB || inst.outputPins.empty()) continue;

            LogicValue outVal = evalGateOutput(instId);

            for (int outPin : inst.outputPins) {
                int outNet = pins[outPin].netId;
                if (outNet >= 0) {
                    assignNetIfChanged(outNet, outVal);
                }
            }
        }
    }

    void dumpNetValues() const {
        for (const auto& net : nets) {
            cout << net.name << " = " << toString(net.value) << "\n";
        }
    }
};

int main() {
    DesignDB db;

    int net_a   = db.createNet("a");
    int net_b   = db.createNet("b");
    int net_ffq = db.createNet("ff_q");
    int net_n1  = db.createNet("n1");
    int net_y   = db.createNet("y");

    db.addPort("a", PinDir::INPUT, net_a);
    db.addPort("b", PinDir::INPUT, net_b);
    db.addPort("y", PinDir::OUTPUT, net_y);

    int and1 = db.createInstance("u_and1", "AND2", InstKind::COMB);
    int or1  = db.createInstance("u_or1", "OR2", InstKind::COMB);

    int and1_A = db.addPin(and1, "A", PinDir::INPUT);
    int and1_B = db.addPin(and1, "B", PinDir::INPUT);
    int and1_Y = db.addPin(and1, "Y", PinDir::OUTPUT);

    int or1_A  = db.addPin(or1, "A", PinDir::INPUT);
    int or1_B  = db.addPin(or1, "B", PinDir::INPUT);
    int or1_Y  = db.addPin(or1, "Y", PinDir::OUTPUT);

    db.connectPinToNet(and1_A, net_a);
    db.connectPinToNet(and1_B, net_b);
    db.connectPinToNet(and1_Y, net_n1);

    db.connectPinToNet(or1_A, net_n1);
    db.connectPinToNet(or1_B, net_ffq);
    db.connectPinToNet(or1_Y, net_y);

    db.finalizeConnectivity();
    db.resetNetValues();

    unordered_map<string, LogicValue> pi = {
        {"a", LogicValue::ONE},
        {"b", LogicValue::ZERO}
    };

    unordered_map<string, LogicValue> flopQ = {
        {"ff_q", LogicValue::ONE}
    };

    db.applyStimulus(pi, flopQ);
    db.runEventDriven();
    db.dumpNetValues();

    return 0;
}
