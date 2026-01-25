"""
Main Flask Application for Raspberry Pi Dashboard.
Provides REST API, WebSocket support, and serves frontend.
"""

import os
import logging
import hashlib
import json
from datetime import datetime, timedelta
from flask import Flask, request, jsonify, send_from_directory, redirect, url_for
from flask_cors import CORS
from flask_socketio import SocketIO, emit, join_room, leave_room
from sqlalchemy import desc
from sqlalchemy.exc import IntegrityError

from config import get_config
from models import db, User, Device, SensorData, Question, Answer, UserPreference
from udp_listener import UDPListener, cleanup_old_sensor_data

# Initialize Flask app
app = Flask(__name__, static_folder='static')
config_class = get_config()
app.config.from_object(config_class)

# Initialize extensions
db.init_app(app)
CORS(app, origins=app.config['CORS_ORIGINS'])
# Use eventlet mode with Redis message queue for inter-process communication
# This allows the UDP listener (separate service) and web server to share WebSocket messages
# IMPORTANT: async_mode must match Gunicorn worker_class (both must be 'eventlet')
socketio = SocketIO(
    app,
    cors_allowed_origins=app.config['CORS_ORIGINS'],
    async_mode='eventlet',  # Must match Gunicorn worker_class
    message_queue='redis://localhost:6379/0',
    logger=True,
    engineio_logger=False
)

# Setup logging
logging.basicConfig(
    level=logging.INFO if not app.config['DEBUG'] else logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Initialize UDP listener (only if not disabled via environment variable)
# In production, the UDP listener runs as a separate service (pocketlab-listener)
# Set DISABLE_UDP_LISTENER=1 when running the web server to avoid port conflicts
udp_listener = None
if not os.environ.get('DISABLE_UDP_LISTENER'):
    udp_listener = UDPListener(
        app,
        socketio,
        port=app.config['UDP_PORT'],
        activity_timeout=app.config['ACTIVITY_TIMEOUT_SECONDS']
    )


# ============================================================================
# Helper Functions
# ============================================================================

def hash_mac_address(mac_address):
    """Hash MAC address with SHA-256 for privacy."""
    return hashlib.sha256(mac_address.encode('utf-8')).hexdigest()


def get_active_devices():
    """Get list of active devices based on timeout."""
    timeout = timedelta(seconds=app.config['ACTIVITY_TIMEOUT_SECONDS'])
    cutoff_time = datetime.utcnow() - timeout
    return Device.query.filter(Device.last_seen > cutoff_time).all()


# ============================================================================
# REST API Routes - Users
# ============================================================================

@app.route('/api/users/register', methods=['POST'])
def register_user():
    """Register a new user or return existing user."""
    try:
        data = request.json
        mac_address = data.get('mac_address')

        if not mac_address:
            return jsonify({'error': 'MAC address is required'}), 400

        # Hash MAC address for privacy
        mac_id = hash_mac_address(mac_address)

        # Check if user exists
        user = User.query.filter_by(mac_id=mac_id).first()

        if user:
            # Update last_seen
            user.last_seen = datetime.utcnow()
            db.session.commit()
            return jsonify({
                'status': 'returning_user',
                'user': user.to_dict()
            }), 200
        else:
            # Create new user
            name = data.get('name')
            grade = data.get('grade')
            science_interest = data.get('science_interest')

            if not name:
                return jsonify({'error': 'Name is required for new users'}), 400

            user = User(
                mac_id=mac_id,
                name=name,
                grade=grade,
                science_interest=science_interest
            )
            db.session.add(user)
            db.session.commit()

            return jsonify({
                'status': 'new_user',
                'user': user.to_dict()
            }), 201

    except Exception as e:
        logger.error(f"Error registering user: {e}", exc_info=True)
        db.session.rollback()
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/users/<mac_id>', methods=['GET'])
def get_user(mac_id):
    """Get user by MAC ID."""
    user = User.query.filter_by(mac_id=mac_id).first()
    if not user:
        return jsonify({'error': 'User not found'}), 404
    return jsonify(user.to_dict()), 200


# ============================================================================
# REST API Routes - Devices
# ============================================================================

@app.route('/api/devices', methods=['GET'])
def get_devices():
    """Get list of active devices."""
    try:
        devices = get_active_devices()
        return jsonify({
            'devices': [d.to_dict(app.config['ACTIVITY_TIMEOUT_SECONDS']) for d in devices]
        }), 200
    except Exception as e:
        logger.error(f"Error getting devices: {e}", exc_info=True)
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/devices/<node_id>', methods=['GET'])
def get_device(node_id):
    """Get specific device by node_id."""
    device = Device.query.filter_by(node_id=node_id).first()
    if not device:
        return jsonify({'error': 'Device not found'}), 404
    return jsonify(device.to_dict(app.config['ACTIVITY_TIMEOUT_SECONDS'])), 200


# ============================================================================
# REST API Routes - Sensor Data
# ============================================================================

@app.route('/api/sensor-data/<node_id>', methods=['GET'])
def get_sensor_data(node_id):
    """
    Get sensor data for a specific device.

    Query parameters:
        - limit: Maximum number of records (default: 1000)
        - hours: Get data from last N hours (default: 1)
        - sensors: Comma-separated list of sensor types (optional)
    """
    try:
        limit = int(request.args.get('limit', 1000))
        hours = int(request.args.get('hours', 1))
        sensors = request.args.get('sensors', '').split(',') if request.args.get('sensors') else None

        # Calculate time range
        cutoff_time = datetime.utcnow() - timedelta(hours=hours)

        # Query sensor data
        query = SensorData.query.filter(
            SensorData.node_id == node_id,
            SensorData.timestamp > cutoff_time
        ).order_by(desc(SensorData.timestamp)).limit(limit)

        data = query.all()

        # Convert to dict (optionally filter sensors)
        result = []
        for record in data:
            record_dict = record.to_dict(include_nulls=False)

            # Filter specific sensors if requested
            if sensors:
                filtered = {'id': record_dict['id'], 'timestamp': record_dict['timestamp'], 'node_id': record_dict['node_id']}
                for sensor in sensors:
                    if sensor in record_dict:
                        filtered[sensor] = record_dict[sensor]
                result.append(filtered)
            else:
                result.append(record_dict)

        return jsonify({
            'node_id': node_id,
            'count': len(result),
            'data': result
        }), 200

    except Exception as e:
        logger.error(f"Error getting sensor data: {e}", exc_info=True)
        return jsonify({'error': 'Internal server error'}), 500


# ============================================================================
# REST API Routes - Questions
# ============================================================================

@app.route('/api/questions', methods=['GET'])
def get_questions():
    """Get all active questions."""
    try:
        questions = Question.query.filter_by(is_active=True).order_by(desc(Question.created_at)).all()
        return jsonify({
            'questions': [q.to_dict() for q in questions]
        }), 200
    except Exception as e:
        logger.error(f"Error getting questions: {e}", exc_info=True)
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/questions', methods=['POST'])
def create_question():
    """Create a new question (teacher only - add auth later)."""
    try:
        data = request.json
        question_text = data.get('question_text')
        question_type = data.get('question_type')
        options = data.get('options')
        created_by = data.get('created_by', 'Teacher')

        if not question_text or not question_type:
            return jsonify({'error': 'question_text and question_type are required'}), 400

        if question_type not in ['multiple_choice', 'free_text']:
            return jsonify({'error': 'question_type must be "multiple_choice" or "free_text"'}), 400

        # Convert options to JSON string if provided
        options_json = json.dumps(options) if options else None

        question = Question(
            question_text=question_text,
            question_type=question_type,
            options=options_json,
            created_by=created_by
        )
        db.session.add(question)
        db.session.commit()

        # Broadcast new question to all clients
        socketio.emit('new_question', question.to_dict(), namespace='/')

        return jsonify({
            'status': 'success',
            'question': question.to_dict()
        }), 201

    except Exception as e:
        logger.error(f"Error creating question: {e}", exc_info=True)
        db.session.rollback()
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/questions/<int:question_id>', methods=['DELETE'])
def delete_question(question_id):
    """Deactivate a question (soft delete)."""
    try:
        question = Question.query.filter_by(id=question_id).first()
        if not question:
            return jsonify({'error': 'Question not found'}), 404

        question.is_active = False
        db.session.commit()

        # Broadcast question removal
        socketio.emit('question_removed', {'question_id': question_id}, namespace='/')

        return jsonify({'status': 'success'}), 200

    except Exception as e:
        logger.error(f"Error deleting question: {e}", exc_info=True)
        db.session.rollback()
        return jsonify({'error': 'Internal server error'}), 500


# ============================================================================
# REST API Routes - Answers
# ============================================================================

@app.route('/api/answers', methods=['POST'])
def submit_answer():
    """Submit an answer to a question."""
    try:
        data = request.json
        question_id = data.get('question_id')
        mac_id = data.get('mac_id')
        answer_text = data.get('answer')

        if not all([question_id, mac_id, answer_text]):
            return jsonify({'error': 'question_id, mac_id, and answer are required'}), 400

        # Check if question exists and is active
        question = Question.query.filter_by(id=question_id, is_active=True).first()
        if not question:
            return jsonify({'error': 'Question not found or inactive'}), 404

        # Check if user exists
        user = User.query.filter_by(mac_id=mac_id).first()
        if not user:
            return jsonify({'error': 'User not found. Please register first.'}), 404

        # Try to create answer (will fail if already answered due to UNIQUE constraint)
        answer = Answer(
            question_id=question_id,
            mac_id=mac_id,
            answer=answer_text
        )
        db.session.add(answer)
        db.session.commit()

        return jsonify({
            'status': 'success',
            'answer': answer.to_dict()
        }), 201

    except IntegrityError:
        db.session.rollback()
        return jsonify({'error': 'You have already answered this question'}), 409
    except Exception as e:
        logger.error(f"Error submitting answer: {e}", exc_info=True)
        db.session.rollback()
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/answers/user/<mac_id>', methods=['GET'])
def get_user_answers(mac_id):
    """Get all answers by a specific user."""
    try:
        answers = Answer.query.filter_by(mac_id=mac_id).all()
        return jsonify({
            'answers': [a.to_dict() for a in answers]
        }), 200
    except Exception as e:
        logger.error(f"Error getting user answers: {e}", exc_info=True)
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/answers/question/<int:question_id>', methods=['GET'])
def get_question_answers(question_id):
    """Get all answers for a specific question (teacher view)."""
    try:
        answers = Answer.query.filter_by(question_id=question_id).all()
        return jsonify({
            'question_id': question_id,
            'count': len(answers),
            'answers': [a.to_dict() for a in answers]
        }), 200
    except Exception as e:
        logger.error(f"Error getting question answers: {e}", exc_info=True)
        return jsonify({'error': 'Internal server error'}), 500


# ============================================================================
# REST API Routes - User Preferences
# ============================================================================

@app.route('/api/preferences', methods=['GET'])
def get_preferences():
    """Get user preferences for a specific device."""
    mac_id = request.args.get('mac_id')
    node_id = request.args.get('node_id')

    if not mac_id or not node_id:
        return jsonify({'error': 'mac_id and node_id are required'}), 400

    try:
        preferences = UserPreference.query.filter_by(mac_id=mac_id, node_id=node_id).all()
        return jsonify({
            'preferences': [p.to_dict() for p in preferences]
        }), 200
    except Exception as e:
        logger.error(f"Error getting preferences: {e}", exc_info=True)
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/preferences', methods=['POST'])
def save_preferences():
    """Save or update user preferences."""
    try:
        data = request.json
        mac_id = data.get('mac_id')
        node_id = data.get('node_id')
        sensor_type = data.get('sensor_type')
        display_type = data.get('display_type')

        if not all([mac_id, node_id, sensor_type, display_type]):
            return jsonify({'error': 'mac_id, node_id, sensor_type, and display_type are required'}), 400

        if display_type not in ['stripchart', 'numeric', 'both', 'none']:
            return jsonify({'error': 'Invalid display_type'}), 400

        # Check if preference exists
        preference = UserPreference.query.filter_by(
            mac_id=mac_id,
            node_id=node_id,
            sensor_type=sensor_type
        ).first()

        if preference:
            # Update existing
            preference.display_type = display_type
            preference.updated_at = datetime.utcnow()
        else:
            # Create new
            preference = UserPreference(
                mac_id=mac_id,
                node_id=node_id,
                sensor_type=sensor_type,
                display_type=display_type
            )
            db.session.add(preference)

        db.session.commit()

        return jsonify({
            'status': 'success',
            'preference': preference.to_dict()
        }), 200

    except Exception as e:
        logger.error(f"Error saving preferences: {e}", exc_info=True)
        db.session.rollback()
        return jsonify({'error': 'Internal server error'}), 500


# ============================================================================
# WebSocket Events
# ============================================================================

@socketio.on('connect')
def handle_connect():
    """Handle client connection."""
    logger.info(f"Client connected: {request.sid}")
    emit('connection_response', {'status': 'connected'})


@socketio.on('disconnect')
def handle_disconnect():
    """Handle client disconnection."""
    logger.info(f"Client disconnected: {request.sid}")


@socketio.on('subscribe_device')
def handle_subscribe_device(data):
    """Subscribe to sensor data for a specific device."""
    node_id = data.get('node_id')
    if not node_id:
        emit('error', {'message': 'node_id is required'})
        return

    room = f'device_{node_id}'
    join_room(room)
    logger.info(f"Client {request.sid} subscribed to {node_id}")
    emit('subscription_response', {'status': 'subscribed', 'node_id': node_id})


@socketio.on('unsubscribe_device')
def handle_unsubscribe_device(data):
    """Unsubscribe from sensor data for a specific device."""
    node_id = data.get('node_id')
    if not node_id:
        emit('error', {'message': 'node_id is required'})
        return

    room = f'device_{node_id}'
    leave_room(room)
    logger.info(f"Client {request.sid} unsubscribed from {node_id}")
    emit('subscription_response', {'status': 'unsubscribed', 'node_id': node_id})


# ============================================================================
# Static File Serving
# ============================================================================

@app.route('/')
def index():
    """Serve the main dashboard with tabs."""
    return send_from_directory(app.static_folder, 'index.html')


@app.route('/<path:path>')
def serve_static(path):
    """Serve static files."""
    return send_from_directory(app.static_folder, path)


# ============================================================================
# Application Startup
# ============================================================================

def main():
    """Main application entry point."""
    with app.app_context():
        # Create tables if they don't exist
        db.create_all()
        logger.info("Database tables verified/created")

    # Start UDP listener (if enabled)
    if udp_listener:
        udp_listener.start()
        logger.info("UDP listener started (integrated mode)")
    else:
        logger.info("UDP listener disabled (running as separate service)")

    # Start Flask-SocketIO server
    logger.info(f"Starting dashboard server on {app.config['BIND_ADDRESS']}:{app.config['WEB_PORT']}")
    socketio.run(
        app,
        host=app.config['BIND_ADDRESS'],
        port=app.config['WEB_PORT'],
        debug=app.config['DEBUG']
    )


if __name__ == '__main__':
    main()
