#include "analyzer.h"

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace std;

 void trimInPlace(string& s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) a++;
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    s = s.substr(a, b - a);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }
}

static bool split3(const string& line, string out[3]) {
    size_t p0 = line.find(',');
    if (p0 == string::npos) return false;
    size_t p1 = line.find(',', p0 + 1);
    if (p1 == string::npos) return false;

    out[0] = line.substr(0, p0);
    out[1] = line.substr(p0 + 1, p1 - (p0 + 1));
    out[2] = line.substr(p1 + 1);

    trimInPlace(out[0]);
    trimInPlace(out[1]);
    trimInPlace(out[2]);
    return true;
}

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

    for (int j = 0; j < 6; ++j) trimInPlace(out[j]);
    return true;
}

void TripAnalyzer::ingestFile(const string& csvPath) {
    zoneCounts.clear();
    slotCounts.clear();

    ifstream file(csvPath, ios::binary);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        int commaCount = 0;
        for (char ch : line) if (ch == ',') commaCount++;

        string zone, dt;

        if (commaCount == 2) {
            string c3[3];
            if (!split3(line, c3)) continue;

            if (c3[0] == "TripID" || c3[1] == "PickupZoneID") continue;

            zone = c3[1];
            dt   = c3[2];
        } else if (commaCount >= 5) {
            string c6[6];
            if (!split6(line, c6)) continue;

            if (c6[0] == "TripID" || c6[1] == "PickupZoneID") continue;

            zone = c6[1];
            dt   = c6[3];
        } else {
            continue;
        }

        if (zone.empty() || dt.empty()) continue;

        int hour = -1;
        if (!parseHour(dt, hour)) continue;

        zoneCounts[zone]++;
        slotCounts[zone + "|" + to_string(hour)]++;
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












