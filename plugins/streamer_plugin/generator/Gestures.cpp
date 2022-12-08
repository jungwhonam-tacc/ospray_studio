#include "sg/generator/Generator.h"

namespace ospray {
  namespace sg {

  struct Gestures : public Generator
  {
    Gestures();
    ~Gestures() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(Gestures, generator_gestures);

  // definitions ////////////////////////////////////////////////

  Gestures::Gestures()
  {
    auto &parameters = child("parameters");
    parameters.createChild("radius", "float", .1f);
    parameters.child("radius").setMinMax(.01f, 2.00f);

    auto &joints = createChild("joints", "transform");
  }

  void Gestures::generateData()
  {
    auto &parameters = child("parameters");
    auto radius = parameters["radius"].valueAs<float>();

    auto &joints = child("joints");

    for (int i = 0; i < 3; i++) {
      auto &sphere = joints.createChild("joint" + std::to_string(i), "geometry_spheres");
      sphere.createChildData("sphere.position", vec3f(i, 0, 0));
      sphere.child("radius") = radius;
      sphere.createChild("color", "rgba", vec4f(1, 1, 0, 1));
      sphere.child("color").setSGOnly();
      sphere.createChild("material", "uint32_t", (uint32_t)0);
      sphere.child("material").setSGOnly();
    }
  }

  } // namespace sg
} // namespace ospray
