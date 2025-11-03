"""
UDP Listener for ESP32 Sensor Data.
Receives JSON packets on port 5000 and stores data in database.
"""

import socket
import json
import logging
import threading
from datetime import datetime
from models import db, Device, SensorData

logger = logging.getLogger(__name__)


class UDPListener:
    """UDP listener for ESP32 sensor data packets."""

    def __init__(self, app, socketio, port=5000, activity_timeout=5):
        """
        Initialize UDP listener.

        Args:
            app: Flask application instance
            socketio: Flask-SocketIO instance for broadcasting
            port: UDP port to listen on (default: 5000)
            activity_timeout: Device activity timeout in seconds (default: 5)
        """
        self.app = app
        self.socketio = socketio
        self.port = port
        self.activity_timeout = activity_timeout
        self.socket = None
        self.running = False
        self.thread = None

    def start(self):
        """Start the UDP listener in a separate thread."""
        if self.running:
            logger.warning("UDP listener already running")
            return

        self.running = True
        self.thread = threading.Thread(target=self._listen, daemon=True)
        self.thread.start()
        logger.info(f"UDP listener started on port {self.port}")

    def stop(self):
        """Stop the UDP listener."""
        self.running = False
        if self.socket:
            self.socket.close()
        logger.info("UDP listener stopped")

    def _listen(self):
        """Main listening loop (runs in separate thread)."""
        try:
            # Create UDP socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket.bind(('0.0.0.0', self.port))
            self.socket.settimeout(1.0)  # 1 second timeout for clean shutdown

            logger.info(f"Listening for ESP32 data on UDP port {self.port}...")

            while self.running:
                try:
                    # Receive data (max 2048 bytes)
                    data, addr = self.socket.recvfrom(2048)

                    # Process in Flask app context (for database access)
                    with self.app.app_context():
                        self._process_packet(data, addr)

                except socket.timeout:
                    # Timeout is expected, continue loop
                    continue
                except Exception as e:
                    logger.error(f"Error processing packet: {e}", exc_info=True)

        except Exception as e:
            logger.error(f"UDP listener error: {e}", exc_info=True)
        finally:
            if self.socket:
                self.socket.close()

    def _process_packet(self, data, addr):
        """
        Process incoming UDP packet.

        Args:
            data: Raw packet data (bytes)
            addr: Source address tuple (ip, port)
        """
        try:
            # Parse JSON
            json_str = data.decode('utf-8')
            packet = json.loads(json_str)

            # LOG RAW INCOMING PACKET
            logger.info(f"📦 RAW UDP PACKET from {addr[0]}: {json_str}")

            # Extract node_id (required)
            node_id = packet.get('node_id')
            if not node_id:
                logger.warning(f"Packet from {addr} missing node_id, ignoring")
                return

            # Update or create device
            device = Device.query.filter_by(node_id=node_id).first()
            if not device:
                device = Device(
                    node_id=node_id,
                    hostname=node_id,  # Default hostname to node_id
                    ip_address=addr[0],
                    last_seen=datetime.utcnow()
                )
                db.session.add(device)
                logger.info(f"New device discovered: {node_id} at {addr[0]}")
            else:
                device.last_seen = datetime.utcnow()
                device.ip_address = addr[0]

            # Create sensor data record
            sensor_data = SensorData(
                node_id=node_id,
                timestamp=datetime.utcnow(),
                esp_timestamp=packet.get('timestamp'),
                temperature=packet.get('temperature'),
                pressure=packet.get('pressure'),
                light_intensity=packet.get('light_intensity'),
                accel_x=packet.get('accel_x'),
                accel_y=packet.get('accel_y'),
                accel_z=packet.get('accel_z'),
                accel_norm=packet.get('accel_norm'),
                gyro_x=packet.get('gyro_x'),
                gyro_y=packet.get('gyro_y'),
                gyro_z=packet.get('gyro_z'),
                gyro_norm=packet.get('gyro_norm'),
                mag_x=packet.get('mag_x'),
                mag_y=packet.get('mag_y'),
                mag_z=packet.get('mag_z'),
                mag_norm=packet.get('mag_norm'),
                volume_rms=packet.get('volume_rms'),
                spectral_f1_405nm=packet.get('spectral_f1'),
                spectral_f2_425nm=packet.get('spectral_f2'),
                spectral_f3_475nm=packet.get('spectral_f3'),
                spectral_f4_515nm=packet.get('spectral_f4'),
                spectral_fz_450nm=packet.get('spectral_fz'),
                spectral_fy_555nm=packet.get('spectral_fy'),
                spectral_f5_550nm=packet.get('spectral_f5'),
                spectral_f6_640nm=packet.get('spectral_f6'),
                spectral_fxl_600nm=packet.get('spectral_fxl'),
                spectral_f7_690nm=packet.get('spectral_f7'),
                spectral_f8_745nm=packet.get('spectral_f8'),
                spectral_nir_855nm=packet.get('spectral_nir'),
                spectral_vis=packet.get('spectral_vis'),
                spectral_fd=packet.get('spectral_fd')
            )
            db.session.add(sensor_data)

            # Commit to database
            db.session.commit()

            # Broadcast to WebSocket clients (only non-null sensor values)
            data_to_broadcast = sensor_data.to_dict(include_nulls=False)
            logger.info(f"📡 BROADCASTING to WebSocket for {node_id}: {json.dumps(data_to_broadcast)}")
            self._broadcast_sensor_data(node_id, data_to_broadcast)

            logger.debug(f"Processed packet from {node_id} ({addr[0]})")

        except json.JSONDecodeError as e:
            logger.error(f"Invalid JSON from {addr}: {e}")
        except Exception as e:
            logger.error(f"Error processing packet from {addr}: {e}", exc_info=True)
            db.session.rollback()

    def _broadcast_sensor_data(self, node_id, data):
        """
        Broadcast sensor data to WebSocket clients.

        Args:
            node_id: Device node ID
            data: Sensor data dictionary
        """
        try:
            room_name = f'device_{node_id}'
            logger.info(f"📢 Emitting 'sensor_data' event to room '{room_name}'")

            # Broadcast to all clients subscribed to this node
            self.socketio.emit(
                'sensor_data',
                {
                    'node_id': node_id,
                    'data': data
                },
                room=room_name,
                namespace='/'
            )

            # Also broadcast device status update
            self.socketio.emit(
                'device_status',
                {
                    'node_id': node_id,
                    'status': 'active',
                    'last_seen': datetime.utcnow().isoformat()
                },
                namespace='/'
            )

        except Exception as e:
            logger.error(f"Error broadcasting data: {e}", exc_info=True)


def cleanup_old_sensor_data(app, retention_hours=24):
    """
    Background task to clean up old sensor data.
    Should be run periodically (e.g., every hour).

    Args:
        app: Flask application instance
        retention_hours: Keep data for this many hours (default: 24)
    """
    with app.app_context():
        try:
            deleted_count = SensorData.cleanup_old_data(retention_hours)
            logger.info(f"Cleaned up {deleted_count} old sensor data records")
            return deleted_count
        except Exception as e:
            logger.error(f"Error cleaning up sensor data: {e}", exc_info=True)
            return 0
