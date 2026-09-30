#pragma once
#include "date.hpp"
#include <string>
#include <vector>
#include <unordered_map>

struct Event {
    std::string time;   // "HH:MM" o "--:--"
    std::string text;
};

class EventStore {
public:
    void load(const std::string& path);
    void save(const std::string& path) const;

    std::vector<Event>& forDate(const Date& d);
    const std::vector<Event>* find(const Date& d) const;

private:
    std::unordered_map<std::string, std::vector<Event>> data_;
    std::string path_;
};