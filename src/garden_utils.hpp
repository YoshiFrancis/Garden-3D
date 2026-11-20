#ifndef GARDEN_UTILS_HPP
#define GARDEN_UTILS_HPP

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

namespace garden_3d {

static const std::string FLOWER_DIR = "flowers/";

struct FlowerData {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> colors;
};

/*
 *
 *  The structure of a line in the file will be x,y,z|r,g,b
 *  Each line describe a sing cube making up the flower.
 *
 *
 */
std::optional<FlowerData> parse_flower_file(const std::string &filename);

} // namespace garden_3d

#endif
