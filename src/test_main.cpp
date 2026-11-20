#include "test_main.hpp"
#include "garden_utils.hpp"

#include <iostream>

bool test_parse_flower_file() {

    if (garden_3d::parse_flower_file("flower_1.txt") == std::nullopt) {
        return false;
    }

    if (garden_3d::parse_flower_file("flower_2.txt") == std::nullopt) {
        return false;
    }

    if (garden_3d::parse_flower_file("flower_3.txt") == std::nullopt) {
        return false;
    }

    if (garden_3d::parse_flower_file("bad_flower.txt") != std::nullopt) {
        return false;
    }

    if (garden_3d::parse_flower_file("file_not_found_flower.txt") != std::nullopt) {
        return false;
    }

    return true;
}

int garden_3d::test_main() {

    if (test_parse_flower_file()) {
        std::cout << "passing <test_parse_flower_file>\n";
    } else {
        std::cout << "failing <test_parse_flower_file>\n";
    }

    return 0;
}
