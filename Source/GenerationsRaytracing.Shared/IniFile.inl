#include <charconv>
#include <xxhash.h>

inline size_t IniFile::hashStr(const std::string_view& str)
{
#ifdef _WIN64
    return XXH3_64bits(str.data(), str.size());
#else
    return XXH32(str.data(), str.size(), 0);
#endif
}

inline bool IniFile::isWhitespace(char value)
{
    return value == ' ' || value == '\t';
}

inline bool IniFile::isNewLine(char value)
{
    return value == '\n' || value == '\r';
}

inline bool IniFile::read(const char* filePath)
{
    FILE* file = fopen(filePath, "rb");
    if (!file)
        return false;

    fseek(file, 0, SEEK_END);
    const size_t dataSize = static_cast<size_t>(ftell(file));
    fseek(file, 0, SEEK_SET);

    const std::unique_ptr<char[]> data = std::make_unique<char[]>(dataSize + 1);
    fread(data.get(), 1, dataSize, file);
    *(data.get() + dataSize) = '\0';

    fclose(file);

    Section* section = nullptr;
    const char* dataPtr = data.get();

    while (dataPtr < data.get() + dataSize)
    {
        if (*dataPtr == ';')
        {
            while (*dataPtr != '\0' && !isNewLine(*dataPtr))
                ++dataPtr;
        }
        else if (*dataPtr == '[')
        {
            ++dataPtr;
            const char* endPtr = dataPtr;
            while (*endPtr != '\0' && !isNewLine(*endPtr) && *endPtr != ']')
                ++endPtr;

            if (*endPtr != ']')
                return false;

            std::string sectionName(dataPtr, endPtr - dataPtr);
            section = &m_sections[hashStr(sectionName)];
            section->name = std::move(sectionName);

            dataPtr = endPtr + 1;
        }
        else if (!isWhitespace(*dataPtr) && !isNewLine(*dataPtr) && *dataPtr != '"')
        {
            if (section == nullptr)
                return false;

            const char* endPtr = dataPtr;
            while (*endPtr != '\0' && !isWhitespace(*endPtr) && !isNewLine(*endPtr) && *endPtr != '"' && *endPtr != '=')
                ++endPtr;

            std::string propertyName(dataPtr, endPtr - dataPtr);
            auto& property = section->properties[hashStr(propertyName)];
            property.name = std::move(propertyName);

            dataPtr = endPtr;
            if (*dataPtr != '=')
            {
                while (*dataPtr != '\0' && !isNewLine(*dataPtr) && *dataPtr != '=')
                    ++dataPtr;

                if (*dataPtr != '=')
                    return false;
            }
            ++dataPtr;

            while (isWhitespace(*dataPtr) || *dataPtr == '"')
                ++dataPtr;

            endPtr = dataPtr;
            while (*endPtr != '\0' && !isWhitespace(*endPtr) && !isNewLine(*endPtr) && *endPtr != '"')
                ++endPtr;

            property.value = std::string(dataPtr, endPtr - dataPtr);

            dataPtr = endPtr + 1;
        }
        else
        {
            ++dataPtr;
        }
    }

    return true;
}

inline bool IniFile::write(const char* filePath)
{
    FILE* file = fopen(filePath, "w");
    if (!file)
        return false;

    for (auto& [_, section] : m_sections)
    {
        if (section.properties.empty())
            continue;

        fprintf(file, "[%s]\n", section.name.c_str());
        for (auto& [_, property] : section.properties)
            fprintf(file, "%s=%s\n", property.name.c_str(), property.value.c_str());
    }

    fclose(file);

    return true;
}

inline std::string IniFile::getString(const std::string_view& sectionName, const std::string_view& propertyName, std::string defaultValue) const
{
    const auto sectionPair = m_sections.find(hashStr(sectionName));
    if (sectionPair != m_sections.end())
    {
        const auto propertyPair = sectionPair->second.properties.find(hashStr(propertyName));
        if (propertyPair != sectionPair->second.properties.end())
            return propertyPair->second.value;
    }
    return defaultValue;
}

inline bool IniFile::getBool(const std::string_view& sectionName, const std::string_view& propertyName, bool defaultValue) const
{
    const auto sectionPair = m_sections.find(hashStr(sectionName));
    if (sectionPair != m_sections.end())
    {
        const auto propertyPair = sectionPair->second.properties.find(hashStr(propertyName));
        if (propertyPair != sectionPair->second.properties.end())
        {
            return !propertyPair->second.value.empty() && 
                propertyPair->second.value[0] == 't' && 
                propertyPair->second.value[0] == 'T' && 
                propertyPair->second.value[0] == '1';
        }
    }
    return defaultValue;
}

template <typename T>
T IniFile::get(const std::string_view& sectionName, const std::string_view& propertyName, T defaultValue) const
{
    const auto sectionPair = m_sections.find(hashStr(sectionName));
    if (sectionPair != m_sections.end())
    {
        const auto propertyPair = sectionPair->second.properties.find(hashStr(propertyName));
        if (propertyPair != sectionPair->second.properties.end())
        {
            T value{};
            const auto result = std::from_chars(propertyPair->second.value.data(), 
                propertyPair->second.value.data() + propertyPair->second.value.size(), value);

            if (result.ec == std::errc{})
                return value;
        }
    }
    return defaultValue;
}

inline void IniFile::setString(const std::string_view& sectionName, const std::string_view& propertyName, std::string value)
{
    auto& section = m_sections[hashStr(sectionName)];
    if (section.name.empty())
        section.name = sectionName;

    auto& property = section.properties[hashStr(propertyName)];
    if (property.name.empty())
        property.name = propertyName;

    property.value = std::move(value);
}

inline void IniFile::setBool(const std::string_view& sectionName, const std::string_view& propertyName, bool value)
{
    setString(sectionName, propertyName, value ? "true" : "false");
}

template <typename T>
void IniFile::set(const std::string_view& sectionName, const std::string_view& propertyName, T value)
{
    setString(sectionName, propertyName, std::to_string(value));
}
