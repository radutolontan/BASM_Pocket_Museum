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
        self.last_status_broadcast = {}  # Track last device_status broadcast time per device

        # Packet rate tracking (for simplified logging at 0.2 Hz)
        self.packet_counts = {}  # {node_id: count}
        self.last_summary_time = datetime.utcnow()
        self.summary_interval = 5.0  # Log summary every 5 seconds (0.2 Hz)

        # Batched database commits (to reduce I/O bottleneck)
        self.pending_commits = 0
        self.last_commit_time = datetime.utcnow()
        self.commit_batch_size = 25  # Commit every 25 packets (scales to 15 ESP32s at 25 Hz)
        self.commit_max_delay = 0.2  # Or every 200ms, whichever comes first

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

    def _log_summary_stats(self):
        """Log packet rate summary for all connected ESP32s (every 5 seconds at 0.2 Hz)."""
        now = datetime.utcnow()
        elapsed = (now - self.last_summary_time).total_seconds()

        if elapsed >= self.summary_interval:
            if self.packet_counts:
                # Calculate rates and build summary message
                summary_parts = []
                for node_id, count in sorted(self.packet_counts.items()):
                    rate = count / elapsed
                    summary_parts.append(f"{node_id}: {rate:.1f} Hz ({count} packets)")

                logger.info(f"📊 UDP Packet Rates: {' | '.join(summary_parts)}")

                # Reset counters
                self.packet_counts = {}
                self.last_summary_time = now

    def _maybe_commit(self):
        """
        Commit database changes if batch size or time threshold reached.
        Batching reduces I/O bottleneck:
        - 2 ESP32s (50 pkt/sec): 50 commits/sec → 2 commits/sec (96% reduction)
        - 15 ESP32s (375 pkt/sec): 375 commits/sec → 15 commits/sec (96% reduction)
        """
        now = datetime.utcnow()
        time_since_commit = (now - self.last_commit_time).total_seconds()

        # Commit if we've accumulated enough packets OR enough time has passed
        if self.pending_commits >= self.commit_batch_size or time_since_commit >= self.commit_max_delay:
            try:
                db.session.commit()
                self.pending_commits = 0
                self.last_commit_time = now
            except Exception as e:
                logger.error(f"Database commit error: {e}", exc_info=True)
                db.session.rollback()
                self.pending_commits = 0

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

            # Extract node_id (required)
            node_id = packet.get('node_id')
            if not node_id:
                logger.warning(f"Packet from {addr} missing node_id, ignoring")
                return

            # Track packet count for this device
            self.packet_counts[node_id] = self.packet_counts.get(node_id, 0) + 1

            # Log summary stats every 5 seconds (0.2 Hz)
            self._log_summary_stats()

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

            # Parse thermal pixels array if present
            thermal_pixels_json = None
            if 'thermal_pixels' in packet:
                thermal_array = packet['thermal_pixels']
                if thermal_array:
                    # Convert 2D array to JSON string for storage
                    thermal_pixels_json = json.dumps(thermal_array)

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
                spectral_fd=packet.get('spectral_fd'),
                spectral_UVA=packet.get('spectral_UVA'),
                spectral_UVB=packet.get('spectral_UVB'),
                spectral_UVC=packet.get('spectral_UVC'),
                thermal_pixels=thermal_pixels_json
            )
            db.session.add(sensor_data)
            self.pending_commits += 1

            # Batch commit (every 25 packets or 200ms, whichever comes first)
            self._maybe_commit()

            # Broadcast to WebSocket clients (only non-null sensor values)
            data_to_broadcast = sensor_data.to_dict(include_nulls=False)
            self._broadcast_sensor_data(node_id, data_to_broadcast)

        except json.JSONDecodeError as e:
            logger.error(f"Invalid JSON from {addr}: {e}")
        except Exception as e:
            logger.error(f"Error processing packet from {addr}: {e}", exc_info=True)
            db.session.rollback()
            self.pending_commits = 0  # Reset counter after rollback

    def _broadcast_sensor_data(self, node_id, data):
        """
        Broadcast sensor data to WebSocket clients.

        Args:
            node_id: Device node ID
            data: Sensor data dictionary
        """
        try:
            room_name = f'device_{node_id}'

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

            # Rate-limit device status updates to 1 Hz (once per second)
            # This prevents overwhelming clients when multiple ESP32s are active
            now = datetime.utcnow()
            last_broadcast = self.last_status_broadcast.get(node_id)

            if last_broadcast is None or (now - last_broadcast).total_seconds() >= 1.0:
                self.last_status_broadcast[node_id] = now

                # Broadcast device status update (to everyone, not room-specific)
                self.socketio.emit(
                    'device_status',
                    {
                        'node_id': node_id,
                        'status': 'active',
                        'last_seen': now.isoformat()
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
