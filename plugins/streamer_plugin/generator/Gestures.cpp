#include "sg/generator/Generator.h"

#include "../Util.h"

namespace ospray {
  namespace sg {

  struct Gestures : public Generator
  {
    Gestures();
    ~Gestures() override = default;

    void generateData() override;

  private:
    std::vector<vec3f> positions;
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

    positions.resize(streamer_plugin::K4ABT_JOINT_COUNT);
    for (int i = 0; i < streamer_plugin::K4ABT_JOINT_COUNT; i++) {
      positions[i] = vec3f(0, 0, 0);
    }

    auto &spheres = joints.createChild("spheres", "geometry_spheres");
    spheres.createChildData("sphere.position", positions);
    spheres.child("radius") = radius;
    const std::vector<uint32_t> mID = {0};
    spheres.createChildData("material", mID); // This is a scenegraph parameter
    spheres.child("material").setSGOnly();
  }

  } // namespace sg
} // namespace ospray
