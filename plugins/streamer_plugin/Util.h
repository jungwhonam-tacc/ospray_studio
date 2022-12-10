#pragma once

#include <string>

namespace ospray {
namespace streamer_plugin {
    /** Model fitting joint definition
     */
    typedef enum
    {
        K4ABT_JOINT_PELVIS = 0,
        K4ABT_JOINT_SPINE_NAVEL,
        K4ABT_JOINT_SPINE_CHEST,
        K4ABT_JOINT_NECK,
        K4ABT_JOINT_CLAVICLE_LEFT,
        K4ABT_JOINT_SHOULDER_LEFT,
        K4ABT_JOINT_ELBOW_LEFT,
        K4ABT_JOINT_WRIST_LEFT,
        K4ABT_JOINT_HAND_LEFT,
        K4ABT_JOINT_HANDTIP_LEFT,
        K4ABT_JOINT_THUMB_LEFT,
        K4ABT_JOINT_CLAVICLE_RIGHT,
        K4ABT_JOINT_SHOULDER_RIGHT,
        K4ABT_JOINT_ELBOW_RIGHT,
        K4ABT_JOINT_WRIST_RIGHT,
        K4ABT_JOINT_HAND_RIGHT,
        K4ABT_JOINT_HANDTIP_RIGHT,
        K4ABT_JOINT_THUMB_RIGHT,
        K4ABT_JOINT_HIP_LEFT,
        K4ABT_JOINT_KNEE_LEFT,
        K4ABT_JOINT_ANKLE_LEFT,
        K4ABT_JOINT_FOOT_LEFT,
        K4ABT_JOINT_HIP_RIGHT,
        K4ABT_JOINT_KNEE_RIGHT,
        K4ABT_JOINT_ANKLE_RIGHT,
        K4ABT_JOINT_FOOT_RIGHT,
        K4ABT_JOINT_HEAD,
        K4ABT_JOINT_NOSE,
        K4ABT_JOINT_EYE_LEFT,
        K4ABT_JOINT_EAR_LEFT,
        K4ABT_JOINT_EYE_RIGHT,
        K4ABT_JOINT_EAR_RIGHT,
        K4ABT_JOINT_COUNT
    } k4abt_joint_id_t;

    const std::string k4abt_joint_id_t_str[] = {
        "PELVIS",
        "SPINE_NAVEL",
        "SPINE_CHEST",
        "NECK",
        "CLAVICLE_LEFT",
        "SHOULDER_LEFT",
        "ELBOW_LEFT",
        "WRIST_LEFT",
        "HAND_LEFT",
        "HANDTIP_LEFT",
        "THUMB_LEFT",
        "CLAVICLE_RIGHT",
        "SHOULDER_RIGHT",
        "ELBOW_RIGHT",
        "WRIST_RIGHT",
        "HAND_RIGHT",
        "HANDTIP_RIGHT",
        "THUMB_RIGHT",
        "HIP_LEFT",
        "KNEE_LEFT",
        "ANKLE_LEFT",
        "FOOT_LEFT",
        "HIP_RIGHT",
        "KNEE_RIGHT",
        "ANKLE_RIGHT",
        "FOOT_RIGHT",
        "HEAD",
        "NOSE",
        "EYE_LEFT",
        "EAR_LEFT",
        "EYE_RIGHT",
        "EAR_RIGHT"
    };

} // namespace streamer_plugin
} // namespace ospray