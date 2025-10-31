# Raspberry Pi Dashboard Architecture

## System Overview

The Raspberry Pi Dashboard is a web-based interface for managing and visualizing data from multiple ESP32 "Pocket Lab" devices. The system provides real-time sensor monitoring, student notebooks, and user management.

## Technology Stack

### Backend
- **Python 3.9+**: Main programming language
- **Flask**: Web framework for REST API and web server
- **Flask-SocketIO**: WebSocket support for real-time data streaming
- **SQLite**: Lightweight database (ideal for Raspberry Pi)
- **SQLAlchemy**: ORM for database operations
- **Threading**: UDP listener runs in separate thread

### Frontend
- **HTML5/CSS3/JavaScript**: Core web technologies
- **Bootstrap 5**: Responsive UI framework
- **Chart.js**: Real-time strip charts and visualizations
- **Socket.IO Client**: WebSocket client for real-time updates
- **Fetch API**: REST API calls

### Communication
- **UDP Server (Port 5000)**: Receives JSON packets from ESP32 devices
- **WebSocket (Socket.IO)**: Real-time data push to browser clients
- **REST API**: User management, device control, questions/answers
- **HTTP/HTTPS**: Web server (default port 8080)

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                          ESP32 Devices                          │
│  (LAB_01, LAB_02, ... LAB_N) @ 25-50 Hz each                   │
│  Send JSON over UDP to 192.168.10.2:5000                       │
└────────────────────────┬────────────────────────────────────────┘
                         │ UDP JSON packets
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Raspberry Pi Server                          │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  UDP Listener Thread (Port 5000)                         │  │
│  │  - Receives JSON packets from ESP32 devices              │  │
│  │  - Parses sensor data                                    │  │
│  │  - Updates device activity status                        │  │
│  │  - Writes to database (sensor_data, devices)             │  │
│  │  - Broadcasts to WebSocket clients                       │  │
│  └─────────────┬────────────────────────────────────────────┘  │
│                │                                                │
│  ┌─────────────▼────────────────────────────────────────────┐  │
│  │  Flask Web Server (Port 8080)                            │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │  REST API Endpoints                                │  │  │
│  │  │  - /api/users (MAC ID management)                  │  │  │
│  │  │  - /api/devices (list active ESP32 devices)        │  │  │
│  │  │  - /api/sensor-data/:node_id (historical data)     │  │  │
│  │  │  - /api/preferences (user sensor preferences)      │  │  │
│  │  │  - /api/questions (teacher question management)    │  │  │
│  │  │  - /api/answers (student answers)                  │  │  │
│  │  └────────────────────────────────────────────────────┘  │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │  WebSocket (Socket.IO)                             │  │  │
│  │  │  - Real-time sensor data broadcast                 │  │  │
│  │  │  - Device status updates                           │  │  │
│  │  │  - Question notifications                          │  │  │
│  │  └────────────────────────────────────────────────────┘  │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │  Static File Server                                │  │  │
│  │  │  - Serves HTML/CSS/JS frontend                     │  │  │
│  │  └────────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  SQLite Database (dashboard.db)                          │  │
│  │  - users (MAC ID registration)                           │  │
│  │  - devices (ESP32 tracking)                              │  │
│  │  - sensor_data (time-series data)                        │  │
│  │  - questions (teacher questions)                         │  │
│  │  - answers (student responses)                           │  │
│  │  - user_preferences (sensor display settings)            │  │
│  └──────────────────────────────────────────────────────────┘  │
└────────────────────────┬────────────────────────────────────────┘
                         │ WebSocket + HTTP
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Browser Clients                            │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Frontend Application (SPA)                              │  │
│  │  ┌────────────────┐ ┌────────────────┐ ┌──────────────┐ │  │
│  │  │  Welcome Tab   │ │ Pocket Lab     │ │  Notebook    │ │  │
│  │  │  - MAC ID reg  │ │ Data Tab       │ │  Tab         │ │  │
│  │  │  - User greet  │ │ - Device list  │ │  - Questions │ │  │
│  │  │                │ │ - Sensor cards │ │  - Answers   │ │  │
│  │  │                │ │ - Strip charts │ │              │ │  │
│  │  └────────────────┘ └────────────────┘ └──────────────┘ │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow

### 1. ESP32 → Raspberry Pi (Sensor Data Ingestion)
```
ESP32 Device
  ↓ (UDP JSON @ 25-50 Hz)
UDP Listener (Port 5000)
  ↓ Parse JSON
  ↓ Extract node_id, sensor values
  ↓ Update device last_seen timestamp
  ↓ Store in database (sensor_data table)
  ↓ Broadcast via WebSocket
Browser Clients (real-time update)
```

### 2. Browser → Raspberry Pi (User Registration)
```
Browser (Welcome Tab)
  ↓ POST /api/users/register
  ↓ {mac_id, name, grade, science_interest}
Flask API
  ↓ Check if user exists
  ↓ Create new user or return existing
  ↓ Store in database (users table)
  ↓ Return user profile
Browser (display welcome message)
```

### 3. Browser → Raspberry Pi (Device & Sensor Selection)
```
Browser (Pocket Lab Data Tab)
  ↓ GET /api/devices (fetch active devices)
Flask API
  ↓ Query devices with last_seen > (now - timeout)
  ↓ Return list of active devices
Browser (display device rectangles)
  ↓ User clicks device
  ↓ User selects sensors & display types
  ↓ POST /api/preferences
  ↓ {mac_id, node_id, sensor_type, display_type}
Flask API
  ↓ Store preferences in database
  ↓ Return success
Browser
  ↓ Subscribe to WebSocket for selected node
  ↓ GET /api/sensor-data/:node_id (historical data)
  ↓ Render strip charts / numeric displays
  ↓ Receive real-time updates via WebSocket
```

### 4. Browser → Raspberry Pi (Notebook Questions/Answers)
```
Teacher:
  Browser (admin interface)
    ↓ POST /api/questions
    ↓ {question_text, question_type, options}
  Flask API
    ↓ Store in database (questions table)
    ↓ Broadcast new question notification
  All browsers (receive notification)

Student:
  Browser (Notebook Tab)
    ↓ GET /api/questions (fetch active questions)
    ↓ Display questions
    ↓ User submits answer
    ↓ POST /api/answers
    ↓ {mac_id, question_id, answer}
  Flask API
    ↓ Check if already answered (UNIQUE constraint)
    ↓ Store in database (answers table)
    ↓ Return success or error
```

## Key Components

### 1. UDP Listener (`udp_listener.py`)
- Runs in separate thread
- Listens on port 5000
- Parses incoming JSON packets
- Updates device activity
- Stores sensor data
- Broadcasts to WebSocket clients
- Handles multiple ESP32 devices concurrently

### 2. Flask Application (`app.py`)
- Main web server
- REST API endpoints
- WebSocket server (Socket.IO)
- Static file serving
- Database initialization
- CORS configuration

### 3. Database Models (`models.py`)
- User (MAC ID, name, grade, science_interest)
- Device (node_id, hostname, last_seen, is_active)
- SensorData (time-series sensor readings)
- Question (teacher questions)
- Answer (student responses)
- UserPreference (sensor display preferences)

### 4. Frontend (`static/` directory)
- `index.html`: Main application shell
- `css/styles.css`: Styling
- `js/app.js`: Main application logic
- `js/welcome.js`: Welcome tab
- `js/data-tab.js`: Pocket Lab Data tab
- `js/notebook.js`: Notebook tab
- `js/charts.js`: Chart.js utilities
- `js/websocket.js`: Socket.IO client

## Device Activity Tracking

### Timeout Logic
- Devices are considered **ACTIVE** if `last_seen > (now - ACTIVITY_TIMEOUT)`
- Default timeout: **5 seconds** (configurable)
- UDP Listener updates `last_seen` on every packet
- Frontend polls `/api/devices` every 2 seconds
- Inactive devices are hidden from the UI

### Hostname Resolution
- ESP32 sends `node_id` in JSON (e.g., "LAB_01")
- Hostname (e.g., "ESP32-01") can be inferred from node_id or stored separately
- Database stores both `node_id` (unique identifier) and `hostname` (display name)

## Security Considerations

### MAC Address Privacy
- MAC addresses are hashed with SHA-256 before storage
- Hash is used as user identifier
- Original MAC address not stored in database
- Frontend sends hash, not raw MAC address

### Authentication (Future Enhancement)
- Current version: Open access (suitable for classroom)
- Future: Teacher admin panel with password protection
- Future: Student login with PIN or QR code

### Data Retention
- Sensor data older than 24 hours automatically pruned
- Configurable retention policy
- Questions can be archived (soft delete)

## Configuration

### Environment Variables (`.env` file)
```
FLASK_ENV=production
FLASK_DEBUG=False
DATABASE_URL=sqlite:///dashboard.db
UDP_PORT=5000
WEB_PORT=8080
ACTIVITY_TIMEOUT=5
SENSOR_DATA_RETENTION_HOURS=24
SECRET_KEY=<random-secret-key>
```

## Deployment on Raspberry Pi

### System Requirements
- Raspberry Pi 4 (2GB+ RAM recommended)
- Raspberry Pi OS (Debian-based)
- Python 3.9+
- 10GB+ available storage

### Installation Steps
```bash
# Clone repository
cd /home/pi/
git clone <repository-url> BASM_Dashboard

# Install dependencies
cd BASM_Dashboard/raspberry_pi_dashboard
pip3 install -r requirements.txt

# Initialize database
python3 init_db.py

# Run application
python3 app.py
```

### Systemd Service (Auto-start on boot)
```bash
sudo cp dashboard.service /etc/systemd/system/
sudo systemctl enable dashboard
sudo systemctl start dashboard
```

### Network Configuration
- Static IP: 192.168.10.2 (matches ESP32 SERVER_IP_ADDRESS)
- Subnet: 192.168.10.0/24
- Router: Configure DHCP reservations for ESP32 devices

## Performance Considerations

### Database Optimization
- Index on `node_id`, `timestamp`, `mac_id`
- Periodic VACUUM to reclaim space
- Write-ahead logging (WAL) mode for concurrent reads

### WebSocket Optimization
- Broadcast only to subscribed rooms (per-device)
- Throttle broadcast rate (max 10 Hz per sensor)
- Client-side data buffering

### Memory Management
- Limit sensor_data table size (auto-prune old data)
- Circular buffer for real-time data (in-memory)
- Connection pooling for database

## Testing Strategy

### Unit Tests
- Database models (CRUD operations)
- API endpoints (REST)
- UDP packet parsing
- WebSocket message handling

### Integration Tests
- End-to-end data flow (ESP32 → DB → Browser)
- Multi-device scenarios
- Concurrent user access

### Load Testing
- Simulate 10+ ESP32 devices @ 50 Hz each
- Measure latency, CPU, memory usage
- Identify bottlenecks

## Future Enhancements

1. **Data Export**: CSV/Excel export of sensor data
2. **Advanced Analytics**: Statistical analysis, data trends
3. **Mobile App**: Native iOS/Android app
4. **Cloud Sync**: Backup to cloud storage
5. **Multi-language Support**: Internationalization (i18n)
6. **Custom Dashboards**: User-configurable layouts
7. **Alerts & Notifications**: Threshold-based alerts
8. **Spectral Sensor Visualization**: Full spectrum charts (AS7343)
9. **Video Recording**: Sync sensor data with video timestamps
10. **Collaborative Notes**: Shared student notebooks
