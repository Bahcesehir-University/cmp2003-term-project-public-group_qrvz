#pragma once
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

struct ZoneCount {
    string zone;
    long long count;
};

struct SlotCount {
    string zone;
    int hour;           
    long long count;
};

class TripAnalyzer {
public:
    void ingestFile(const string& csvPath);

    vector<ZoneCount> topZones(int k = 10) const;
    vector<SlotCount> topBusySlots(int k = 10) const;

    unordered_map<string, long long> zoneCounts;
    unordered_map<string, long long> slotCounts;

    bool parseHour(const string& dt, int& hour);
    bool split6(const string& line, string out[6]);
};

