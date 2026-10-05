#pragma once

#include <map>
#include <string>
#include <vector>

class ModelCache
{
  public:
    ModelCache();

    const std::vector<float> &LoadOrCreate(const std::string &name, std::vector<float> generatedModel);

  private:
    void Load();
    void Save() const;

    std::map<std::string, std::vector<float>> m_Models;
    bool m_IsDirty = false;
};
