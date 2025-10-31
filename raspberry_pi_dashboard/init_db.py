"""
Database initialization script.
Creates all tables and indexes, and inserts initial schema version.
"""

from flask import Flask
from config import get_config
from models import db, SchemaVersion

def init_database():
    """Initialize the database with all tables."""
    app = Flask(__name__)
    config_class = get_config()
    app.config.from_object(config_class)

    db.init_app(app)

    with app.app_context():
        print("Creating database tables...")

        # Create all tables
        db.create_all()

        # Check if schema version already exists
        version = SchemaVersion.query.filter_by(version=1).first()

        if not version:
            # Insert initial schema version
            version = SchemaVersion(
                version=1,
                description='Initial schema with users, devices, sensor_data, questions, answers, user_preferences'
            )
            db.session.add(version)
            db.session.commit()
            print("✓ Database initialized successfully!")
            print("✓ Schema version 1 applied")
        else:
            print("✓ Database already initialized")
            print(f"✓ Current schema version: {version.version}")

        print("\nDatabase tables created:")
        print("  - users")
        print("  - devices")
        print("  - sensor_data")
        print("  - questions")
        print("  - answers")
        print("  - user_preferences")
        print("  - schema_version")


if __name__ == '__main__':
    init_database()
