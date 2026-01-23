#!/bin/bash
# Production Setup Script for Pocket Lab Dashboard
# This script configures the Raspberry Pi for production deployment

set -e  # Exit on error

echo "========================================="
echo "Pocket Lab Dashboard - Production Setup"
echo "========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DASHBOARD_DIR="$SCRIPT_DIR"

echo -e "${GREEN}Step 1: Installing system dependencies...${NC}"
sudo apt-get update
sudo apt-get install -y nginx avahi-daemon python3-pip python3-venv

echo ""
echo -e "${GREEN}Step 2: Setting up Python virtual environment...${NC}"
cd "$DASHBOARD_DIR"
if [ ! -d "venv" ]; then
    python3 -m venv venv
    echo "Virtual environment created"
fi

source venv/bin/activate

echo ""
echo -e "${GREEN}Step 3: Installing Python dependencies...${NC}"
pip install --upgrade pip
pip install gunicorn eventlet flask flask-cors flask-socketio python-socketio sqlalchemy

echo ""
echo -e "${GREEN}Step 4: Creating log directories...${NC}"
sudo mkdir -p /var/log/pocketlab
sudo mkdir -p /var/run/pocketlab
sudo chown -R $USER:$USER /var/log/pocketlab
sudo chown -R $USER:$USER /var/run/pocketlab

echo ""
echo -e "${GREEN}Step 5: Configuring mDNS/Avahi (pocketlab.local)...${NC}"
# Enable and start Avahi daemon
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon

# Set hostname to pocketlab
CURRENT_HOSTNAME=$(hostname)
if [ "$CURRENT_HOSTNAME" != "pocketlab" ]; then
    echo -e "${YELLOW}Current hostname: $CURRENT_HOSTNAME${NC}"
    read -p "Change hostname to 'pocketlab'? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo hostnamectl set-hostname pocketlab
        echo "127.0.1.1    pocketlab" | sudo tee -a /etc/hosts
        echo -e "${GREEN}Hostname changed to 'pocketlab'${NC}"
        echo -e "${YELLOW}You may need to reboot for this to take full effect${NC}"
    fi
fi

echo ""
echo -e "${GREEN}Step 6: Configuring Nginx...${NC}"
# Copy Nginx configuration
sudo cp "$DASHBOARD_DIR/nginx_pocketlab.conf" /etc/nginx/sites-available/pocketlab

# Update the path in Nginx config to use current user's home directory
sudo sed -i "s|/home/pi|$HOME|g" /etc/nginx/sites-available/pocketlab

# Remove default site and enable pocketlab site
sudo rm -f /etc/nginx/sites-enabled/default
sudo ln -sf /etc/nginx/sites-available/pocketlab /etc/nginx/sites-enabled/pocketlab

# Test Nginx configuration
sudo nginx -t

# Reload Nginx
sudo systemctl enable nginx
sudo systemctl restart nginx

echo ""
echo -e "${GREEN}Step 7: Configuring systemd service...${NC}"
# Copy systemd service file
sudo cp "$DASHBOARD_DIR/pocketlab.service" /etc/systemd/system/pocketlab.service

# Update the path in service file to use current user and directory
sudo sed -i "s|/home/pi|$HOME|g" /etc/systemd/system/pocketlab.service
sudo sed -i "s|User=pi|User=$USER|g" /etc/systemd/system/pocketlab.service
sudo sed -i "s|Group=pi|Group=$USER|g" /etc/systemd/system/pocketlab.service

# Reload systemd
sudo systemctl daemon-reload

# Enable and start the service
sudo systemctl enable pocketlab.service
sudo systemctl start pocketlab.service

echo ""
echo -e "${GREEN}Step 8: Checking service status...${NC}"
sleep 2
sudo systemctl status pocketlab.service --no-pager

echo ""
echo "========================================="
echo -e "${GREEN}✓ Production setup complete!${NC}"
echo "========================================="
echo ""
echo "Access your dashboard at:"
echo -e "${GREEN}  http://pocketlab.local${NC}"
echo -e "${GREEN}  http://pocketlab${NC}"
echo -e "${GREEN}  http://$(hostname -I | awk '{print $1}')${NC}"
echo ""
echo "Useful commands:"
echo "  sudo systemctl status pocketlab    # Check service status"
echo "  sudo systemctl restart pocketlab   # Restart service"
echo "  sudo systemctl stop pocketlab      # Stop service"
echo "  sudo journalctl -u pocketlab -f    # View logs"
echo ""
echo -e "${YELLOW}Note: If you changed the hostname, please reboot for mDNS to work properly:${NC}"
echo "  sudo reboot"
