"""Tests d'intégration pour le flux de données embedded-data-logger.

Valide :
- La simulation STM32 (trames UART)
- Le parsing des données côté Python
- La validation d'intégrité des trames
- Le format CSV généré
- Le flux complet de bout en bout
"""

import csv
import os
import sys
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from stm32_simulator import STM32Simulator


def parse_data_frame(frame: bytes, num_channels: int) -> dict:
    """Parse une trame #D# et retourne valeurs + checksum.

    Format: #D#<val0>#<val1>#...#<valN>#<checksum>#
    """
    decoded = frame.decode("utf-8").strip()
    parts = decoded.split("#")
    # parts = ['', 'D', 'val0', 'val1', ..., 'valN', 'checksum', '']
    assert parts[1] == "D"
    raw_values = parts[2:-1]  # toutes les chaines entre D et le dernier #
    checksum_raw = raw_values[-1]
    raw_values = raw_values[:-1]
    assert len(raw_values) == num_channels
    checksum = int(checksum_raw)
    return {"values": [int(v) for v in raw_values], "checksum": checksum}


# ====================================================================
# Tests du simulateur STM32
# ====================================================================


class TestSTM32Simulator:
    def setup_method(self):
        self.sim = STM32Simulator(num_channels=4)

    def test_generate_sync_response_format(self):
        resp = self.sim.generate_sync_response()
        decoded = resp.decode("utf-8").strip()
        assert decoded.startswith("#D#!")
        parts = decoded.split("#")
        # parts = ['', 'D', '!4', '1', ''] (ou similaire)
        sync_part = parts[2]
        assert sync_part.startswith("!")
        nb_channels = int(sync_part[1:])
        assert nb_channels == 4

    def test_generate_data_frame_format(self):
        frame = self.sim.generate_data_frame()
        decoded = frame.decode("utf-8").strip()
        assert decoded.startswith("#D#")
        assert decoded[-1] == "#"
        result = parse_data_frame(frame, 4)
        assert len(result["values"]) == 4

    def test_generate_data_frame_checksum_valid(self):
        frame = self.sim.generate_data_frame()
        result = parse_data_frame(frame, 4)
        expected = sum(len(str(v)) for v in result["values"])
        assert result["checksum"] == expected

    def test_generate_data_frame_num_values(self):
        frame = self.sim.generate_data_frame()
        result = parse_data_frame(frame, 4)
        assert len(result["values"]) == 4

    def test_process_sync_command(self):
        resp = self.sim.process_command(b"#?#\n")
        assert resp is not None
        assert b"#D#!" in resp

    def test_process_start_stream(self):
        resp = self.sim.process_command(b"#A#\n")
        assert resp is None
        assert self.sim.streaming is True

    def test_process_stop_stream(self):
        self.sim.streaming = True
        resp = self.sim.process_command(b"#S#\n")
        assert resp is not None
        assert b"#STOP#" in resp
        assert self.sim.streaming is False

    def test_process_disconnect(self):
        resp = self.sim.process_command(b"#P#\n")
        assert resp is not None
        assert b"#DISCONNECTED#" in resp

    def test_simulate_error_frame(self):
        frame = self.sim.simulate_error_frame()
        assert b"#D#" in frame
        decoded = frame.decode("utf-8").strip()
        parts = decoded.split("#")
        raw_values = parts[2:-1]
        checksum_raw = raw_values[-1]
        raw_values = raw_values[:-1]
        expected = sum(len(v) for v in raw_values)
        assert int(checksum_raw) != expected

    def test_simulate_overflow_frame(self):
        frame = self.sim.simulate_overflow_frame()
        assert b"#E#OVF#" in frame

    def test_set_luminosity_updates_frame(self):
        self.sim.set_luminosity(1500)
        r1 = parse_data_frame(self.sim.generate_data_frame(), 4)
        self.sim.set_luminosity(97)
        r2 = parse_data_frame(self.sim.generate_data_frame(), 4)
        assert r1["values"][2] != r2["values"][2]

    def test_simulate_data_stream(self):
        frames = self.sim.simulate_data_stream(num_frames=5)
        assert len(frames) == 5
        for f in frames:
            assert f.startswith(b"#D#")

    def test_simulate_full_session(self):
        sequence = self.sim.simulate_full_session(num_frames=5)
        assert len(sequence) == 7
        assert b"#D#!" in sequence[0]
        assert b"#STOP#" in sequence[-1]
        for frame in sequence[1:-1]:
            assert frame.startswith(b"#D#")

    def test_simulate_noise(self):
        frame = self.sim.simulate_noise()
        assert b"garbage" in frame


# ====================================================================
# Tests du parsing de trames UART
# ====================================================================


class TestDataFrameParsing:
    def setup_method(self):
        self.sim = STM32Simulator(num_channels=4)

    def test_parse_valid_data_frame(self):
        result = parse_data_frame(self.sim.generate_data_frame(), 4)
        assert len(result["values"]) == 4
        assert result["values"][2] == self.sim.sensor_values["luminosity"]

    def test_parse_sync_response(self):
        frame = self.sim.generate_sync_response()
        decoded = frame.decode("utf-8").strip()
        parts = decoded.split("#")
        sync_part = parts[2]
        assert sync_part[0] == "!"
        nb_channels = int(sync_part[1:])
        assert nb_channels == 4
        checksum = int(parts[3])
        assert checksum == len(str(nb_channels))

    def test_parse_with_changed_luminosity(self):
        self.sim.set_luminosity(1200)
        result = parse_data_frame(self.sim.generate_data_frame(), 4)
        assert result["values"][2] == 1200

    def test_parse_invalid_frame(self):
        frame = self.sim.simulate_noise()
        decoded = frame.decode("utf-8").strip()
        parts = decoded.split("#")
        assert parts[1] != "D" if len(parts) > 1 else True

    def test_parse_overflow_frame(self):
        frame = self.sim.simulate_overflow_frame()
        assert b"#E#OVF#" in frame


class TestErrorRecovery:
    def setup_method(self):
        self.sim = STM32Simulator(num_channels=4)

    def test_disconnect_response_bytes(self):
        resp = self.sim.process_command(b"#P#\n")
        assert resp == b"#DISCONNECTED#\r\n"
        assert isinstance(resp, bytes)

    def test_stop_response_bytes(self):
        self.sim.streaming = True
        resp = self.sim.process_command(b"#S#\n")
        assert resp == b"#STOP#\r\n"
        assert isinstance(resp, bytes)

    def test_streaming_stops_on_disconnect(self):
        self.sim.streaming = True
        self.sim.process_command(b"#P#\n")
        assert self.sim.streaming is False

    def test_streaming_stops_on_stop(self):
        self.sim.streaming = True
        self.sim.process_command(b"#S#\n")
        assert self.sim.streaming is False

    def test_reconnect_after_disconnect(self):
        self.sim.streaming = True
        self.sim.process_command(b"#P#\n")
        assert self.sim.streaming is False
        resp = self.sim.process_command(b"#?#\n")
        assert resp is not None
        assert b"#D#!" in resp
        self.sim.process_command(b"#A#\n")
        assert self.sim.streaming is True
        frame = self.sim.generate_data_frame()
        assert frame.startswith(b"#D#")

    def test_malformed_frame_no_checksum(self):
        frame = b"#D#100#200#300#\n"
        decoded = frame.decode("utf-8").strip()
        parts = decoded.split("#")
        raw = parts[2:-1]
        assert len(raw) < 5

    def test_malformed_frame_empty_values(self):
        frame = b"#D####\n"
        decoded = frame.decode("utf-8").strip()
        parts = decoded.split("#")
        raw = parts[2:-1]
        assert all(v == "" for v in raw)

    def test_overflow_frame_detected(self):
        frame = self.sim.simulate_overflow_frame()
        assert frame.startswith(b"#E#")
        assert b"OVF" in frame

    def test_full_disconnect_cycle(self):
        assert self.sim.process_command(b"#?#\n") is not None
        self.sim.process_command(b"#A#\n")
        assert self.sim.streaming is True
        frame = self.sim.generate_data_frame()
        r = parse_data_frame(frame, 4)
        assert len(r["values"]) == 4
        stop_resp = self.sim.process_command(b"#S#\n")
        assert stop_resp == b"#STOP#\r\n"
        assert self.sim.streaming is False
        disconnect_resp = self.sim.process_command(b"#P#\n")
        assert disconnect_resp == b"#DISCONNECTED#\r\n"
        assert self.sim.streaming is False


# ====================================================================
# Tests de vérification d'intégrité des trames
# ====================================================================


class TestFrameIntegrity:
    def test_checksum_valid(self):
        sim = STM32Simulator(num_channels=4)
        r = parse_data_frame(sim.generate_data_frame(), 4)
        expected = sum(len(str(v)) for v in r["values"])
        assert r["checksum"] == expected

    def test_checksum_invalid(self):
        frame = self._corrupt_checksum()
        decoded = frame.decode("utf-8").strip()
        parts = decoded.split("#")
        raw = parts[2:-1]
        cs_raw = raw[-1]
        vals = raw[:-1]
        expected = sum(len(v) for v in vals)
        assert int(cs_raw) != expected

    def test_wrong_channel_count_rejected(self):
        frame = b"#D#100#200#300#9#\n"
        decoded = frame.decode("utf-8").strip()
        parts = decoded.split("#")
        raw = parts[2:-1]
        vals = raw[:-1]
        assert len(vals) != 4

    def _corrupt_checksum(self) -> bytes:
        sim = STM32Simulator(num_channels=4)
        frame = sim.generate_data_frame()
        decoded = frame.decode("utf-8").strip()
        parts = decoded.split("#")
        parts[-2] = "9999" if len(parts) >= 2 else parts[-3]
        return "#".join(parts).encode("utf-8")


# ====================================================================
# Tests de sauvegarde CSV
# ====================================================================


class TestCSVOutput:
    def test_csv_headers_format(self):
        channel_names = {
            "Ch0": "Humidity (%)",
            "Ch1": "Temperature (°C)",
            "Ch2": "Luminosity (lux)",
            "Ch3": "Channel_4",
        }
        headers = ["timestamp"] + [channel_names[f"Ch{i}"] for i in range(4)]
        assert headers == [
            "timestamp",
            "Humidity (%)",
            "Temperature (°C)",
            "Luminosity (lux)",
            "Channel_4",
        ]

    def test_csv_write_and_read(self):
        sim = STM32Simulator(num_channels=4)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".csv", delete=False) as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "Humidity (%)", "Temperature (°C)",
                        "Luminosity (lux)", "Channel_4"])
            r = parse_data_frame(sim.generate_data_frame(), 4)
            w.writerow([round(0.0, 4)] + r["values"])
            fname = f.name
        try:
            with open(fname) as f:
                rows = list(csv.reader(f))
                assert len(rows[1]) == 5
                assert rows[1][3] == "97"
        finally:
            os.unlink(fname)

    def test_csv_multiple_rows(self):
        sim = STM32Simulator(num_channels=4)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".csv", delete=False) as f:
            w = csv.writer(f)
            w.writerow(["timestamp", "Humidity (%)", "Temperature (°C)",
                        "Luminosity (lux)", "Channel_4"])
            for t in range(5):
                sim.set_luminosity(100 + t * 10)
                r = parse_data_frame(sim.generate_data_frame(), 4)
                w.writerow([round(t * 0.1, 4)] + r["values"])
            fname = f.name
        try:
            with open(fname) as f:
                rows = list(csv.reader(f))
                assert len(rows) == 6
                assert rows[1][0] == "0.0"
                assert rows[3][3] == "120"
        finally:
            os.unlink(fname)


# ====================================================================
# Tests de bout en bout
# ====================================================================


class TestEndToEnd:
    def test_full_session_data_flow(self):
        sim = STM32Simulator(num_channels=4)

        sync_resp = sim.process_command(b"#?#\n")
        assert sync_resp is not None
        assert b"#D#!" in sync_resp

        sim.process_command(b"#A#\n")
        assert sim.streaming is True

        frames = []
        for _ in range(10):
            frames.append(sim.generate_data_frame())
        assert len(frames) == 10
        for f in frames:
            r = parse_data_frame(f, 4)
            assert all(v is not None for v in r["values"])

        stop_resp = sim.process_command(b"#S#\n")
        assert sim.streaming is False
        assert b"#STOP#" in stop_resp

    def test_data_consistency_across_session(self):
        sim = STM32Simulator(num_channels=4)
        sim.process_command(b"#A#\n")
        values = []
        for lux in [100, 200, 300, 400, 500]:
            sim.set_luminosity(lux)
            r = parse_data_frame(sim.generate_data_frame(), 4)
            values.append(r["values"][2])
        assert values == [100, 200, 300, 400, 500]

    def test_invalid_frame_in_stream(self):
        sim = STM32Simulator(num_channels=4)
        assert sim.generate_data_frame().startswith(b"#D#")
        assert not sim.simulate_noise().startswith(b"#D#")
        assert not sim.simulate_overflow_frame().startswith(b"#D#")
