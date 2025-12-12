#!/usr/bin/env python3
"""
Database migration to add UV spectral sensor and thermal camera fields.
Run this script to update the database schema for new sensor support.
"""

import sqlite3
import sys
from pathlib import Path

# Database path
DB_PATH = Path(__file__).parent / 'instance' / 'dashboard.db'

def migrate():
    """Add UV spectral and thermal camera columns to sensor_data table."""
    if not DB_PATH.exists():
        print(f"❌ Database not found at {DB_PATH}")
        print("   Run init_db.py first to create the database.")
        sys.exit(1)

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    try:
        # Check if columns already exist
        cursor.execute("PRAGMA table_info(sensor_data)")
        existing_columns = [row[1] for row in cursor.fetchall()]

        columns_to_add = []

        # UV Spectral sensor columns
        if 'spectral_UVA' not in existing_columns:
            columns_to_add.append(('spectral_UVA', 'REAL'))
        if 'spectral_UVB' not in existing_columns:
            columns_to_add.append(('spectral_UVB', 'REAL'))
        if 'spectral_UVC' not in existing_columns:
            columns_to_add.append(('spectral_UVC', 'REAL'))

        # Thermal imaging camera column
        if 'thermal_pixels' not in existing_columns:
            columns_to_add.append(('thermal_pixels', 'TEXT'))

        if not columns_to_add:
            print("✅ Database already up to date. No migration needed.")
            return

        # Add new columns
        print(f"🔧 Adding {len(columns_to_add)} new columns to sensor_data table...")
        for column_name, column_type in columns_to_add:
            sql = f"ALTER TABLE sensor_data ADD COLUMN {column_name} {column_type}"
            print(f"   Adding column: {column_name} ({column_type})")
            cursor.execute(sql)

        conn.commit()
        print("✅ Migration completed successfully!")
        print(f"   Added columns: {', '.join([col[0] for col in columns_to_add])}")

        # Update schema version if table exists
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='schema_version'")
        if cursor.fetchone():
            cursor.execute(
                "INSERT INTO schema_version (version, description) VALUES (?, ?)",
                (2, "Added UV spectral sensor (AS7331) and thermal camera (AMG88XX) support")
            )
            conn.commit()
            print("   Schema version updated to 2")

    except sqlite3.Error as e:
        print(f"❌ Migration failed: {e}")
        conn.rollback()
        sys.exit(1)
    finally:
        conn.close()

if __name__ == '__main__':
    migrate()
