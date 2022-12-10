#include "sg/generator/Generator.h"

#include "../Util.h"

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
    parameters.createChild("radius", "float", .025f);
    parameters.child("radius").setMinMax(.01f, 2.00f);

    auto &joints = createChild("joints", "transform");
  }

  void Gestures::generateData()
  {
    auto &parameters = child("parameters");
    auto radius = parameters["radius"].valueAs<float>();

    auto &joints = child("joints");

    for (int i = 0; i < streamer_plugin::K4ABT_JOINT_COUNT; i++) {
      auto &sphere = joints.createChild(streamer_plugin::k4abt_joint_id_t_str[i], "geometry_spheres");
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
