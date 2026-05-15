import pytest

from lerobot.teleoperators.steamvr_tracker.steamvr_tracker import parse_steamvr_tracker_packet


PACKET = (
    "2026-04-02 17:39:32:050;LG=F;RG=F;LT=F;RT=F;"
    "LPos: (-0.004536, 1.138480, 0.407648) LRot: (12.439250, 69.856450, 299.125500);"
    "RPos: (0.159430, 1.139674, 0.369927) RRot: (12.275980, 310.135700, 61.552390);"
    "X=F;A=F;Y=F;B=F"
)


def test_parse_steamvr_tracker_packet_extracts_positions_and_times():
    sample = parse_steamvr_tracker_packet(PACKET, receive_time_s=123.4, seq=7)

    assert sample.left_pos == pytest.approx((-0.004536, 1.138480, 0.407648))
    assert sample.left_rot == pytest.approx((12.439250, 69.856450, 299.125500))
    assert sample.right_pos == pytest.approx((0.159430, 1.139674, 0.369927))
    assert sample.right_rot == pytest.approx((12.275980, 310.135700, 61.552390))
    assert sample.receive_time_s == 123.4
    assert sample.seq == 7
    assert sample.source_time_s > 0


def test_parse_steamvr_tracker_packet_accepts_non_padded_timestamp():
    packet = (
        "2026-5-11 17:30:52:34;LG=F;RG=F;LT=F;RT=F;"
        "LPos: (0.214709, 0.339443, -0.005864) LRot: (75.356390, 7.681817, -28.121210);"
        "RPos: (-0.131392, 0.369865, 0.111333) RRot: (94.895443, -2.840729, -40.594106);"
        "X=F;A=F;Y=F;B=F"
    )

    sample = parse_steamvr_tracker_packet(packet, receive_time_s=123.4, seq=8)

    assert sample.left_pos == pytest.approx((0.214709, 0.339443, -0.005864))
    assert sample.left_rot == pytest.approx((75.356390, 7.681817, -28.121210))
    assert sample.right_pos == pytest.approx((-0.131392, 0.369865, 0.111333))
    assert sample.right_rot == pytest.approx((94.895443, -2.840729, -40.594106))
    assert sample.source_time_s == pytest.approx(17 * 3600 + 30 * 60 + 52.034)


def test_parse_steamvr_tracker_packet_rejects_missing_positions():
    with pytest.raises(ValueError, match="Could not parse"):
        parse_steamvr_tracker_packet("2026-04-02 17:39:32:050;LG=F", receive_time_s=1.0, seq=1)
