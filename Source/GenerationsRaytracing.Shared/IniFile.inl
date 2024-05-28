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
    const auto data = std::make_unique<char[]>(dataSize + 1);
    data[dataSize] = '\0';

    fseek(file, 0, SEEK_SET);
    fread(data.get(), 1, dataSize, file);

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
        else if (!isWhitespace(*dataPtr) && !isNewLine(*dataPtr))
        {
            if (section == nullptr)
                return false;

            const char* endPtr;
            if (*dataPtr == '"')
            {
                ++dataPtr;
                endPtr = dataPtr;

                while (*endPtr != '\0' && !isNewLine(*endPtr) && *endPtr != '"')
                    ++endPtr;

                if (*endPtr != '"')
                    return false;
            }
            else
            {
                endPtr = dataPtr;

                while (*endPtr != '\0' && !isNewLine(*endPtr) && !isWhitespace(*endPtr) && *endPtr != '=')
                    ++endPtr;

                if (!isNewLine(*endPtr) && !isWhitespace(*endPtr) && *endPtr != '=')
                    return false;
            }

            std::string propertyName(dataPtr, endPtr - dataPtr);
            auto& property = section->properties[hashStr(propertyName)];
            property.name = std::move(propertyName);

            dataPtr = endPtr;
            while (*dataPtr != '\0' && !isNewLine(*dataPtr) && *dataPtr != '=')
                ++dataPtr;

            if (*dataPtr == '=')
            {
                ++dataPtr;

                while (*dataPtr != '\0' && isWhitespace(*dataPtr))
                    ++dataPtr;

                if (*dataPtr == '"')
                {
                    ++dataPtr;
                    endPtr = dataPtr;

                    while (*endPtr != '\0' && !isNewLine(*endPtr) && *endPtr != '"')
                        ++endPtr;

                    if (*endPtr != '"')
                        return false;
                }
                else
                {
                    endPtr = dataPtr;

                    while (*endPtr != '\0' && !isNewLine(*endPtr) && !isWhitespace(*endPtr))
                        ++endPtr;
                }

                property.value = std::string(dataPtr, endPtr - dataPtr);
                dataPtr = endPtr + 1;
            }
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

        fputc('[', file);
        fputs(section.name.c_str(), file);
        fputs("]\n", file);

        for (auto& [_, property] : section.properties)
        {
            const bool quoteName = property.name.find_first_of(" \t") != std::string::npos;
            const bool quoteValue = property.value.find_first_of(" \t") != std::string::npos;

            if (quoteName) fputc('"', file);
            fputs(property.name.c_str(), file);
            if (quoteName) fputc('"', file);

            fputc('=', file);

            if (quoteValue) fputc('"', file);
            fputs(property.value.c_str(), file);
            if (quoteValue) fputc('"', file);

            fputc('\n', file);
        }
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
        if (propertyPair != sectionPair->second.properties.end() && !propertyPair->second.value.empty())
        {
            const char firstChar = propertyPair->second.value[0];
            return firstChar == 't' || firstChar == 'T' || firstChar == 'y' || firstChar == 'Y' || firstChar == '1';
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

template<typename T>
inline void IniFile::enumerate(const T& function) const
{
    for (const auto& [_, section] : m_sections)
    {
        for (auto& property : section.properties)
            function(section.name, property.second.name, property.second.value);
    }
}

template <typename T>
void IniFile::enumerate(const std::string_view& sectionName, const T& function) const
{
    const auto sectionPair = m_sections.find(hashStr(sectionName));
    if (sectionPair != m_sections.end())
    {
        for (const auto& property : sectionPair->second.properties)
            function(property.second.name, property.second.value);
    }
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
