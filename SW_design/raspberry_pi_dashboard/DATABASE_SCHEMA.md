# Database Schema

## Overview

The dashboard uses **SQLite** as the database engine. SQLite is lightweight, serverless, and perfect for Raspberry Pi deployment. The schema is designed for:
- Fast reads (real-time data access)
- Efficient writes (25-50 Hz data ingestion per ESP32)
- Data integrity (foreign keys, unique constraints)
- Automatic cleanup (TTL for sensor data)

## Schema Diagram

```
┌──────────────┐
│    users     │
│──────────────│
│ mac_id (PK)  │◄────────┐
│ name         │          │
│ grade        │          │
│ science_int  │          │
│ first_seen   │          │
│ last_seen    │          │
└──────────────┘          │
                          │
┌──────────────┐          │
│   devices    │          │
│──────────────│          │
│ node_id (PK) │◄───┐     │
│ hostname     │    │     │
│ ip_address   │    │     │
│ last_seen    │    │     │
│ is_active    │    │     │
└──────────────┘    │     │
                    │     │
┌──────────────┐    │     │
│ sensor_data  │    │     │
│──────────────│    │     │
│ id (PK)      │    │     │
│ node_id (FK) │────┘     │
│ timestamp    │          │
│ temperature  │          │
│ pressure     │          │
│ light_int    │          │
│ accel_x/y/z  │          │
│ gyro_x/y/z   │          │
│ mag_x/y/z    │          │
│ volume_rms   │          │
│ ... etc      │          │
└──────────────┘          │
                          │
┌──────────────┐          │
│  questions   │          │
│──────────────│          │
│ id (PK)      │◄───┐     │
│ question_txt │    │     │
│ type         │    │     │
│ options      │    │     │
│ created_at   │    │     │
│ is_active    │    │     │
└──────────────┘    │     │
                    │     │
┌──────────────┐    │     │
│   answers    │    │     │
│──────────────│    │     │
│ id (PK)      │    │     │
│ question_id  │────┘     │
│ mac_id (FK)  │──────────┘
│ answer       │
│ answered_at  │
│ UNIQUE(q,m)  │
└──────────────┘

┌──────────────┐
│ user_prefs   │
│──────────────│
│ id (PK)      │
│ mac_id (FK)  │──────────┘
│ node_id (FK) │──────────┘
│ sensor_type  │
│ display_type │
│ created_at   │
│ updated_at   │
└──────────────┘
```

## Tables

### 1. `users` - User Registration & Profiles

Stores student/user information collected on first visit.

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `mac_id` | VARCHAR(64) | PRIMARY KEY | SHA-256 hash of MAC address |
| `name` | VARCHAR(100) | NOT NULL | User's name |
| `grade` | VARCHAR(20) | NULL | Grade level (e.g., "9th", "10th") |
| `science_interest` | TEXT | NULL | Interest in science (free text) |
| `first_seen` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | First visit timestamp |
| `last_seen` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | Most recent visit |

**Indexes:**
- PRIMARY KEY on `mac_id`
- INDEX on `last_seen` (for recent user queries)

**Example Data:**
```sql
INSERT INTO users VALUES (
  'a3f5b8c9d2e1f4g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6a7b8c9d0e1f2',
  'Raul Popescu',
  '10th',
  'I love physics and robotics!',
  '2025-10-31 09:15:23',
  '2025-10-31 14:32:11'
);
```

---

### 2. `devices` - ESP32 Device Tracking

Tracks all ESP32 "Pocket Lab" devices that have ever connected.

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `node_id` | VARCHAR(50) | PRIMARY KEY | Node ID from ESP32 (e.g., "LAB_01") |
| `hostname` | VARCHAR(100) | NULL | Network hostname (e.g., "ESP32-01") |
| `ip_address` | VARCHAR(15) | NULL | Last known IP address |
| `last_seen` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | Last data packet received |
| `is_active` | BOOLEAN | DEFAULT TRUE | Computed: last_seen > (now - timeout) |

**Indexes:**
- PRIMARY KEY on `node_id`
- INDEX on `last_seen` (for active device queries)
- INDEX on `is_active, last_seen` (composite index for active filtering)

**Example Data:**
```sql
INSERT INTO devices VALUES (
  'LAB_01',
  'ESP32-01',
  '192.168.10.11',
  '2025-10-31 14:32:15',
  TRUE
);
```

**Notes:**
- `is_active` is computed on read: `last_seen > (CURRENT_TIMESTAMP - ACTIVITY_TIMEOUT)`
- Devices are never deleted, only marked inactive
- `hostname` extracted from initial connection or inferred from `node_id`

---

### 3. `sensor_data` - Time-Series Sensor Readings

Stores all sensor data from ESP32 devices. This is the largest table.

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique record ID |
| `node_id` | VARCHAR(50) | FOREIGN KEY → devices(node_id) | Source device |
| `timestamp` | TIMESTAMP | NOT NULL | Server receive time |
| `esp_timestamp` | INTEGER | NULL | ESP32 millis() value |
| `temperature` | REAL | NULL | Temperature (°C) |
| `pressure` | REAL | NULL | Pressure (Pa) |
| `light_intensity` | REAL | NULL | Ambient light (lux) |
| `accel_x` | REAL | NULL | Acceleration X (mg) |
| `accel_y` | REAL | NULL | Acceleration Y (mg) |
| `accel_z` | REAL | NULL | Acceleration Z (mg) |
| `accel_norm` | REAL | NULL | Acceleration norm (mg) |
| `gyro_x` | REAL | NULL | Gyroscope X (deg/s) |
| `gyro_y` | REAL | NULL | Gyroscope Y (deg/s) |
| `gyro_z` | REAL | NULL | Gyroscope Z (deg/s) |
| `gyro_norm` | REAL | NULL | Gyroscope norm (deg/s) |
| `mag_x` | REAL | NULL | Magnetometer X (µT) |
| `mag_y` | REAL | NULL | Magnetometer Y (µT) |
| `mag_z` | REAL | NULL | Magnetometer Z (µT) |
| `mag_norm` | REAL | NULL | Magnetometer norm (µT) |
| `volume_rms` | REAL | NULL | Sound level (dB) |
| `spectral_f1_405nm` | REAL | NULL | Spectral channel F1 (405nm) |
| `spectral_f2_425nm` | REAL | NULL | Spectral channel F2 (425nm) |
| `spectral_f3_475nm` | REAL | NULL | Spectral channel F3 (475nm) |
| `spectral_f4_515nm` | REAL | NULL | Spectral channel F4 (515nm) |
| `spectral_fz_450nm` | REAL | NULL | Spectral channel FZ (450nm) |
| `spectral_fy_555nm` | REAL | NULL | Spectral channel FY (555nm) |
| `spectral_f5_550nm` | REAL | NULL | Spectral channel F5 (550nm) |
| `spectral_f6_640nm` | REAL | NULL | Spectral channel F6 (640nm) |
| `spectral_fxl_600nm` | REAL | NULL | Spectral channel FXL (600nm) |
| `spectral_f7_690nm` | REAL | NULL | Spectral channel F7 (690nm) |
| `spectral_f8_745nm` | REAL | NULL | Spectral channel F8 (745nm) |
| `spectral_nir_855nm` | REAL | NULL | Spectral channel NIR (855nm) |
| `spectral_vis` | REAL | NULL | Spectral visible light |
| `spectral_fd` | REAL | NULL | Spectral flicker detection |

**Indexes:**
- PRIMARY KEY on `id`
- INDEX on `node_id` (for per-device queries)
- INDEX on `timestamp` (for time-range queries)
- COMPOSITE INDEX on `node_id, timestamp DESC` (most common query pattern)

**Constraints:**
- FOREIGN KEY `node_id` REFERENCES `devices(node_id)` ON DELETE CASCADE
- CHECK: `timestamp >= '2025-01-01'` (prevent garbage data)

**Example Data:**
```sql
INSERT INTO sensor_data (node_id, timestamp, esp_timestamp, temperature, pressure, light_intensity, accel_x, accel_y, accel_z, accel_norm) VALUES (
  'LAB_01',
  '2025-10-31 14:32:15.123',
  123456789,
  23.45,
  101325.0,
  542.1,
  0.012,
  -0.005,
  1.003,
  1.004
);
```

**Data Retention Policy:**
- Automatic deletion of records older than `SENSOR_DATA_RETENTION_HOURS` (default: 24 hours)
- Implemented via cron job or periodic background task
- Example cleanup query:
```sql
DELETE FROM sensor_data
WHERE timestamp < datetime('now', '-24 hours');
```

**Storage Estimate:**
- ~200 bytes per record (with all sensors)
- 10 devices @ 50 Hz = 500 records/sec = 43M records/day
- 43M × 200 bytes = 8.6 GB/day (if all sensors active)
- With 24-hour retention: ~10 GB database size (worst case)
- Realistic: ~2-3 GB (sparse sensor data, lower rates)

---

### 4. `questions` - Teacher Questions for Notebook

Stores questions published by teachers for students to answer.

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique question ID |
| `question_text` | TEXT | NOT NULL | The question text |
| `question_type` | VARCHAR(20) | NOT NULL | 'multiple_choice' or 'free_text' |
| `options` | TEXT | NULL | JSON array of options (for multiple choice) |
| `created_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | When question was published |
| `is_active` | BOOLEAN | DEFAULT TRUE | Active questions shown to students |
| `created_by` | VARCHAR(100) | NULL | Teacher name (future: teacher_id FK) |

**Indexes:**
- PRIMARY KEY on `id`
- INDEX on `is_active, created_at DESC` (for active question listing)

**Example Data:**

Multiple Choice:
```sql
INSERT INTO questions (question_text, question_type, options) VALUES (
  'What is the acceleration due to gravity on Earth?',
  'multiple_choice',
  '["9.8 m/s²", "10 m/s²", "8.9 m/s²", "11.2 m/s²"]'
);
```

Free Text:
```sql
INSERT INTO questions (question_text, question_type) VALUES (
  'Describe what happens to air pressure as you increase altitude.',
  'free_text',
  NULL
);
```

**Notes:**
- `options` is stored as JSON string
- Only shown to students if `is_active = TRUE`
- Teachers can archive questions (set `is_active = FALSE`)

---

### 5. `answers` - Student Answers to Questions

Stores student responses to questions. Students can only answer each question once.

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique answer ID |
| `question_id` | INTEGER | FOREIGN KEY → questions(id) | Which question |
| `mac_id` | VARCHAR(64) | FOREIGN KEY → users(mac_id) | Which student |
| `answer` | TEXT | NOT NULL | Student's answer |
| `answered_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | When answered |

**Indexes:**
- PRIMARY KEY on `id`
- UNIQUE INDEX on `(question_id, mac_id)` (prevent duplicate answers)
- INDEX on `question_id` (for per-question answer listing)
- INDEX on `mac_id` (for per-student answer history)

**Constraints:**
- FOREIGN KEY `question_id` REFERENCES `questions(id)` ON DELETE CASCADE
- FOREIGN KEY `mac_id` REFERENCES `users(mac_id)` ON DELETE CASCADE
- UNIQUE (`question_id`, `mac_id`) - prevents answering same question twice

**Example Data:**
```sql
INSERT INTO answers (question_id, mac_id, answer) VALUES (
  1,
  'a3f5b8c9d2e1f4g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6a7b8c9d0e1f2',
  '9.8 m/s²',
  '2025-10-31 14:35:22'
);
```

**Error Handling:**
- If student tries to answer same question twice → return 409 Conflict
- Frontend disables "Submit" button for already-answered questions

---

### 6. `user_preferences` - Sensor Display Preferences

Stores user preferences for which sensors to display and how (stripchart, numeric, both, none).

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique preference ID |
| `mac_id` | VARCHAR(64) | FOREIGN KEY → users(mac_id) | Which user |
| `node_id` | VARCHAR(50) | FOREIGN KEY → devices(node_id) | Which device |
| `sensor_type` | VARCHAR(50) | NOT NULL | Sensor name (e.g., 'temperature', 'accel_x') |
| `display_type` | VARCHAR(20) | NOT NULL | 'stripchart', 'numeric', 'both', 'none' |
| `created_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | When preference created |
| `updated_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | Last updated |

**Indexes:**
- PRIMARY KEY on `id`
- UNIQUE INDEX on `(mac_id, node_id, sensor_type)` (one preference per user/device/sensor combo)
- INDEX on `mac_id` (for fetching user's preferences)

**Constraints:**
- FOREIGN KEY `mac_id` REFERENCES `users(mac_id)` ON DELETE CASCADE
- FOREIGN KEY `node_id` REFERENCES `devices(node_id)` ON DELETE CASCADE
- UNIQUE (`mac_id`, `node_id`, `sensor_type`)
- CHECK: `display_type IN ('stripchart', 'numeric', 'both', 'none')`
- CHECK: `sensor_type IN ('temperature', 'pressure', 'light_intensity', 'accel_x', 'accel_y', 'accel_z', 'accel_norm', 'gyro_x', 'gyro_y', 'gyro_z', 'gyro_norm', 'mag_x', 'mag_y', 'mag_z', 'mag_norm', 'volume_rms', ...)`

**Example Data:**
```sql
INSERT INTO user_preferences (mac_id, node_id, sensor_type, display_type) VALUES (
  'a3f5b8c9d2e1f4g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6a7b8c9d0e1f2',
  'LAB_01',
  'temperature',
  'both'  -- Show both stripchart AND numeric
);

INSERT INTO user_preferences (mac_id, node_id, sensor_type, display_type) VALUES (
  'a3f5b8c9d2e1f4g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6a7b8c9d0e1f2',
  'LAB_01',
  'accel_norm',
  'stripchart'  -- Show only stripchart
);
```

**Usage Flow:**
1. User selects device (LAB_01)
2. User checks "Temperature - Stripchart" and "Temperature - Numeric"
3. Frontend sends: `{mac_id, node_id: 'LAB_01', sensor_type: 'temperature', display_type: 'both'}`
4. Backend stores in `user_preferences`
5. Next time user selects LAB_01, preferences are loaded and checkboxes pre-populated

---

## SQL Schema (SQLite)

```sql
-- Enable foreign keys (must run on each connection)
PRAGMA foreign_keys = ON;

-- Users table
CREATE TABLE users (
    mac_id VARCHAR(64) PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    grade VARCHAR(20),
    science_interest TEXT,
    first_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_users_last_seen ON users(last_seen);

-- Devices table
CREATE TABLE devices (
    node_id VARCHAR(50) PRIMARY KEY,
    hostname VARCHAR(100),
    ip_address VARCHAR(15),
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE
);

CREATE INDEX idx_devices_last_seen ON devices(last_seen);
CREATE INDEX idx_devices_active ON devices(is_active, last_seen);

-- Sensor data table (time-series)
CREATE TABLE sensor_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    node_id VARCHAR(50) NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    esp_timestamp INTEGER,
    temperature REAL,
    pressure REAL,
    light_intensity REAL,
    accel_x REAL,
    accel_y REAL,
    accel_z REAL,
    accel_norm REAL,
    gyro_x REAL,
    gyro_y REAL,
    gyro_z REAL,
    gyro_norm REAL,
    mag_x REAL,
    mag_y REAL,
    mag_z REAL,
    mag_norm REAL,
    volume_rms REAL,
    spectral_f1_405nm REAL,
    spectral_f2_425nm REAL,
    spectral_f3_475nm REAL,
    spectral_f4_515nm REAL,
    spectral_fz_450nm REAL,
    spectral_fy_555nm REAL,
    spectral_f5_550nm REAL,
    spectral_f6_640nm REAL,
    spectral_fxl_600nm REAL,
    spectral_f7_690nm REAL,
    spectral_f8_745nm REAL,
    spectral_nir_855nm REAL,
    spectral_vis REAL,
    spectral_fd REAL,
    FOREIGN KEY (node_id) REFERENCES devices(node_id) ON DELETE CASCADE
);

CREATE INDEX idx_sensor_data_node_id ON sensor_data(node_id);
CREATE INDEX idx_sensor_data_timestamp ON sensor_data(timestamp);
CREATE INDEX idx_sensor_data_node_timestamp ON sensor_data(node_id, timestamp DESC);

-- Questions table
CREATE TABLE questions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    question_text TEXT NOT NULL,
    question_type VARCHAR(20) NOT NULL CHECK(question_type IN ('multiple_choice', 'free_text')),
    options TEXT,  -- JSON array for multiple choice
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    created_by VARCHAR(100)
);

CREATE INDEX idx_questions_active ON questions(is_active, created_at DESC);

-- Answers table
CREATE TABLE answers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    question_id INTEGER NOT NULL,
    mac_id VARCHAR(64) NOT NULL,
    answer TEXT NOT NULL,
    answered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (question_id) REFERENCES questions(id) ON DELETE CASCADE,
    FOREIGN KEY (mac_id) REFERENCES users(mac_id) ON DELETE CASCADE,
    UNIQUE(question_id, mac_id)  -- Prevent duplicate answers
);

CREATE INDEX idx_answers_question_id ON answers(question_id);
CREATE INDEX idx_answers_mac_id ON answers(mac_id);

-- User preferences table
CREATE TABLE user_preferences (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    mac_id VARCHAR(64) NOT NULL,
    node_id VARCHAR(50) NOT NULL,
    sensor_type VARCHAR(50) NOT NULL,
    display_type VARCHAR(20) NOT NULL CHECK(display_type IN ('stripchart', 'numeric', 'both', 'none')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (mac_id) REFERENCES users(mac_id) ON DELETE CASCADE,
    FOREIGN KEY (node_id) REFERENCES devices(node_id) ON DELETE CASCADE,
    UNIQUE(mac_id, node_id, sensor_type)
);

CREATE INDEX idx_user_preferences_mac_id ON user_preferences(mac_id);
CREATE UNIQUE INDEX idx_user_preferences_unique ON user_preferences(mac_id, node_id, sensor_type);
```

---

## Query Examples

### Get Active Devices
```sql
SELECT node_id, hostname, last_seen
FROM devices
WHERE last_seen > datetime('now', '-5 seconds')
ORDER BY last_seen DESC;
```

### Get Recent Sensor Data for Device
```sql
SELECT timestamp, temperature, pressure, light_intensity
FROM sensor_data
WHERE node_id = 'LAB_01'
  AND timestamp > datetime('now', '-1 hour')
ORDER BY timestamp DESC
LIMIT 1000;
```

### Get User's Unanswered Questions
```sql
SELECT q.id, q.question_text, q.question_type, q.options
FROM questions q
LEFT JOIN answers a ON q.id = a.question_id AND a.mac_id = ?
WHERE q.is_active = TRUE
  AND a.id IS NULL;
```

### Get User's Preferences for Device
```sql
SELECT sensor_type, display_type
FROM user_preferences
WHERE mac_id = ?
  AND node_id = ?;
```

### Clean Up Old Sensor Data
```sql
DELETE FROM sensor_data
WHERE timestamp < datetime('now', '-24 hours');
```

---

## Database Maintenance

### Periodic Tasks (Cron/Systemd Timer)

1. **Sensor Data Cleanup** (every hour)
```bash
sqlite3 dashboard.db "DELETE FROM sensor_data WHERE timestamp < datetime('now', '-24 hours');"
```

2. **VACUUM** (weekly, reclaim space)
```bash
sqlite3 dashboard.db "VACUUM;"
```

3. **Analyze** (weekly, update query planner statistics)
```bash
sqlite3 dashboard.db "ANALYZE;"
```

### Backup Strategy

1. **Daily Backup** (retain 7 days)
```bash
sqlite3 dashboard.db ".backup /var/backups/dashboard_$(date +%Y%m%d).db"
```

2. **Before Schema Changes**
```bash
cp dashboard.db dashboard.db.backup
```

---

## Performance Considerations

### Write Performance
- **Expected**: 500 writes/sec (10 devices @ 50 Hz)
- **SQLite Limit**: 50,000+ writes/sec (WAL mode)
- **Bottleneck**: Network (UDP packet processing)
- **Optimization**: Batch inserts (10-100 records per transaction)

### Read Performance
- **Expected**: 10-100 reads/sec (WebSocket broadcasts)
- **SQLite Limit**: Unlimited concurrent reads (WAL mode)
- **Optimization**: Proper indexing, query caching

### Storage Growth
- **Rate**: ~100 MB/hour (10 devices @ 50 Hz, all sensors)
- **With 24-hour retention**: ~2.4 GB steady-state
- **Mitigation**: Regular cleanup, WAL checkpointing

---

## Schema Versioning

Track schema changes for migrations:

```sql
CREATE TABLE schema_version (
    version INTEGER PRIMARY KEY,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    description TEXT
);

INSERT INTO schema_version (version, description) VALUES
(1, 'Initial schema with users, devices, sensor_data, questions, answers, user_preferences');
```

Future migrations:
```python
# Example migration script
def migrate_v1_to_v2(db):
    db.execute("ALTER TABLE users ADD COLUMN email VARCHAR(100);")
    db.execute("INSERT INTO schema_version (version, description) VALUES (2, 'Add email to users');")
```
