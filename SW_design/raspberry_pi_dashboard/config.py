"""
Configuration module for the Raspberry Pi Dashboard.
Loads settings from environment variables with sensible defaults.
"""

import os
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()


class Config:
    """Application configuration class."""

    # Flask
    SECRET_KEY = os.getenv('SECRET_KEY', 'dev-secret-key-change-in-production')
    FLASK_ENV = os.getenv('FLASK_ENV', 'development')
    DEBUG = os.getenv('FLASK_DEBUG', 'False').lower() == 'true'

    # Database
    DATABASE_URL = os.getenv('DATABASE_URL', 'sqlite:///dashboard.db')
    SQLALCHEMY_DATABASE_URI = DATABASE_URL
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    SQLALCHEMY_ENGINE_OPTIONS = {
        'pool_pre_ping': True,
        'pool_recycle': 300,
    }

    # Network
    UDP_PORT = int(os.getenv('UDP_PORT', 5000))
    WEB_PORT = int(os.getenv('WEB_PORT', 8080))
    BIND_ADDRESS = os.getenv('BIND_ADDRESS', '0.0.0.0')

    # Device Activity
    ACTIVITY_TIMEOUT_SECONDS = int(os.getenv('ACTIVITY_TIMEOUT_SECONDS', 5))

    # Data Retention
    SENSOR_DATA_RETENTION_HOURS = int(os.getenv('SENSOR_DATA_RETENTION_HOURS', 24))

    # WebSocket
    WEBSOCKET_PING_INTERVAL = int(os.getenv('WEBSOCKET_PING_INTERVAL', 25))
    WEBSOCKET_PING_TIMEOUT = int(os.getenv('WEBSOCKET_PING_TIMEOUT', 60))

    # Broadcast Rate Limiting
    MAX_BROADCAST_RATE_HZ = int(os.getenv('MAX_BROADCAST_RATE_HZ', 10))

    # CORS (allow all origins for classroom use)
    CORS_ORIGINS = '*'

    @staticmethod
    def init_app(app):
        """Initialize application with this config."""
        pass


class DevelopmentConfig(Config):
    """Development configuration."""
    DEBUG = True
    FLASK_ENV = 'development'


class ProductionConfig(Config):
    """Production configuration."""
    DEBUG = False
    FLASK_ENV = 'production'

    @classmethod
    def init_app(cls, app):
        """Production-specific initialization."""
        Config.init_app(app)

        # Log to syslog in production
        import logging
        from logging.handlers import SysLogHandler
        syslog_handler = SysLogHandler()
        syslog_handler.setLevel(logging.WARNING)
        app.logger.addHandler(syslog_handler)


# Configuration dictionary
config = {
    'development': DevelopmentConfig,
    'production': ProductionConfig,
    'default': DevelopmentConfig
}


def get_config():
    """Get configuration based on FLASK_ENV."""
    env = os.getenv('FLASK_ENV', 'development')
    return config.get(env, config['default'])
