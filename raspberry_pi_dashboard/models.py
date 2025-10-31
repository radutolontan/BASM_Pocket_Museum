"""
Database models for the Raspberry Pi Dashboard.
Uses SQLAlchemy ORM for database operations.
"""

from datetime import datetime, timedelta
from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()


class User(db.Model):
    """User model for student registration."""
    __tablename__ = 'users'

    mac_id = db.Column(db.String(64), primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    grade = db.Column(db.String(20))
    science_interest = db.Column(db.Text)
    first_seen = db.Column(db.DateTime, default=datetime.utcnow)
    last_seen = db.Column(db.DateTime, default=datetime.utcnow)

    # Relationships
    answers = db.relationship('Answer', backref='user', lazy='dynamic', cascade='all, delete-orphan')
    preferences = db.relationship('UserPreference', backref='user', lazy='dynamic', cascade='all, delete-orphan')

    def to_dict(self):
        """Convert to dictionary for JSON serialization."""
        return {
            'mac_id': self.mac_id,
            'name': self.name,
            'grade': self.grade,
            'science_interest': self.science_interest,
            'first_seen': self.first_seen.isoformat() if self.first_seen else None,
            'last_seen': self.last_seen.isoformat() if self.last_seen else None
        }

    def __repr__(self):
        return f'<User {self.name} ({self.mac_id[:8]}...)>'


class Device(db.Model):
    """Device model for ESP32 tracking."""
    __tablename__ = 'devices'

    node_id = db.Column(db.String(50), primary_key=True)
    hostname = db.Column(db.String(100))
    ip_address = db.Column(db.String(15))
    last_seen = db.Column(db.DateTime, default=datetime.utcnow)
    is_active = db.Column(db.Boolean, default=True)

    # Relationships
    sensor_data = db.relationship('SensorData', backref='device', lazy='dynamic', cascade='all, delete-orphan')
    preferences = db.relationship('UserPreference', backref='device', lazy='dynamic', cascade='all, delete-orphan')

    def to_dict(self, activity_timeout_seconds=5):
        """Convert to dictionary for JSON serialization."""
        is_currently_active = (
            datetime.utcnow() - self.last_seen
        ).total_seconds() < activity_timeout_seconds if self.last_seen else False

        return {
            'node_id': self.node_id,
            'hostname': self.hostname,
            'ip_address': self.ip_address,
            'last_seen': self.last_seen.isoformat() if self.last_seen else None,
            'is_active': is_currently_active
        }

    def __repr__(self):
        return f'<Device {self.node_id} ({self.hostname})>'


class SensorData(db.Model):
    """Sensor data model for time-series storage."""
    __tablename__ = 'sensor_data'

    id = db.Column(db.Integer, primary_key=True)
    node_id = db.Column(db.String(50), db.ForeignKey('devices.node_id', ondelete='CASCADE'), nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow, nullable=False, index=True)
    esp_timestamp = db.Column(db.Integer)  # millis() from ESP32

    # Pressure sensor
    temperature = db.Column(db.Float)
    pressure = db.Column(db.Float)

    # Ambient light sensor
    light_intensity = db.Column(db.Float)

    # IMU - Accelerometer
    accel_x = db.Column(db.Float)
    accel_y = db.Column(db.Float)
    accel_z = db.Column(db.Float)
    accel_norm = db.Column(db.Float)

    # IMU - Gyroscope
    gyro_x = db.Column(db.Float)
    gyro_y = db.Column(db.Float)
    gyro_z = db.Column(db.Float)
    gyro_norm = db.Column(db.Float)

    # IMU - Magnetometer
    mag_x = db.Column(db.Float)
    mag_y = db.Column(db.Float)
    mag_z = db.Column(db.Float)
    mag_norm = db.Column(db.Float)

    # Microphone
    volume_rms = db.Column(db.Float)

    # Spectral sensor (AS7343 - 14 channels)
    spectral_f1_405nm = db.Column(db.Float)
    spectral_f2_425nm = db.Column(db.Float)
    spectral_f3_475nm = db.Column(db.Float)
    spectral_f4_515nm = db.Column(db.Float)
    spectral_fz_450nm = db.Column(db.Float)
    spectral_fy_555nm = db.Column(db.Float)
    spectral_f5_550nm = db.Column(db.Float)
    spectral_f6_640nm = db.Column(db.Float)
    spectral_fxl_600nm = db.Column(db.Float)
    spectral_f7_690nm = db.Column(db.Float)
    spectral_f8_745nm = db.Column(db.Float)
    spectral_nir_855nm = db.Column(db.Float)
    spectral_vis = db.Column(db.Float)
    spectral_fd = db.Column(db.Float)

    # Composite index for common queries
    __table_args__ = (
        db.Index('idx_node_timestamp', 'node_id', 'timestamp'),
    )

    def to_dict(self, include_nulls=False):
        """Convert to dictionary for JSON serialization."""
        data = {
            'id': self.id,
            'node_id': self.node_id,
            'timestamp': self.timestamp.isoformat() if self.timestamp else None,
            'esp_timestamp': self.esp_timestamp,
        }

        # Sensor fields (only include non-null values unless include_nulls=True)
        sensor_fields = [
            'temperature', 'pressure', 'light_intensity',
            'accel_x', 'accel_y', 'accel_z', 'accel_norm',
            'gyro_x', 'gyro_y', 'gyro_z', 'gyro_norm',
            'mag_x', 'mag_y', 'mag_z', 'mag_norm',
            'volume_rms',
            'spectral_f1_405nm', 'spectral_f2_425nm', 'spectral_f3_475nm',
            'spectral_f4_515nm', 'spectral_fz_450nm', 'spectral_fy_555nm',
            'spectral_f5_550nm', 'spectral_f6_640nm', 'spectral_fxl_600nm',
            'spectral_f7_690nm', 'spectral_f8_745nm', 'spectral_nir_855nm',
            'spectral_vis', 'spectral_fd'
        ]

        for field in sensor_fields:
            value = getattr(self, field)
            if include_nulls or value is not None:
                data[field] = value

        return data

    @classmethod
    def cleanup_old_data(cls, retention_hours=24):
        """Delete sensor data older than retention_hours."""
        cutoff_time = datetime.utcnow() - timedelta(hours=retention_hours)
        deleted_count = cls.query.filter(cls.timestamp < cutoff_time).delete()
        db.session.commit()
        return deleted_count

    def __repr__(self):
        return f'<SensorData {self.node_id} @ {self.timestamp}>'


class Question(db.Model):
    """Question model for teacher-published questions."""
    __tablename__ = 'questions'

    id = db.Column(db.Integer, primary_key=True)
    question_text = db.Column(db.Text, nullable=False)
    question_type = db.Column(db.String(20), nullable=False)  # 'multiple_choice' or 'free_text'
    options = db.Column(db.Text)  # JSON array for multiple choice
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    is_active = db.Column(db.Boolean, default=True, index=True)
    created_by = db.Column(db.String(100))

    # Relationships
    answers = db.relationship('Answer', backref='question', lazy='dynamic', cascade='all, delete-orphan')

    def to_dict(self):
        """Convert to dictionary for JSON serialization."""
        import json
        return {
            'id': self.id,
            'question_text': self.question_text,
            'question_type': self.question_type,
            'options': json.loads(self.options) if self.options else None,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'is_active': self.is_active,
            'created_by': self.created_by
        }

    def __repr__(self):
        return f'<Question {self.id}: {self.question_text[:50]}...>'


class Answer(db.Model):
    """Answer model for student responses."""
    __tablename__ = 'answers'

    id = db.Column(db.Integer, primary_key=True)
    question_id = db.Column(db.Integer, db.ForeignKey('questions.id', ondelete='CASCADE'), nullable=False)
    mac_id = db.Column(db.String(64), db.ForeignKey('users.mac_id', ondelete='CASCADE'), nullable=False)
    answer = db.Column(db.Text, nullable=False)
    answered_at = db.Column(db.DateTime, default=datetime.utcnow)

    # Unique constraint: one answer per user per question
    __table_args__ = (
        db.UniqueConstraint('question_id', 'mac_id', name='uq_question_user'),
    )

    def to_dict(self):
        """Convert to dictionary for JSON serialization."""
        return {
            'id': self.id,
            'question_id': self.question_id,
            'mac_id': self.mac_id,
            'answer': self.answer,
            'answered_at': self.answered_at.isoformat() if self.answered_at else None
        }

    def __repr__(self):
        return f'<Answer {self.id}: Q{self.question_id} by {self.mac_id[:8]}...>'


class UserPreference(db.Model):
    """User preference model for sensor display settings."""
    __tablename__ = 'user_preferences'

    id = db.Column(db.Integer, primary_key=True)
    mac_id = db.Column(db.String(64), db.ForeignKey('users.mac_id', ondelete='CASCADE'), nullable=False)
    node_id = db.Column(db.String(50), db.ForeignKey('devices.node_id', ondelete='CASCADE'), nullable=False)
    sensor_type = db.Column(db.String(50), nullable=False)
    display_type = db.Column(db.String(20), nullable=False)  # 'stripchart', 'numeric', 'both', 'none'
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    # Unique constraint: one preference per user/device/sensor combo
    __table_args__ = (
        db.UniqueConstraint('mac_id', 'node_id', 'sensor_type', name='uq_user_device_sensor'),
    )

    def to_dict(self):
        """Convert to dictionary for JSON serialization."""
        return {
            'id': self.id,
            'mac_id': self.mac_id,
            'node_id': self.node_id,
            'sensor_type': self.sensor_type,
            'display_type': self.display_type,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'updated_at': self.updated_at.isoformat() if self.updated_at else None
        }

    def __repr__(self):
        return f'<UserPreference {self.mac_id[:8]}... {self.node_id} {self.sensor_type}={self.display_type}>'


class SchemaVersion(db.Model):
    """Schema version tracking for database migrations."""
    __tablename__ = 'schema_version'

    version = db.Column(db.Integer, primary_key=True)
    applied_at = db.Column(db.DateTime, default=datetime.utcnow)
    description = db.Column(db.Text)

    def __repr__(self):
        return f'<SchemaVersion {self.version}: {self.description}>'
