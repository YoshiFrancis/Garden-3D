#ifndef GARDEN_UTILS_HPP
#define GARDEN_UTILS_HPP

#include <vector>
#include <glm/glm.hpp>

namespace garden_3d {

    struct FlowerData {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> colors;
    };

    /*
     *
     *  The structure of a line in the file will be <x,y,z> <r, g, b>
     *  Each line describe a sing cube making up the flower.
     *
     *
     */
    FlowerData parse_flower_file(const std::string& filename);
    
}

#endif
