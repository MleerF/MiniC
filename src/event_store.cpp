#include "event_store.hpp"
#include <fstream>

void EventStore::load(const std::string& path) {
    path_ = path;
    data_.clear();
    std::ifstream f(path);
    if (!f) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;

        // Formato: YYYY-MM-DD|HH:MM|Texto
        auto p1 = line.find('|');
        if (p1 == std::string::npos) continue;
        auto p2 = line.find('|', p1 + 1);
        if (p2 == std::string::npos) continue;

        std::string date = line.substr(0, p1);
        std::string time = line.substr(p1 + 1, p2 - p1 - 1);
        std::string text = line.substr(p2 + 1);

        if (date.size() != 10) continue;

        data_[date].push_back({time, text});
    }
}

void EventStore::save(const std::string& path) const {
    std::ofstream f(path, std::ios::trunc);
    if (!f) return;

    f << "# MinCalendar events\n";
    for (const auto& [date, events] : data_) {
        for (const auto& ev : events) {
            f << date << '|' << ev.time << '|' << ev.text << '\n';
        }
    }
}

std::vector<Event>& EventStore::forDate(const Date& d) {
    return data_[d.iso()];
}

const std::vector<Event>* EventStore::find(const Date& d) const {
    auto it = data_.find(d.iso());
    return (it == data_.end()) ? nullptr : &it->second;
}