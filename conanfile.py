from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake


class BaseGlfwRecipe(ConanFile):
    name = "base_glfw"
    version = "1.0"

    settings = "os", "compiler", "build_type", "arch"

    generators = (
        "CMakeDeps",
        "CMakeToolchain",
    )

    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = "build/generators"

    def requirements(self):
        self.requires("glfw/3.4")
        self.requires("glm/1.0.1")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
