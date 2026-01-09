#include "analyzer.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>  
#include <cctype>

using namespace std;

bool TripAnalyzer::parseHour(const string& dt, int& hour) {
    size_t sep = dt.find(' ');
    if (sep == string::npos) sep = dt.find('T');
    if (sep == string::npos) return false;

    size_t i = sep + 1;
    while (i < dt.size() && (dt[i] == ' ' || dt[i] == '"')) i++;

    int h = 0;
    int digits = 0;
    while (i < dt.size() && digits < 2 && isdigit((unsigned char)dt[i])) {
        h = h * 10 + (dt[i] - '0');
        i++;
        digits++;
    }

    if (digits == 0) return false;
    if (i >= dt.size() || dt[i] != ':') return false;
    if (h < 0 || h > 23) return false;

    hour = h;
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

void TripAnalyzer::ingestFile(const string& csvPath) {
    zoneCounts.clear();
    slotCounts.clear();

    ifstream file(csvPath);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back(); 
        string col[6];
        if (!split6(line, col)) continue;

        if (col[0] == "TripID" || col[1] == "PickupZoneID" || col[3] == "PickupDateTime") continue;

        const string& pickupZone = col[1];
        const string& pickupDT   = col[3];

        if (pickupZone.empty()) continue;

        int hour = -1;
        if (!parseHour(pickupDT, hour)) continue;

        zoneCounts[pickupZone]++;
        slotCounts[pickupZone + "|" + to_string(hour)]++;
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







