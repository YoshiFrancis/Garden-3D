#ifndef GRAPHICS_GARDEN_HPP
#define GRAPHICS_GARDEN_HPP

/*
 * Flower will hold information on itself and how to draw itself by referencing a file.
 *
 * The flower class will hold information on the L-System used to generate it. The flower 
 * class will also hold the UI needed to collect the L-System and generate the flower from such.
 *
 * Interface:
 * 1. generate(std::vector<std::string> flowers, int seed);
 * 2. draw();
 * 3. clear();
 *
 */

namespace garden_3d {

    constexpr int GARDEN_WIDTH = 40;
    constexpr int GARDEN_LENGTH = 60;
    
};

#endif
