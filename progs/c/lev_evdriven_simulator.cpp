//levelized event driven simulation

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <cstdint>
#include <algorithm>

using namespace std;

enum class PinDir : uint8_t { INPUT, OUTPUT };
enum class InstKind : uint8_t { COMB, FLOP, PORT_DRV, UNKNOWN };
enum class LogicValue : uint8_t { ZERO = 0, ONE = 1, X = 2 };
enum class GateOp : uint8_t { BUF, NOT1, AND, NAND, OR, NOR, XOR, XNOR, UNKNOWN };

static inline const char* lvToStr(LogicValue v) {
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

struct Gate {
    int id = -1;
    string name;
    GateOp op = GateOp::UNKNOWN;
    InstKind kind = InstKind::UNKNOWN;
    vector<int> inNets;
    vector<int> outNets;
    int level = 0;
};

struct Net {
    int id = -1;
    string name;
    LogicValue value = LogicValue::X;
    vector<int> fanoutGates;
    int driverGate = -1;
};

class LevelizedEventSim {
public:
    vector<Gate> gates;
    vector<Net> nets;
    unordered_map<string, Port> ports;
    unordered_map<string, int> netNameToId;

    vector<vector<int>> activeByLevel;
    vector<uint8_t> scheduled;
    int maxLevel = 0;
    int minDirtyLevel = -1;
    int maxDirtyLevel = -1;

    int createNet(const string& netName) {
        int id = (int)nets.size();
        nets.push_back(Net{id, netName});
        netNameToId[netName] = id;
        return id;
    }

    int createGate(const string& name, GateOp op, InstKind kind,
                   const vector<int>& inNets,
                   const vector<int>& outNets) {
        int id = (int)gates.size();
        gates.push_back(Gate{id, name, op, kind, inNets, outNets});
        for (int n : outNets) nets[n].driverGate = id;
        return id;
    }

    void addPort(const string& portName, PinDir dir, int netId) {
        ports[portName] = Port{portName, dir, netId};
    }

    void finalizeConnectivityAndLevels() {
        for (auto& net : nets) net.fanoutGates.clear();

        for (const auto& g : gates) {
            if (g.kind != InstKind::COMB) continue;
            for (int inNet : g.inNets) {
                nets[inNet].fanoutGates.push_back(g.id);
            }
        }

        computeLevels();
        activeByLevel.assign(maxLevel + 1, {});
        scheduled.assign(gates.size(), 0);
    }

    void computeLevels() {
        vector<int> indegree(gates.size(), 0);
        vector<vector<int>> adj(gates.size());

        for (auto& g : gates) g.level = 0;

        for (const auto& g : gates) {
            if (g.kind != InstKind::COMB) continue;

            for (int inNet : g.inNets) {
                int drv = nets[inNet].driverGate;
                if (drv >= 0 && gates[drv].kind == InstKind::COMB) {
                    adj[drv].push_back(g.id);
                    indegree[g.id]++;
                }
            }
        }

        queue<int> q;
        for (const auto& g : gates) {
            if (g.kind == InstKind::COMB && indegree[g.id] == 0) {
                q.push(g.id);
            }
        }

        maxLevel = 0;
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            maxLevel = max(maxLevel, gates[u].level);

            for (int v : adj[u]) {
                gates[v].level = max(gates[v].level, gates[u].level + 1);
                if (--indegree[v] == 0) q.push(v);
            }
        }
    }

    void resetValues(LogicValue init = LogicValue::X) {
        for (auto& n : nets) n.value = init;
    }

    void clearActiveLists() {
        if (minDirtyLevel == -1) return;
        for (int lvl = minDirtyLevel; lvl <= maxDirtyLevel; ++lvl) {
            activeByLevel[lvl].clear();
        }
        minDirtyLevel = maxDirtyLevel = -1;
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
            if (v == LogicValue::ONE) ++ones;
        }
        return (ones & 1) ? LogicValue::ONE : LogicValue::ZERO;
    }

    LogicValue evalGate(int gateId) const {
        const Gate& g = gates[gateId];
        vector<LogicValue> inVals;
        inVals.reserve(g.inNets.size());

        for (int n : g.inNets) {
            inVals.push_back(nets[n].value);
        }

        switch (g.op) {
            case GateOp::BUF:   return inVals.empty() ? LogicValue::X : inVals[0];
            case GateOp::NOT1:  return inVals.empty() ? LogicValue::X : inv(inVals[0]);
            case GateOp::AND:   return evalAND(inVals);
            case GateOp::NAND:  return inv(evalAND(inVals));
            case GateOp::OR:    return evalOR(inVals);
            case GateOp::NOR:   return inv(evalOR(inVals));
            case GateOp::XOR:   return evalXOR(inVals);
            case GateOp::XNOR:  return inv(evalXOR(inVals));
            default:            return LogicValue::X;
        }
    }

    inline void scheduleGate(int gateId) {
        if (scheduled[gateId]) return;
        scheduled[gateId] = 1;

        int lvl = gates[gateId].level;
        activeByLevel[lvl].push_back(gateId);

        if (minDirtyLevel == -1 || lvl < minDirtyLevel) minDirtyLevel = lvl;
        if (maxDirtyLevel == -1 || lvl > maxDirtyLevel) maxDirtyLevel = lvl;
    }

    inline void scheduleFanout(int netId) {
        for (int gateId : nets[netId].fanoutGates) {
            scheduleGate(gateId);
        }
    }

    bool assignNetIfChanged(int netId, LogicValue newVal) {
        if (nets[netId].value == newVal) return false;
        nets[netId].value = newVal;
        scheduleFanout(netId);
        return true;
    }

    void applyStimulus(const unordered_map<string, LogicValue>& primaryInputs,
                       const unordered_map<string, LogicValue>& flopOutputs) {
        clearActiveLists();

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

    void runScheduledInLevelOrder() {
        if (minDirtyLevel == -1) return;

        for (int lvl = minDirtyLevel; lvl <= maxDirtyLevel; ++lvl) {
            auto& bucket = activeByLevel[lvl];

            for (int gateId : bucket) {
                scheduled[gateId] = 0;

                const Gate& g = gates[gateId];
                if (g.kind != InstKind::COMB) continue;

                LogicValue out = evalGate(gateId);
                for (int outNet : g.outNets) {
                    assignNetIfChanged(outNet, out);
                }
            }

            bucket.clear();
        }

        minDirtyLevel = maxDirtyLevel = -1;
    }

    LogicValue getNetValue(const string& netName) const {
        auto it = netNameToId.find(netName);
        if (it == netNameToId.end()) return LogicValue::X;
        return nets[it->second].value;
    }

    void printPrimaryOutputs() const {
        cout << "Primary outputs:\n";
        for (const auto& [name, port] : ports) {
            if (port.dir == PinDir::OUTPUT) {
                cout << "  " << name << " = " << lvToStr(nets[port.netId].value) << "\n";
            }
        }
    }

    void printSelectedNets(const vector<string>& names) const {
        for (const auto& n : names) {
            auto it = netNameToId.find(n);
            if (it != netNameToId.end()) {
                cout << "  " << n << " = " << lvToStr(nets[it->second].value) << "\n";
            }
        }
    }

    void printAllNets() const {
        cout << "All nets:\n";
        for (const auto& n : nets) {
            cout << "  " << n.name << " = " << lvToStr(n.value) << "\n";
        }
    }
};

int main() {
    LevelizedEventSim sim;

    // Nets
    int net_a    = sim.createNet("a");
    int net_b    = sim.createNet("b");
    int net_ff1q = sim.createNet("ff1_q");
    int net_n1   = sim.createNet("n1");
    int net_n2   = sim.createNet("n2");
    int net_y    = sim.createNet("y");
    int net_d1   = sim.createNet("ff1_d");

    // Ports
    sim.addPort("a", PinDir::INPUT, net_a);
    sim.addPort("b", PinDir::INPUT, net_b);
    sim.addPort("y", PinDir::OUTPUT, net_y);

    // Combinational logic:
    // n1   = a AND b
    // n2   = n1 OR ff1_q
    // y    = NOT n2
    // ff1_d = a XOR ff1_q

    sim.createGate("u_and1", GateOp::AND,  InstKind::COMB, {net_a, net_b},      {net_n1});
    sim.createGate("u_or1",  GateOp::OR,   InstKind::COMB, {net_n1, net_ff1q},  {net_n2});
    sim.createGate("u_inv1", GateOp::NOT1, InstKind::COMB, {net_n2},             {net_y});
    sim.createGate("u_xor1", GateOp::XOR,  InstKind::COMB, {net_a, net_ff1q},    {net_d1});

    sim.finalizeConnectivityAndLevels();
    sim.resetValues();

    vector<pair<unordered_map<string, LogicValue>, unordered_map<string, LogicValue>>> testVectors = {
        { {{"a", LogicValue::ZERO}, {"b", LogicValue::ZERO}}, {{"ff1_q", LogicValue::ZERO}} },
        { {{"a", LogicValue::ONE},  {"b", LogicValue::ZERO}}, {{"ff1_q", LogicValue::ZERO}} },
        { {{"a", LogicValue::ONE},  {"b", LogicValue::ONE}},  {{"ff1_q", LogicValue::ZERO}} },
        { {{"a", LogicValue::ONE},  {"b", LogicValue::ONE}},  {{"ff1_q", LogicValue::ONE}}  },
        { {{"a", LogicValue::ZERO}, {"b", LogicValue::ONE}},  {{"ff1_q", LogicValue::ONE}}  }
    };

    for (size_t i = 0; i < testVectors.size(); ++i) {
        cout << "\n=== Vector " << (i + 1) << " ===\n";

        const auto& piVec   = testVectors[i].first;
        const auto& flopVec = testVectors[i].second;

        cout << "Stimulus:\n";
        for (const auto& [k, v] : piVec) {
            cout << "  PI  " << k << " = " << lvToStr(v) << "\n";
        }
        for (const auto& [k, v] : flopVec) {
            cout << "  FFQ " << k << " = " << lvToStr(v) << "\n";
        }

        sim.applyStimulus(piVec, flopVec);
        sim.runScheduledInLevelOrder();

        sim.printPrimaryOutputs();

        cout << "Selected internal / sequential boundary nets:\n";
        sim.printSelectedNets({"n1", "n2", "ff1_d", "ff1_q"});
    }

    cout << "\nFinal complete net dump:\n";
    sim.printAllNets();

    return 0;
}
