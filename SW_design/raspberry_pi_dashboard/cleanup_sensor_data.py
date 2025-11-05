#!/usr/bin/env python3
"""
Cleanup script for old sensor data.
Run this periodically (e.g., via cron) to remove old sensor data.
"""

import sys
from flask import Flask
from config import get_config
from models import db, SensorData
from udp_listener import cleanup_old_sensor_data

def main():
    """Main cleanup function."""
    app = Flask(__name__)
    config_class = get_config()
    app.config.from_object(config_class)

    db.init_app(app)

    retention_hours = app.config['SENSOR_DATA_RETENTION_HOURS']

    print(f"Cleaning up sensor data older than {retention_hours} hours...")

    deleted_count = cleanup_old_sensor_data(app, retention_hours)

    print(f"✓ Deleted {deleted_count} old sensor data records")

    # Also run VACUUM to reclaim disk space
    with app.app_context():
        try:
            db.session.execute(db.text('VACUUM'))
            print("✓ Database vacuumed successfully")
        except Exception as e:
            print(f"Warning: Could not vacuum database: {e}")

    return 0


if __name__ == '__main__':
    sys.exit(main())
