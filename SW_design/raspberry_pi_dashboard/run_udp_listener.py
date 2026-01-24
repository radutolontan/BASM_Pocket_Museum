#!/usr/bin/env python3
"""
Standalone UDP Listener Service for ESP32 Sensor Data.
This script runs the UDP listener as a separate systemd service.
"""

import os
import sys
import signal
import logging
from flask import Flask
from flask_socketio import SocketIO

from config import get_config
from models import db
from udp_listener import UDPListener

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Global variables for cleanup
app = None
socketio = None
udp_listener = None


def signal_handler(signum, frame):
    """Handle shutdown signals gracefully."""
    logger.info(f"Received signal {signum}, shutting down...")
    if udp_listener:
        udp_listener.stop()
    sys.exit(0)


def main():
    """Main entry point for standalone UDP listener."""
    global app, socketio, udp_listener

    # Register signal handlers for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    logger.info("Starting Pocket Lab UDP Listener Service")

    # Initialize Flask app with minimal configuration
    app = Flask(__name__)
    config_class = get_config()
    app.config.from_object(config_class)

    # Initialize database
    db.init_app(app)

    # Initialize SocketIO with threading mode
    socketio = SocketIO(
        app,
        cors_allowed_origins=app.config['CORS_ORIGINS'],
        async_mode='threading'
    )

    # Create database tables if needed
    with app.app_context():
        db.create_all()
        logger.info("Database tables verified/created")

    # Initialize and start UDP listener
    udp_listener = UDPListener(
        app,
        socketio,
        port=app.config['UDP_PORT'],
        activity_timeout=app.config['ACTIVITY_TIMEOUT_SECONDS']
    )

    logger.info(f"Starting UDP listener on port {app.config['UDP_PORT']}")
    udp_listener.start()

    logger.info("UDP Listener service is running. Press Ctrl+C to stop.")

    # Keep the main thread alive
    try:
        # Wait indefinitely (the UDP listener runs in a daemon thread)
        signal.pause()
    except AttributeError:
        # signal.pause() not available on Windows, use alternative
        import time
        while True:
            time.sleep(1)


if __name__ == '__main__':
    main()
