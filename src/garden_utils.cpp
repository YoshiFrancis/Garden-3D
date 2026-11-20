#include "garden_utils.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace garden_3d;

static void print_flower_data(const std::string &filename, FlowerData &data) {

  std::cout << "Printing flower data for file: " << filename << "\n";
  for (int idx = 0; idx < data.positions.size(); ++idx) {
    std::cout << " <" << data.positions[idx].x << "," << data.positions[idx].y
              << "," << data.positions[idx].z << ">";
  }
  std::cout << "\n";

  for (int idx = 0; idx < data.colors.size(); ++idx) {
    std::cout << " <" << data.colors[idx].x << "," << data.colors[idx].y << ","
              << data.colors[idx].z << ">";
  }
  std::cout << "\n";
}

std::optional<FlowerData>
garden_3d::parse_flower_file(const std::string &filename) {

  FlowerData data;

  std::ifstream file(FLOWER_DIR + filename);

  if (!file.is_open()) {
    std::cerr << "Error opening flower file: " << filename << "\n";
    return std::nullopt;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string x, y, z, r, g, b;
    std::getline(iss, x, ',');
    std::getline(iss, y, ',');
    std::getline(iss, z, '|');
    std::getline(iss, r, ',');
    std::getline(iss, g, ',');
    std::getline(iss, b);
    try {
      glm::vec3 pos = glm::vec3(std::stoi(x), std::stoi(y), std::stoi(z));
      glm::vec3 rgb = glm::vec3(std::stoi(r), std::stoi(g), std::stoi(b));
      data.positions.push_back(pos);
      data.colors.push_back(rgb);
    } catch (std::exception e) {
      std::cerr << "Error pasing file: " << e.what() << "\n";
      return std::nullopt;
    }
  }

  print_flower_data(filename, data);
  return data;
}
