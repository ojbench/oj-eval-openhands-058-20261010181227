#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// Basic exception class
class BasicException {
protected:
    std::string message;
public:
    explicit BasicException(const std::string& _message) : message(_message) {}
    virtual const char* what() const noexcept { return message.c_str(); }
};

class ArgumentException : public BasicException {
public:
    using BasicException::BasicException;
};

class IteratorException : public BasicException {
public:
    using BasicException::BasicException;
};

struct Pokemon {
    std::string name;
    int id;
    std::vector<std::string> types; // each type string
};

class Pokedex {
private:
    std::string filename;
    // map ordered by id (ascending)
    std::map<int, Pokemon> data;

    // Helper: trim whitespace
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

    // Helper: split by delimiter
    static std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> res;
        std::string token;
        std::istringstream iss(s);
        while (std::getline(iss, token, delim)) {
            res.push_back(token);
        }
        return res;
    }

    // Validate name (only letters, length <=10)
    static void validate_name(const std::string& name) {
        if (name.empty() || name.size() > 10) {
            throw ArgumentException("Argument Error: PM Name Invalid (" + name + ")");
        }
        for (char c : name) {
            if (!std::isalpha(static_cast<unsigned char>(c))) {
                throw ArgumentException("Argument Error: PM Name Invalid (" + name + ")");
            }
        }
    }

    // Validate types string and return vector
    static std::vector<std::string> parse_and_validate_types(const std::string& types_str) {
        if (types_str.empty()) {
            throw ArgumentException("Argument Error: PM Type Invalid ()");
        }
        std::vector<std::string> parts = split(types_str, '#');
        if (parts.size() < 1 || parts.size() > 7) {
            throw ArgumentException("Argument Error: PM Type Invalid (" + types_str + ")");
        }
        std::set<std::string> uniq;
        for (const std::string& t : parts) {
            if (t.empty() || t.size() > 10) {
                throw ArgumentException("Argument Error: PM Type Invalid (" + t + ")");
            }
            for (char c : t) {
                if (!std::isalpha(static_cast<unsigned char>(c))) {
                    throw ArgumentException("Argument Error: PM Type Invalid (" + t + ")");
                }
            }
            if (!uniq.insert(t).second) {
                // duplicate type, still allowed? treat as invalid
                throw ArgumentException("Argument Error: PM Type Invalid (" + t + ")");
            }
        }
        return parts;
    }

    // Damage multiplier table for 7 types (water, fire, grass, electric, ground, rock, flying)
    static double get_multiplier(const std::string& attack, const std::string& defend) {
        static const std::map<std::string, std::map<std::string, double>> table = {
            {"water", {{"fire",2.0},{"ground",2.0},{"rock",2.0},{"grass",0.5},{"electric",0.5}}},
            {"fire", {{"grass",2.0},{"rock",0.5},{"water",0.5}}},
            {"grass", {{"water",2.0},{"rock",2.0},{"ground",2.0},{"fire",0.5},{"grass",0.5},{"flying",0.5}}},
            {"electric", {{"water",2.0},{"flying",2.0},{"ground",0.0},{"grass",0.5}}},
            {"ground", {{"electric",2.0},{"rock",2.0},{"fire",2.0},{"grass",0.5},{"flying",0.0}}},
            {"rock", {{"flying",0.5},{"fire",2.0},{"ground",2.0},{"grass",0.5}}},
            {"flying", {{"grass",2.0},{"electric",0.5},{"rock",0.5},{"ground",0.0}}}
        };
        auto itA = table.find(attack);
        if (itA == table.end()) return 1.0; // unknown attack type treated neutral
        auto itD = itA->second.find(defend);
        if (itD == itA->second.end()) return 1.0;
        return itD->second;
    }

public:
    explicit Pokedex(const char* _fileName) : filename(_fileName) {
        std::ifstream fin(filename);
        if (fin) {
            std::string line;
            while (std::getline(fin, line)) {
                line = trim(line);
                if (line.empty()) continue;
                // Expected format: name id types
                std::istringstream iss(line);
                std::string name, id_str, types_str;
                if (!(iss >> name >> id_str)) continue; // malformed line
                int id = std::stoi(id_str);
                std::getline(iss, types_str);
                types_str = trim(types_str);
                std::vector<std::string> types = split(types_str, '#');
                Pokemon p{ name, id, types };
                data[id] = p;
            }
        } else {
            // create empty file
            std::ofstream fout(filename, std::ios::app);
        }
    }

    ~Pokedex() {
        std::ofstream fout(filename, std::ios::trunc);
        for (const auto& kv : data) {
            const Pokemon& p = kv.second;
            fout << p.name << ' ' << p.id << ' ';
            for (size_t i = 0; i < p.types.size(); ++i) {
                fout << p.types[i];
                if (i + 1 < p.types.size()) fout << '#';
            }
            fout << '\n';
        }
    }

    bool pokeAdd(const char* name_c, int id, const char* types_c) {
        try {
            std::string name(name_c);
            std::string types_str(types_c);
            validate_name(name);
            if (id <= 0) throw ArgumentException("Argument Error: PM ID Invalid (" + std::to_string(id) + ")");
            if (data.count(id) || std::any_of(data.begin(), data.end(), [&](const auto& kv){return kv.second.name == name;})) {
                return false; // duplicate
            }
            std::vector<std::string> types = parse_and_validate_types(types_str);
            Pokemon p{ name, id, types };
            data[id] = p;
            return true;
        } catch (const ArgumentException& e) {
            throw; // rethrow as required
        } catch (...) {
            return false;
        }
    }

    bool pokeDel(int id) {
        auto it = data.find(id);
        if (it != data.end()) {
            data.erase(it);
            return true;
        }
        return false;
    }

    std::string pokeFind(int id) const {
        auto it = data.find(id);
        if (it != data.end()) return it->second.name;
        return "None";
    }

    std::string typeFind(const char* types_c) const {
        std::string types_str(types_c);
        std::vector<std::string> required = split(types_str, '#');
        std::vector<const Pokemon*> matches;
        for (const auto& kv : data) {
            const Pokemon& p = kv.second;
            std::set<std::string> have(p.types.begin(), p.types.end());
            bool ok = true;
            for (const std::string& r : required) {
                if (have.find(r) == have.end()) { ok = false; break; }
            }
            if (ok) matches.push_back(&p);
        }
        if (matches.empty()) return "None";
        std::sort(matches.begin(), matches.end(), [](const Pokemon* a, const Pokemon* b){return a->id < b->id;});
        std::ostringstream oss;
        oss << matches.size();
        for (const Pokemon* p : matches) {
            oss << '\n' << p->name;
        }
        return oss.str();
    }

    float attack(const char* type_c, int id) const {
        auto it = data.find(id);
        if (it == data.end()) return -1.0f;
        std::string attack_type(type_c);
        const Pokemon& p = it->second;
        double mult = 1.0;
        for (const std::string& def : p.types) {
            mult *= get_multiplier(attack_type, def);
        }
        return static_cast<float>(mult);
    }

    int catchTry() const {
        if (data.empty()) return 0;
        // owned set of ids
        std::set<int> owned;
        // start with smallest id
        int start_id = data.begin()->first;
        owned.insert(start_id);
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& kv : data) {
                int target_id = kv.first;
                if (owned.count(target_id)) continue;
                const Pokemon& target = kv.second;
                // check if any owned pokemon can capture
                bool can = false;
                for (int own_id : owned) {
                    const Pokemon& own = data.at(own_id);
                    for (const std::string& atk : own.types) {
                        double mult = 1.0;
                        for (const std::string& def : target.types) {
                            mult *= get_multiplier(atk, def);
                        }
                        if (mult >= 2.0 - 1e-9) { can = true; break; }
                    }
                    if (can) break;
                }
                if (can) {
                    owned.insert(target_id);
                    changed = true;
                }
            }
        }
        return static_cast<int>(owned.size());
    }

    // Iterator implementation
    class iterator {
    private:
        std::map<int, Pokemon>::iterator it;
    public:
        explicit iterator(const std::map<int, Pokemon>::iterator& _it) : it(_it) {}
        iterator& operator++() { ++it; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++it; return tmp; }
        iterator& operator--() { --it; return *this; }
        iterator operator--(int) { iterator tmp = *this; --it; return tmp; }
        iterator& operator=(const iterator& rhs) { it = rhs.it; return *this; }
        bool operator==(const iterator& rhs) const { return it == rhs.it; }
        bool operator!=(const iterator& rhs) const { return it != rhs.it; }
        Pokemon& operator*() const { return const_cast<Pokemon&>(it->second); }
        Pokemon* operator->() const { return const_cast<Pokemon*>(&it->second); }
    };

    iterator begin() { return iterator(data.begin()); }
    iterator end() { return iterator(data.end()); }
};
