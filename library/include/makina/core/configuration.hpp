#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>

namespace mak
{
class configuration
{
public:
  explicit configuration  (std::filesystem::path filepath = "config.json")
  : filepath_(std::move(filepath))
  {
    load();
  }
  configuration           (const configuration&  that) = delete ;
  configuration           (      configuration&& temp) = default;
  virtual ~configuration  ()
  {
    save();
  }
  configuration& operator=(const configuration&  that) = delete ;
  configuration& operator=(      configuration&& temp) = default;

  template <typename type>
  type                         get     (const std::string& key, const type default_value = type()) const
  {
    if (const auto iterator = json_.find(key); iterator != json_.cend())
      return iterator->get<type>();

    json_[key] = default_value;
    return default_value;
  }
  template <typename type>
  void                         set     (const std::string& key, const type value)
  {
    json_[key] = value;
  }

  [[nodiscard]]
  const std::filesystem::path& filepath() const
  {
    return filepath_;
  }
  [[nodiscard]]
  const nlohmann::json&        json    () const
  {
    return json_;
  }
  
protected:
  void                         load    ()
  {
    if (!exists(filepath_))
      return;

    std::ifstream stream(filepath_);
    stream >> json_;
  }
  void                         save    () const
  {
    std::ofstream stream(filepath_);
    stream << std::setw(2) << json_ << std::endl;
  }

  std::filesystem::path filepath_;
  nlohmann::json        json_    ;
};
}