// Import logger
const logger = require("./components/logger");
// Import Express
const express = require('express');
const cors = require('cors');
const app = express();
// Import MQTT
const mqtt = require('mqtt');
// Import MySQL
const mysql = require('mysql');

// Init CORS
app.use(cors({ origin: '*' }));

// Init ENVs
require('dotenv').config();

// Get configs
const mqtt_host = process.env.MQTT_HOST;
const mqtt_port = process.env.MQTT_PORT;
const mqtt_user = process.env.MQTT_USERNAME;
const mqtt_pass = process.env.MQTT_PASSWORD;
const mysql_host = process.env.MYSQL_HOST;
const mysql_port = process.env.MYSQL_PORT;
const mysql_user = process.env.MYSQL_USERNAME;
const mysql_pass = process.env.MYSQL_PASSWORD;
const mysql_database = process.env.MYSQL_DATABASE;

// Global variables
let mqtt_data_fire = false;
let mqtt_data_light = false;
let mqtt_data_temperature = 0;
let mqtt_data_apparent_temperature = 0;
let mqtt_data_humidity = 0;

// Starting 
logger.info('Starting API...');

// Init MQTT client
logger.debug('Starting MQTT...');
// Try to connect to MQTT broker
logger.debug('Connecting to MQTT broker...');
logger.debug('MQTT broker: ' + mqtt_host + ':' + mqtt_port);
const mqttClient = mqtt.connect('mqtt://' + mqtt_host + ':' + mqtt_port, {
    protocol: 'mqtt',
    clientId: 'api',
    clean: true,
    connectTimeout: 4000,
    username: mqtt_user,
    password: mqtt_pass,
    reconnectPeriod: 1000,
});

// Connect to MQTT broker
mqttClient.on('connect', function () {
    // Connected to MQTT broker
    logger.info('Connected to MQTT broker.');
    // Subscribe to MQTT topics
    mqttClient.subscribe('unishare/sensors/807D3A42D1C5/light', function (err) {
        if (!err) logger.info('Subscribed to topic LIGHT.');
    });
    mqttClient.subscribe('unishare/sensors/807D3A42D1C5/flame', function (err) {
        if (!err) logger.info('Subscribed to topic FIRE.');
    });
    mqttClient.subscribe('unishare/sensors/807D3A42D1C5/temperature', function (err) {
        if (!err) logger.info('Subscribed to topic TEMP.');
    });
    mqttClient.subscribe('unishare/sensors/807D3A42D1C5/apparent_temperature', function (err) {
        if (!err) logger.info('Subscribed to topic APPARENT TEMP.');
    });
    mqttClient.subscribe('unishare/sensors/807D3A42D1C5/humidity', function (err) {
        if (!err) logger.info('Subscribed to topic HUMIDITY.');
    });
});

// MQTT message handler
mqttClient.on('message', function (topic, message) {
    // Get message
    const msg = message.toString();
    // Get JSON data
    const data = JSON.parse(msg);
    // Log message
    logger.debug('MQTT message: ' + message.toString());
    // Check topic
    if (topic.includes('flame')) {
        // Get fire data
        const fire = data.value || false;
        // Set fire data
        mqtt_data_fire = fire;
    }
    else if (topic.includes('light')) {
        // Get light data
        const light = data.value || false;
        // Set light data
        mqtt_data_light = light;
    }
    else if (topic.includes('/temperature')) {
        // Get light data
        const temperature = data.value || 'N/A';
        // Set light data
        mqtt_data_temperature = temperature;
    }
    else if (topic.includes('apparent_temperature')) {
        // Get light data
        const apparent_temperature = data.value || 'N/A';
        // Set light data
        mqtt_data_apparent_temperature = apparent_temperature;
    }
    else if (topic.includes('humidity')) {
        // Get light data
        const humidity = data.value || 'N/A';
        // Set light data
        mqtt_data_humidity = humidity;
    }
});

// Init MYSQL client 
logger.debug('Starting MYSQL...');
const mysqlClient = mysql.createConnection({
    host     : mysql_host || 'localhost',
    port     : mysql_port || 3306,
    user     : mysql_user,
    password : mysql_pass,
    database : mysql_database
});
// Try to connect to MYSQL server
logger.debug('Connecting to MYSQL server...');
logger.debug('MYSQL server: ' + mysql_host + ':' + mysql_port);
mysqlClient.connect(function(err) {
    if (err) {
      logger.error('MySQL Connecting Error: ' + err.stack);
      return;
    }
   
    logger.info('Connected to MYSQL server as id ' + mysqlClient.threadId);
});

// Root
app.get('/', function (req, res) {
    res.status(200).send('Status: UP!');
});

// Fire
app.get('/fire', function (req, res) {
    res.status(200).json({
        success: true,
        value: mqtt_data_fire || false,
    });
});
// Light
app.get('/light', function (req, res) {
    res.status(200).json({
        success: true,
        value: mqtt_data_light || false,
    });
});
// Temperature
app.get('/temperature', function (req, res) {
    res.status(200).json({
        success: true,
        value: mqtt_data_temperature.toFixed(2) || 'N/A',
    });
});
// Apparent Temperature
app.get('/apparent_temperature', function (req, res) {
    res.status(200).json({
        success: true,
        value: mqtt_data_apparent_temperature.toFixed(2) || 'N/A',
    });
});
// Humidity
app.get('/humidity', function (req, res) {
    res.status(200).json({
        success: true,
        value: mqtt_data_humidity.toFixed(0) || 'N/A',
    });
});

// Sensors
app.get('/sensors', async function (req, res) {
    // Logs
    logger.debug('Trying to get the sensors list...');
    // Check it client is set
    if (mysqlClient) {
        // Get devices from DB
        mysqlClient.query("SELECT NAME AS name, MAC_ADDRESS as mac FROM home_monitor_devices WHERE TYPE = 'sensors'", function (error, results, fields) {
            if (error) {
                // Error
                logger.error('Error getting sensors list.');
                // Response
                res.status(500).json({ sensors: [], success: false });
            }
            // Log
            logger.debug((results ? results.length : 0) + ' sensor(s) found!');
            // Return data
            res.status(200).json({ sensors: results, success: true });
        });
    } else {
        // Send response
        res.status(500).json({ sensors: [], success: false });
        // Log
        logger.error('Failed to get the sensors list.');
    }
});

// Control LIGHT
// LIGHT: ON
app.get('/control/:mac/light/on', function (req, res) {
    // Logs
    logger.debug('Turning ON light of ' + req.params.mac + '...');
    // Check it client is set
    if (mqttClient) {
        // Send MQTT message
        mqttClient.publish('unishare/control/' + req.params.mac + '/light', '{"control": "on"}');
        // Send response
        res.status(200).json({ status: 'ON', success: true });
        // Log
        logger.info('Light of ' + req.params.mac + ' turned ON.');
    } else {
        // Send response
        res.status(500).json({ status: 'ON', success: false });
        // Log
        logger.warning('Light of ' + req.params.mac + ' not turned ON.');
    }
});
// LIGHT: OFF
app.get('/control/:mac/light/off', function (req, res) {
    // Logs
    logger.debug('Turning OFF light of ' + req.params.mac + '...');
    // Check it client is set
    if (mqttClient) {
        // Send MQTT message
        mqttClient.publish('unishare/control/' + req.params.mac + '/light', '{"control": "off"}');
        // Send response
        res.status(200).json({ status: 'OFF', success: true });
        // Log
        logger.info('Light of ' + req.params.mac + ' turned OFF.');
    } else {
        // Send response
        res.status(500).json({ status: 'OFF', success: false });
        // Log
        logger.warning('Light of ' + req.params.mac + ' not turned OFF.');
    }
});

// Control AIR
// AIR: ON
app.get('/control/:mac/air/on', function (req, res) {
    // Logs
    logger.debug('Turning ON air of ' + req.params.mac + '...');
    // Check it client is set
    if (mqttClient) {
        // Send MQTT message
        mqttClient.publish('unishare/control/' + req.params.mac + '/ac', '{"control": "on"}');
        // Send response
        res.status(200).json({ status: 'ON', success: true });
        // Log
        logger.info('Air of ' + req.params.mac + ' turned ON.');
    } else {
        // Send response
        res.status(500).json({ status: 'ON', success: false });
        // Log
        logger.warning('Air of ' + req.params.mac + ' not turned ON.');
    }
});
// AIR: OFF
app.get('/control/:mac/air/off', function (req, res) {
    // Logs
    logger.debug('Turning OFF air of ' + req.params.mac + '...');
    // Check it client is set
    if (mqttClient) {
        // Send MQTT message
        mqttClient.publish('unishare/control/' + req.params.mac + '/ac', '{"control": "off"}');
        // Send response
        res.status(200).json({ status: 'OFF', success: true });
        // Log
        logger.info('Air of ' + req.params.mac + ' turned OFF.');
    } else {
        // Send response
        res.status(500).json({ status: 'OFF', success: false });
        // Log
        logger.warning('Air of ' + req.params.mac + ' not turned OFF.');
    }
});

// Listen
app.listen(3001);
