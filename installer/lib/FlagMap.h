#ifndef FLAGMAP_H
#define FLAGMAP_H
#include <functional>
#include <string>
#include <unordered_map>

class FlagMap {
public:
    [[nodiscard]] std::string getFlag(const std::string& flag) const
    {
        auto it = flags.find(flag);
        if (it != flags.end()) {
            return it->second;
        }
        return "";
    }

    void setFlag(const std::string& flag, const std::string& value)
    {
        flags[flag] = value;
    }

    void forEach(const std::function<void(const std::string&, const std::string&)>& func) const
    {
        for (const auto& [flag, value] : flags) {
            func(flag, value);
        }
    }

    void clearAll()
    {
        flags.clear();
    }

    [[nodiscard]] size_t getFlagCount() const
    {
        return flags.size();
    }


private:
    std::unordered_map<std::string, std::string> flags;
};
#endif //FLAGMAP_H