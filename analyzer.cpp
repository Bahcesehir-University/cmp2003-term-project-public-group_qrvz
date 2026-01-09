#include "analyzer.h"
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <vector>

static inline void trimInPlace(string& s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) a++;
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    s = s.substr(a, b - a);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }
}

bool TripAnalyzer::parseHour(const string& dt, int& hour) {
    size_t colon = dt.find(':');
    if (colon == string::npos) return false;

    size_t j = colon;
    while (j > 0 && (dt[j - 1] == '"' || isspace((unsigned char)dt[j - 1]))) j--;

    size_t start = j;
    int digits = 0;
    while (start > 0 && digits < 2 && isdigit((unsigned char)dt[start - 1])) {
        start--;
        digits++;
    }
    if (digits == 0) return false;

    int h = 0;
    for (size_t t = start; t < j; ++t) {
        if (!isdigit((unsigned char)dt[t])) return false;
        h = h * 10 + (dt[t] - '0');
    }

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

void TripAnalyzer::ingestFile(const std::string& csvPath) {
    zoneCounts.clear();
    slotCounts.clear();

    ifstream file(csvPath);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        string col[6];
        if (!split6(line, col)) continue;

        for (int i = 0; i < 6; ++i) trimInPlace(col[i]);

        if (col[0] == "TripID") continue;

        const string& pickupZone = col[1];
        const string& pickupDT = col[3];

        if (pickupZone.empty() || pickupDT.empty()) continue;

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
        v.push_back(ZoneCount{ p.first, p.second });
    }

    sort(v.begin(), v.end(), [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    });

    size_t take = (k <= 0) ? 0 : min((size_t)k, v.size());
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

    size_t take = (k <= 0) ? 0 : min((size_t)k, v.size());
    return vector<SlotCount>(v.begin(), v.begin() + take);
}










