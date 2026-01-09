#include "analyzer.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

bool TripAnalyzer::parseHour(const string& dt, int& hour) {
    if (dt.size() < 16) return false;   

    char h1 = dt[11], h2 = dt[12];
    if (h1 < '0' || h1 > '9') return false;
    if (h2 < '0' || h2 > '9') return false;

    hour = (h1 - '0') * 10 + (h2 - '0');
    if (hour < 0 || hour > 23) return false;

    return true;
}

bool TripAnalyzer::split6(const string& line, string out[6]) {
    int i = 0;
    size_t start = 0;

    while (i < 5) {
        size_t comma = line.find(',', start);
        if (comma == string::npos) return false;
        out[i++] = line.substr(start, comma - start);
        start = comma + 1;
    }
    out[i] = line.substr(start);
    return true;
}

void TripAnalyzer::ingestFile(const std::string& csvPath) {
        ifstream file(csvPath);
    if (!file.is_open()) {

        return;
    }

    string line;

    while (getline(file, line)) {
        string col[6];
        if (!split6(line, col)) {
            continue; 
        }

        const string& pickupZone = col[1];
        const string& pickupDT = col[3];

        if (pickupZone.empty()) continue;

        int hour = -1;
        if (!parseHour(pickupDT, hour)) {
            continue;
        }

        zoneCounts[pickupZone]++;

        string key = pickupZone + "|" + to_string(hour);
        slotCounts[key]++;
    }
}


vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> v;
    v.reserve(zoneCounts.size());

    for (const auto& p : zoneCounts) {
        v.push_back(ZoneCount{p.first, p.second});
    }

    sort(v.begin(), v.end(),
        [](const ZoneCount& a, const ZoneCount& b) {
            if (a.count != b.count) return a.count > b.count; 
            return a.zone < b.zone;                           
        });
    
    size_t take = min(static_cast<size_t>(k), v.size());
    return vector<ZoneCount>(v.begin(), v.begin() + take); 
    
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    vector<SlotCount> v;
    v.reserve(slotCounts.size());

    for (const auto& p : slotCounts) {
        const string& key = p.first;
        long long count = p.second;

        size_t bar = key.find('|');
        if (bar == string::npos) continue;

        string zone = key.substr(0, bar);

        int hour = -1;
        try {
            hour = stoi(key.substr(bar + 1));
        } catch (...) {
            continue;
        }

        v.push_back(SlotCount{ zone, hour, count });
    }

    sort(v.begin(), v.end(), [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    });

    size_t take = min((size_t)k, v.size());
    return vector<SlotCount>(v.begin(), v.begin() + take);
}

