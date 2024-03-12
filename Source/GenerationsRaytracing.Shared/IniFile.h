#pragma once

#include <string>
#include <unordered_map>

class IniFile
{
protected:
    struct Hasher
    {
        size_t operator()(std::size_t value) const noexcept
        {
            return value;
        }
    };

    struct Property
    {
        std::string name;
        std::string value;
    };

    struct Section
    {
        std::string name;
        std::unordered_map<size_t, Property, Hasher> properties;
    };

    std::unordered_map<size_t, Section, Hasher> m_sections;

    static size_t hashStr(const std::string_view& str);

    static bool isWhitespace(char value);
    static bool isNewLine(char value);

public:
    bool read(const char* filePath);
    bool write(const char* filePath);

    std::string getString(const std::string_view& sectionName, const std::string_view& propertyName, std::string defaultValue) const;

    bool getBool(const std::string_view& sectionName, const std::string_view& propertyName, bool defaultValue) const;

    template<typename T>
    T get(const std::string_view& sectionName, const std::string_view& propertyName, T defaultValue) const;

    template<typename T>
    void enumerate(const std::string_view& sectionName, const T& function) const;

    void setString(const std::string_view& sectionName, const std::string_view& propertyName, std::string value);

    void setBool(const std::string_view& sectionName, const std::string_view& propertyName, bool value);

    template<typename T>
    void set(const std::string_view& sectionName, const std::string_view& propertyName, T value);
};

#include <IniFile.inl>
