#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <chrono>

#define HIGH 0x1
#define LOW  0x0
#define LED_BUILTIN 2

class String : public std::string {
public:
    String() : std::string() {}
    String(const char* s) : std::string(s ? s : "") {}
    String(const std::string& s) : std::string(s) {}
    String(int val) : std::string(std::to_string(val)) {}
    String(unsigned int val) : std::string(std::to_string(val)) {}
    String(long val) : std::string(std::to_string(val)) {}
    String(unsigned long val) : std::string(std::to_string(val)) {}
    String(float val, int decimals = 2) {
        std::ostringstream ss;
        ss.precision(decimals);
        ss << std::fixed << val;
        *this = ss.str();
    }
    String(double val, int decimals = 2) {
        std::ostringstream ss;
        ss.precision(decimals);
        ss << std::fixed << val;
        *this = ss.str();
    }

    size_t length() const {
        return size();
    }

    size_t write(uint8_t c) {
        push_back(static_cast<char>(c));
        return 1;
    }

    size_t write(const uint8_t* buffer, size_t size) {
        if (!buffer || size == 0) return 0;
        append(reinterpret_cast<const char*>(buffer), size);
        return size;
    }

    size_t write(const char* s, size_t n) {
        if (!s || n == 0) return 0;
        append(s, n);
        return n;
    }

    int toInt() const {
        try {
            return std::stoi(*this);
        } catch (...) {
            return 0;
        }
    }

    float toFloat() const {
        try {
            return std::stof(*this);
        } catch (...) {
            return 0.0f;
        }
    }

    double toDouble() const {
        try {
            return std::stod(*this);
        } catch (...) {
            return 0.0;
        }
    }

    String substring(size_t from, size_t to = std::string::npos) const {
        if (from >= size()) return String("");
        if (to == std::string::npos) return String(substr(from));
        if (to <= from) return String("");
        return String(substr(from, to - from));
    }

    int indexOf(char c, size_t from = 0) const {
        auto pos = find(c, from);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }

    int indexOf(const String& s, size_t from = 0) const {
        auto pos = find(s, from);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }

    bool equals(const String& s) const {
        return *this == s;
    }

    bool equalsIgnoreCase(const String& s) const {
        if (size() != s.size()) return false;
        for (size_t i = 0; i < size(); ++i) {
            if (tolower((*this)[i]) != tolower(s[i])) return false;
        }
        return true;
    }

    bool startsWith(const String& prefix) const {
        if (prefix.size() > size()) return false;
        return substr(0, prefix.size()) == prefix;
    }

    bool endsWith(const String& suffix) const {
        if (suffix.size() > size()) return false;
        return substr(size() - suffix.size()) == suffix;
    }
};

inline uint32_t millis() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

inline void delay(uint32_t ms) {
}

inline void pinMode(uint8_t pin, uint8_t mode) {}
inline void digitalWrite(uint8_t pin, uint8_t val) {}
inline int digitalRead(uint8_t pin) { return LOW; }
inline int analogRead(uint8_t pin) { return 512; }

#endif
