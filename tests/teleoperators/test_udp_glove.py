import json

from lerobot.teleoperators.udp_glove.udp_glove import (
    ACTION_GLOVE_FIELDS,
    RAW_GLOVE_FIELDS,
    parse_udp_glove_packet,
)


def make_packet() -> str:
    return json.dumps(
        {
            "Udhand": {
                "Bones": [],
                "Parameter": [
                    {"Name": "L_CalibrationStatus", "Value": 3},
                    *[
                        {"Name": name, "Value": idx}
                        for idx, name in enumerate(RAW_GLOVE_FIELDS)
                    ],
                    {"Name": "l_aButton", "Value": False},
                    {"Name": "r_aButton", "Value": False},
                ],
            }
        }
    )


def test_parse_udp_glove_packet_extracts_l_and_r_fields_only():
    sample = parse_udp_glove_packet(make_packet(), receive_time_s=12.3, seq=4)

    assert sample.receive_time_s == 12.3
    assert sample.seq == 4
    assert set(sample.values) == set(ACTION_GLOVE_FIELDS)
    assert sample.values["left_thumb_third_joint_pitch"] == 0
    assert sample.values["left_imu_quaternion_z"] == 27
    assert sample.values["right_thumb_third_joint_pitch"] == 28
    assert sample.values["right_imu_quaternion_z"] == 55


def test_parse_udp_glove_packet_accepts_dynamic_role_name():
    packet = json.dumps(
        {
            "232": {
                "Bones": [],
                "Parameter": [
                    {"Name": name, "Value": idx}
                    for idx, name in enumerate(RAW_GLOVE_FIELDS)
                ],
            }
        }
    )

    sample = parse_udp_glove_packet(packet, receive_time_s=12.3, seq=5)

    assert sample.values["left_thumb_third_joint_pitch"] == 0
    assert sample.values["right_imu_quaternion_z"] == 55


def test_parse_udp_glove_packet_fills_missing_fields_with_zero():
    packet = json.dumps({"Udhand": {"Parameter": [{"Name": "l0", "Value": 1}]}})

    sample = parse_udp_glove_packet(packet, receive_time_s=12.3, seq=4)

    assert sample.values["left_thumb_third_joint_pitch"] == 1
    assert sample.values["right_imu_quaternion_z"] == 0
