#include "sg/generator/Generator.h"

namespace ospray {
  namespace sg {

  struct Axes : public Generator
  {
    Axes();
    ~Axes() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(Axes, generator_axes);

  // definitions ////////////////////////////////////////////////

  Axes::Axes()
  {
    auto &parameters = child("parameters");
    parameters.createChild("Visibility", "bool", true);
    parameters.createChild("Thickness", "float", 0.010f);
    parameters["Thickness"].setMinMax(0.005f, 0.100f);
    parameters.createChild("Length", "float", 1.0f);
    parameters["Length"].setMinMax(0.10f, 10.0f);

    auto &axes = createChild("axes", "transform");
  }

  void Axes::generateData()
  {
    auto &parameters = child("parameters");
    auto visible = parameters["Visibility"].valueAs<bool>();
    auto b = parameters["Thickness"].valueAs<float>();
    auto l = parameters["Length"].valueAs<float>();
    auto &axes = child("axes");

    {
      box3f boxes = {box3f({vec3f(0, -b, -b), vec3f(l, b, b)})};
      auto &box = axes.createChild("x-axis", "geometry_boxes");
      box.createChildData("box", boxes);
      box.createChild("color", "rgba", vec4f(1, 0, 0, 1));
      box.child("color").setSGOnly();
      box.createChild("material", "uint32_t", (uint32_t)0);
      box.child("material").setSGOnly();
      box.child("visible").setValue(visible);
    }

    {
      box3f boxes = {box3f({vec3f(-b, 0, -b), vec3f(b, l, b)})};
      auto &box = axes.createChild("y-axis", "geometry_boxes");
      box.createChildData("box", boxes);
      box.createChild("color", "rgba", vec4f(0, 1, 0, 1));
      box.child("color").setSGOnly();
      box.createChild("material", "uint32_t", (uint32_t)0);
      box.child("material").setSGOnly();
      box.child("visible").setValue(visible);
    }

    {
      box3f boxes = {box3f({vec3f(-b, -b, 0), vec3f(b, b, l)})};
      auto &box = axes.createChild("z-axis", "geometry_boxes");
      box.createChildData("box", boxes);
      box.createChild("color", "rgba", vec4f(0, 0, 1, 1));
      box.child("color").setSGOnly();
      box.createChild("material", "uint32_t", (uint32_t)0);
      box.child("material").setSGOnly();
      box.child("visible").setValue(visible);
    }
  }

  } // namespace sg
} // namespace ospray