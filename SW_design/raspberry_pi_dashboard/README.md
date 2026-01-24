# BASM Pocket Museum Dashboard

A web-based dashboard for the Raspberry Pi that receives sensor data from ESP32 "Pocket Lab" devices and provides real-time visualization, student notebooks, and user management.

## Features

### 1. Welcome Tab
- First-time user registration (name, grade, science interest)
- Returning user greeting ("Welcome back, [Name]!")
- MAC address-based identification (hashed for privacy)

### 2. Pocket Lab Data Tab
- Real-time display of active ESP32 devices
- Interactive sensor selection (checkboxes for each sensor)
- Flexible display modes: stripchart, numeric, both, or none
- Supports all sensors:
  - Temperature & Pressure
  - Ambient Light
  - Accelerometer (X, Y, Z, Norm)
  - Gyroscope (X, Y, Z, Norm)
  - Magnetometer (X, Y, Z, Norm)
  - Volume (RMS)
  - Spectral sensor (14 channels)

### 3. Pocket Lab Notebook Tab
- Teacher-published questions (multiple choice or free text)
- Student answer submission (one answer per question)
- Answer history tracking

## Architecture

### Development Mode
- **Backend**: Python Flask + Flask-SocketIO
- **Database**: SQLite (lightweight, perfect for Raspberry Pi)
- **Communication**: UDP (port 5000) for ESP32 data, WebSocket for real-time browser updates
- **Frontend**: HTML/CSS/JavaScript

### Production Mode (Multi-Service Architecture)
The production setup runs three separate services that communicate via Redis:

1. **Web Server (`pocketlab.service`)**
   - Gunicorn WSGI server with eventlet workers
   - Serves HTTP API and WebSocket connections
   - Listens on `127.0.0.1:8080` (proxied by Nginx)

2. **UDP Listener (`pocketlab-listener.service`)**
   - Standalone service receiving ESP32 sensor data via UDP
   - Saves data to SQLite database
   - Broadcasts sensor updates via Redis message queue

3. **Redis Server (`redis-server.service`)**
   - Message queue enabling WebSocket communication between services
   - Allows UDP listener broadcasts to reach web clients
   - Essential for multi-process/multi-service architecture

4. **Nginx (Reverse Proxy)**
   - Serves frontend assets
   - Proxies WebSocket and HTTP requests to Gunicorn
   - Enables mDNS access via `http://pocketlab.local`

**Why Redis?** In production, the web server runs with multiple Gunicorn workers (separate processes), and the UDP listener runs as an independent service. Without Redis, WebSocket messages from the UDP listener wouldn't reach clients connected to the web server. Redis acts as a message broker, synchronizing WebSocket events across all processes.

## Installation

### Prerequisites

- Raspberry Pi 4 (2GB+ RAM recommended)
- Raspberry Pi OS (Debian-based)
- Python 3.9+
- Static IP configured: 192.168.10.2

### Step 1: Clone Repository

```bash
cd /home/pi/
git clone <repository-url> BASM_Pocket_Museum
cd BASM_Pocket_Museum/SW_design/raspberry_pi_dashboard
```

### Step 2: Install Dependencies

```bash
pip3 install -r requirements.txt
```

### Step 3: Configure Environment

```bash
cp .env.example .env
nano .env
```

Edit `.env` to customize settings (or use defaults):

```env
FLASK_ENV=production
SECRET_KEY=<generate-random-secret-key>
UDP_PORT=5000
WEB_PORT=8080
ACTIVITY_TIMEOUT_SECONDS=5
SENSOR_DATA_RETENTION_HOURS=24
```

### Step 4: Initialize Database

```bash
python3 init_db.py
```

You should see:
```
✓ Database initialized successfully!
✓ Schema version 1 applied
```

### Step 5: Run the Application

#### Development Mode
```bash
python3 app.py
```
This runs both the web server and UDP listener in a single process.

#### Production Mode (Recommended)
Use the automated setup script:
```bash
chmod +x setup_production.sh
./setup_production.sh
```

This script will:
1. Install system dependencies (Nginx, Redis, Avahi, Python3)
2. Create Python virtual environment
3. Install Python dependencies
4. Configure mDNS (hostname: pocketlab.local)
5. Configure Nginx reverse proxy
6. Set up Redis message queue
7. Install and start systemd services
8. Verify all services are running

After setup completes, access the dashboard at:
- `http://pocketlab.local`
- `http://pocketlab`
- `http://<raspberry-pi-ip>`

**Service Management**
```bash
# Check service status
sudo systemctl status redis-server
sudo systemctl status pocketlab
sudo systemctl status pocketlab-listener

# Restart services
sudo systemctl restart pocketlab
sudo systemctl restart pocketlab-listener

# View logs
sudo journalctl -u pocketlab -f
sudo journalctl -u pocketlab-listener -f

# Test Redis connection
redis-cli ping  # Should return PONG
```

## Configuration

### Network Setup

The Raspberry Pi must have a static IP address that matches the ESP32 configuration:

**Raspberry Pi**: 192.168.10.2
**ESP32 Devices**: 192.168.10.11, 192.168.10.12, etc.

Edit `/etc/dhcpcd.conf`:
```
interface eth0
static ip_address=192.168.10.2/24
static routers=192.168.10.1
static domain_name_servers=8.8.8.8
```

Restart networking:
```bash
sudo systemctl restart dhcpcd
```

### Firewall

Allow incoming UDP on port 5000 and HTTP on port 8080:

```bash
sudo ufw allow 5000/udp
sudo ufw allow 8080/tcp
```

## API Documentation

### REST API Endpoints

#### Users

**Register/Login User**
```http
POST /api/users/register
Content-Type: application/json

{
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "name": "John Doe",
  "grade": "10th",
  "science_interest": "Physics and robotics"
}
```

**Get User**
```http
GET /api/users/<mac_id>
```

#### Devices

**Get Active Devices**
```http
GET /api/devices
```

**Get Specific Device**
```http
GET /api/devices/<node_id>
```

#### Sensor Data

**Get Sensor Data**
```http
GET /api/sensor-data/<node_id>?limit=1000&hours=1&sensors=temperature,pressure
```

Parameters:
- `limit`: Max records (default: 1000)
- `hours`: Data from last N hours (default: 1)
- `sensors`: Comma-separated sensor list (optional)

#### Questions

**Get Active Questions**
```http
GET /api/questions
```

**Create Question (Teacher)**
```http
POST /api/questions
Content-Type: application/json

{
  "question_text": "What is the acceleration due to gravity?",
  "question_type": "multiple_choice",
  "options": ["9.8 m/s²", "10 m/s²", "8.9 m/s²", "11.2 m/s²"],
  "created_by": "Mrs. Smith"
}
```

**Delete Question**
```http
DELETE /api/questions/<question_id>
```

#### Answers

**Submit Answer**
```http
POST /api/answers
Content-Type: application/json

{
  "question_id": 1,
  "mac_id": "<hashed_mac_id>",
  "answer": "9.8 m/s²"
}
```

**Get User Answers**
```http
GET /api/answers/user/<mac_id>
```

**Get Question Answers (Teacher)**
```http
GET /api/answers/question/<question_id>
```

#### Preferences

**Get Preferences**
```http
GET /api/preferences?mac_id=<mac_id>&node_id=<node_id>
```

**Save Preferences**
```http
POST /api/preferences
Content-Type: application/json

{
  "mac_id": "<mac_id>",
  "node_id": "LAB_01",
  "sensor_type": "temperature",
  "display_type": "both"
}
```

Display types: `stripchart`, `numeric`, `both`, `none`

### WebSocket Events

**Connect**
```javascript
const socket = io('http://192.168.10.2:8080');
```

**Subscribe to Device**
```javascript
socket.emit('subscribe_device', {node_id: 'LAB_01'});
```

**Receive Sensor Data**
```javascript
socket.on('sensor_data', (data) => {
  console.log(data.node_id, data.data);
});
```

**Receive New Question**
```javascript
socket.on('new_question', (question) => {
  console.log('New question:', question);
});
```

## Maintenance

### Automatic Cleanup

Add to crontab for hourly cleanup:

```bash
crontab -e
```

Add line:
```
0 * * * * /usr/bin/python3 /home/pi/BASM_Pocket_Museum/raspberry_pi_dashboard/cleanup_sensor_data.py >> /var/log/basm-cleanup.log 2>&1
```

### Manual Cleanup

```bash
python3 cleanup_sensor_data.py
```

### Database Backup

```bash
# Create backup
cp dashboard.db dashboard_backup_$(date +%Y%m%d).db

# Or use SQLite backup command
sqlite3 dashboard.db ".backup /path/to/backup.db"
```

### View Database

```bash
sqlite3 dashboard.db

# Useful queries
SELECT COUNT(*) FROM sensor_data;
SELECT node_id, MAX(timestamp) FROM sensor_data GROUP BY node_id;
SELECT * FROM users;
```

## Troubleshooting

### No ESP32 Data Received

1. **Check UDP listener is running**:
```bash
sudo systemctl status pocketlab-listener
# Should show "active (running)"
```

2. **Check ESP32 is transmitting**:
```bash
# Listen on UDP port 5000
nc -ul 5000
# You should see JSON packets from ESP32
```

3. **Check Redis is running**:
```bash
sudo systemctl status redis-server
redis-cli ping  # Should return PONG
```

4. **Check UDP listener logs**:
```bash
sudo journalctl -u pocketlab-listener -f
# Look for "UDP listener started on port 5000"
# Look for packet reception logs
```

5. **Check web server logs**:
```bash
sudo journalctl -u pocketlab -f
# Look for WebSocket connection logs
```

### WebSocket Data Not Updating in Browser

This usually indicates Redis message queue issues:

1. **Verify Redis is running and accessible**:
```bash
sudo systemctl status redis-server
redis-cli ping  # Should return PONG
```

2. **Check both services are using Redis**:
```bash
sudo journalctl -u pocketlab -n 50 | grep -i redis
sudo journalctl -u pocketlab-listener -n 50 | grep -i redis
# Look for connection messages
```

3. **Restart services in order**:
```bash
sudo systemctl restart redis-server
sudo systemctl restart pocketlab-listener
sudo systemctl restart pocketlab
```

4. **Check for Redis connection errors**:
```bash
redis-cli client list
# Should show connections from both services
```

### Database Errors

1. Check permissions:
```bash
ls -la dashboard.db
# Should be writable by the user running the service
```

2. Re-initialize:
```bash
rm dashboard.db
python3 init_db.py
```

### High CPU/Memory Usage

1. Check sensor data table size:
```bash
sqlite3 dashboard.db "SELECT COUNT(*) FROM sensor_data;"
```

2. Run cleanup:
```bash
python3 cleanup_sensor_data.py
```

3. Reduce retention hours in `.env`:
```env
SENSOR_DATA_RETENTION_HOURS=12
```

## Development

### Run in Development Mode

```bash
export FLASK_ENV=development
export FLASK_DEBUG=True
python3 app.py
```

### Testing

```bash
# Test UDP listener
python3 -c "import socket, json; s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.sendto(json.dumps({'node_id': 'LAB_TEST', 'temperature': 23.5}).encode(), ('127.0.0.1', 5000))"

# Test REST API
curl http://localhost:8080/api/devices

# Test WebSocket
# Use browser console or tool like wscat
```

## Next Steps

- [ ] Implement frontend UI (HTML/CSS/JavaScript)
- [ ] Add authentication for teacher admin panel
- [ ] Implement data export (CSV/Excel)
- [ ] Add advanced visualizations (spectral charts)
- [ ] Mobile responsive design

## License

[Your License Here]

## Contact

[Your Contact Information]
