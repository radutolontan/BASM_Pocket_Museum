"""
Gunicorn configuration for Pocket Lab Dashboard
Production WSGI server configuration
"""

import multiprocessing
import os

# Server socket
bind = "127.0.0.1:8080"  # Bind to localhost, Nginx will reverse proxy
backlog = 2048

# Worker processes
workers = multiprocessing.cpu_count() * 2 + 1  # Recommended formula
worker_class = "eventlet"  # Required for SocketIO support
worker_connections = 1000
timeout = 120
keepalive = 5

# Logging
accesslog = "/var/log/pocketlab/access.log"
errorlog = "/var/log/pocketlab/error.log"
loglevel = "info"
access_log_format = '%(h)s %(l)s %(u)s %(t)s "%(r)s" %(s)s %(b)s "%(f)s" "%(a)s"'

# Process naming
proc_name = "pocketlab_dashboard"

# Server mechanics
daemon = False  # Systemd will manage the process
pidfile = "/var/run/pocketlab/pocketlab.pid"
umask = 0
user = None  # Run as current user (typically 'pi')
group = None
tmp_upload_dir = None

# SSL (not needed since Nginx handles this)
# If you want HTTPS, configure it in Nginx instead

# Preload app for better performance
preload_app = True

# Restart workers gracefully
max_requests = 1000
max_requests_jitter = 50
