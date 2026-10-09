#include "encoding-util.hpp"

namespace cv {

void to_json(nlohmann::json &j, const Point &p) {
  j = nlohmann::json{
      {"x", p.x},
      {"y", p.y},
  };
}

void from_json(const nlohmann::json &j, Point &p) {
  j.at("x").get_to(p.x);
  j.at("y").get_to(p.y);
}

void to_json(nlohmann::json &j, const Size &s) {
  j = nlohmann::json{
      {"width", s.width},
      {"height", s.height},
  };
}

void from_json(const nlohmann::json &j, Size &s) {
  j.at("width").get_to(s.width);
  j.at("height").get_to(s.height);
}

} // namespace cv
