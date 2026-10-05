#include "stdafx.h"
#include "ModelCache.h"

#include <Windows.h>

#include <cstdint>
#include <fstream>

namespace
{
constexpr std::uint32_t CacheMagic = 0x53474D43; // SGMC
constexpr std::uint32_t CacheVersion = 1;
const char *CacheDirectory = "Cache";
const char *CachePath = "Cache/world_models.cache";
}

ModelCache::ModelCache()
{
    Load();
}

const std::vector<float> &ModelCache::LoadOrCreate(const std::string &name, std::vector<float> generatedModel)
{
    const auto found = m_Models.find(name);
    if (found != m_Models.end())
        return found->second;
    const auto inserted = m_Models.emplace(name, std::move(generatedModel));
    m_IsDirty = true;
    Save();
    return inserted.first->second;
}

void ModelCache::Load()
{
    std::ifstream input(CachePath, std::ios::binary);
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t count = 0;
    if (!input.read(reinterpret_cast<char *>(&magic), sizeof(magic)) ||
        !input.read(reinterpret_cast<char *>(&version), sizeof(version)) ||
        !input.read(reinterpret_cast<char *>(&count), sizeof(count)) || magic != CacheMagic || version != CacheVersion)
        return;
    for (std::uint32_t index = 0; index < count; ++index)
    {
        std::uint32_t nameLength = 0;
        std::uint32_t valueCount = 0;
        if (!input.read(reinterpret_cast<char *>(&nameLength), sizeof(nameLength)))
            return;
        std::string name(nameLength, '\0');
        if (!input.read(&name[0], nameLength) || !input.read(reinterpret_cast<char *>(&valueCount), sizeof(valueCount)))
            return;
        std::vector<float> values(valueCount);
        if (!input.read(reinterpret_cast<char *>(values.data()), sizeof(float) * valueCount))
            return;
        m_Models.emplace(std::move(name), std::move(values));
    }
}

void ModelCache::Save() const
{
    if (!m_IsDirty)
        return;
    CreateDirectoryA(CacheDirectory, nullptr);
    std::ofstream output(CachePath, std::ios::binary | std::ios::trunc);
    const std::uint32_t count = static_cast<std::uint32_t>(m_Models.size());
    output.write(reinterpret_cast<const char *>(&CacheMagic), sizeof(CacheMagic));
    output.write(reinterpret_cast<const char *>(&CacheVersion), sizeof(CacheVersion));
    output.write(reinterpret_cast<const char *>(&count), sizeof(count));
    for (const auto &entry : m_Models)
    {
        const std::uint32_t nameLength = static_cast<std::uint32_t>(entry.first.size());
        const std::uint32_t valueCount = static_cast<std::uint32_t>(entry.second.size());
        output.write(reinterpret_cast<const char *>(&nameLength), sizeof(nameLength));
        output.write(entry.first.data(), nameLength);
        output.write(reinterpret_cast<const char *>(&valueCount), sizeof(valueCount));
        output.write(reinterpret_cast<const char *>(entry.second.data()), sizeof(float) * valueCount);
    }
}
