"""Tests unitaires pour le client InfluxDB.

Valide :
- L'envoi de donnees avec send_data()
- La gestion des erreurs de connexion
- La fermeture de la connexion
"""

from unittest.mock import MagicMock, patch

import pytest


@pytest.fixture
def mock_influx_module():
    """Mock complet du module influxdb_client."""
    with patch("influx_client.InfluxDBClient") as mock_client_class, \
         patch("influx_client.Point") as mock_point_class:

        mock_client_instance = MagicMock()
        mock_write_api = MagicMock()
        mock_client_class.return_value = mock_client_instance
        mock_client_instance.write_api.return_value = mock_write_api

        mock_point_instance = MagicMock()
        mock_point_class.return_value = mock_point_instance
        mock_point_instance.field.return_value = mock_point_instance
        mock_point_instance.time.return_value = mock_point_instance

        yield {
            "client_class": mock_client_class,
            "client_instance": mock_client_instance,
            "write_api": mock_write_api,
            "point_class": mock_point_class,
            "point_instance": mock_point_instance,
        }


class TestInfluxClientInit:
    def test_init_success(self, mock_influx_module):
        from influx_client import InfluxClient
        client = InfluxClient()
        assert client.client is not None
        assert client.write_api is not None
        mock_influx_module["client_class"].assert_called_once()

    def test_init_failure_sets_none(self):
        with patch("influx_client.InfluxDBClient", side_effect=Exception("DB down")):
            from influx_client import InfluxClient
            client = InfluxClient()
            assert client.client is None
            assert client.write_api is None


class TestInfluxClientSendData:
    def test_send_data_valid(self, mock_influx_module):
        from influx_client import InfluxClient
        client = InfluxClient()
        client.send_data(
            channel_names=["Humidity (%)", "Temperature (C)", "Luminosity (lux)", "Channel_4"],
            values=[52, 21, 97, 0],
            timestamp=1.234,
        )
        point = mock_influx_module["point_class"].return_value
        expected_calls = [
            ("field", ("Humidity (%)", 52.0)),
            ("field", ("Temperature (C)", 21.0)),
            ("field", ("Luminosity (lux)", 97.0)),
            ("field", ("Channel_4", 0.0)),
        ]
        for method, args in expected_calls:
            getattr(point, method).assert_any_call(*args)
        mock_influx_module["write_api"].write.assert_called_once()

    def test_send_data_no_write_api(self, mock_influx_module):
        with patch("influx_client.InfluxDBClient", side_effect=Exception("DB down")):
            from influx_client import InfluxClient
            client = InfluxClient()
            assert client.write_api is None
            client.send_data(
                channel_names=["Humidity (%)"],
                values=[52],
                timestamp=1.0,
            )
            mock_influx_module["write_api"].write.assert_not_called()

    def test_send_data_single_channel(self, mock_influx_module):
        from influx_client import InfluxClient
        client = InfluxClient()
        client.send_data(
            channel_names=["Temperature (C)"],
            values=[25],
            timestamp=5.0,
        )
        point = mock_influx_module["point_class"].return_value
        point.field.assert_called_once_with("Temperature (C)", 25.0)

    def test_send_data_empty_lists(self, mock_influx_module):
        from influx_client import InfluxClient
        client = InfluxClient()
        client.send_data(
            channel_names=[],
            values=[],
            timestamp=0.0,
        )
        mock_influx_module["write_api"].write.assert_called_once()


class TestInfluxClientClose:
    def test_close_with_client(self, mock_influx_module):
        from influx_client import InfluxClient
        client = InfluxClient()
        client.close()
        mock_influx_module["client_instance"].close.assert_called_once()

    def test_close_no_client(self):
        with patch("influx_client.InfluxDBClient", side_effect=Exception("DB down")):
            from influx_client import InfluxClient
            client = InfluxClient()
            client.close()
