# Production Deployment Guide

This guide explains how to deploy the Pocket Lab Dashboard in production mode on your Raspberry Pi.

## Quick Start

Run the automated setup script:

```bash
cd ~/BASM_Pocket_Museum/SW_design/raspberry_pi_dashboard
./setup_production.sh
```

This script will:
1. Install Nginx and Avahi (mDNS)
2. Set up Python virtual environment with Gunicorn
3. Configure the hostname to `pocketlab`
4. Set up Nginx reverse proxy (port 80 → 8080)
5. Create and enable the systemd service for auto-start

After setup, reboot your Raspberry Pi:
```bash
sudo reboot
```

## Student-Friendly URLs

Students can access the dashboard using any of these URLs:

- **`http://pocketlab.local`** ✅ Recommended (works on all devices)
- **`http://pocketlab`** (works on most devices)
- **`http://192.168.10.2`** (fallback if mDNS doesn't work)

All URLs automatically redirect to the live dashboard!

## Production Architecture

```
Student Browser
    ↓
http://pocketlab.local (port 80)
    ↓
Nginx (reverse proxy)
    ↓
Gunicorn WSGI Server (port 8080)
    ↓
Flask Application + SocketIO
    ↓
SQLite Database
```

## Manual Setup (if needed)

If you prefer to set up manually or need to troubleshoot:

### 1. Install Dependencies

```bash
sudo apt-get update
sudo apt-get install -y nginx avahi-daemon python3-pip python3-venv
```

### 2. Set Up Python Environment

```bash
cd ~/BASM_Pocket_Museum/SW_design/raspberry_pi_dashboard
python3 -m venv venv
source venv/bin/activate
pip install gunicorn eventlet flask flask-cors flask-socketio python-socketio sqlalchemy
```

### 3. Configure Hostname

```bash
sudo hostnamectl set-hostname pocketlab
sudo nano /etc/hosts
# Add line: 127.0.1.1    pocketlab
```

### 4. Set Up Nginx

```bash
sudo cp nginx_pocketlab.conf /etc/nginx/sites-available/pocketlab
sudo ln -s /etc/nginx/sites-available/pocketlab /etc/nginx/sites-enabled/
sudo rm /etc/nginx/sites-enabled/default
sudo nginx -t
sudo systemctl restart nginx
```

### 5. Set Up Systemd Service

```bash
sudo cp pocketlab.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable pocketlab.service
sudo systemctl start pocketlab.service
```

## Service Management

### Check Status
```bash
sudo systemctl status pocketlab
```

### View Live Logs
```bash
sudo journalctl -u pocketlab -f
```

### Restart Service
```bash
sudo systemctl restart pocketlab
```

### Stop Service
```bash
sudo systemctl stop pocketlab
```

### Start Service
```bash
sudo systemctl start pocketlab
```

## Troubleshooting

### Service won't start
```bash
# Check logs
sudo journalctl -u pocketlab -n 50 --no-pager

# Check if port 8080 is in use
sudo netstat -tulpn | grep 8080

# Test Gunicorn manually
cd ~/BASM_Pocket_Museum/SW_design/raspberry_pi_dashboard
source venv/bin/activate
gunicorn --config gunicorn_config.py app:app
```

### mDNS not working (pocketlab.local doesn't resolve)

```bash
# Restart Avahi
sudo systemctl restart avahi-daemon

# Check Avahi status
sudo systemctl status avahi-daemon

# Test from another device
ping pocketlab.local
```

### Nginx errors

```bash
# Check Nginx configuration
sudo nginx -t

# View Nginx logs
sudo tail -f /var/log/nginx/error.log
```

### Can't access from student devices

1. Make sure all devices are on the same WiFi network
2. Check Raspberry Pi IP: `hostname -I`
3. Try accessing by IP directly: `http://192.168.10.2`
4. Check firewall: `sudo ufw status` (should be inactive or allow port 80)

## Performance Tuning

### For Raspberry Pi 4 (4GB+ RAM)

Edit `/etc/systemd/system/pocketlab.service`:
```ini
Environment="WEB_CONCURRENCY=4"  # More workers
```

### For Raspberry Pi 3/Zero

Keep default settings (3 workers) or reduce to 2 if experiencing issues.

## Security Notes

- The dashboard runs on the local network only (192.168.10.x)
- No external internet access required or configured
- No HTTPS (not needed for local classroom use)
- To add HTTPS, uncomment the SSL section in `nginx_pocketlab.conf` and obtain certificates

## Reverting to Development Mode

To go back to development mode:

```bash
# Stop production service
sudo systemctl stop pocketlab
sudo systemctl disable pocketlab

# Run development server
cd ~/BASM_Pocket_Museum/SW_design/raspberry_pi_dashboard
source venv/bin/activate
python app.py
```

## Updating the Application

When you pull new code:

```bash
cd ~/BASM_Pocket_Museum/SW_design/raspberry_pi_dashboard
git pull
sudo systemctl restart pocketlab
```

## Logs Location

- Application logs: `/var/log/pocketlab/`
- Nginx access logs: `/var/log/nginx/pocketlab_access.log`
- Nginx error logs: `/var/log/nginx/pocketlab_error.log`
- Systemd journal: `journalctl -u pocketlab`
