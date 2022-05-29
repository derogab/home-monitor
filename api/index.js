// Import logger
const Logger = require("@ptkdev/logger");
// Import Express
const express = require('express');
const app = express();
// Import MQTT
const mqtt = require('mqtt');
// Import MySQL
const mysql = require('mysql');

// Init ENVs
require('dotenv').config();

// Init logger
const logger = new Logger();

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
    res.send('Status: UP!');
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
                console.log(error.stack);
                // Response
                res.status(500).json({ sensors: [], success: false });
            }
            // Return data
            res.json({ sensors: results, success: true });
        });
    } else {
        // Send response
        res.json({ sensors: [], success: false });
        // Log
        logger.error('Failed to get the sensors list.');
    }
});

// Control ON
app.get('/control/:mac/light/on', function (req, res) {
    // Logs
    logger.debug('Turning ON light of ' + req.params.mac + '...');
    // Check it client is set
    if (mqttClient) {
        // Send MQTT message
        mqttClient.publish('unishare/control/' + req.params.mac + '/light', '{value: true}');
        // Send response
        res.json({ status: 'ON', success: true });
        // Log
        logger.info('Light of ' + req.params.mac + ' turned ON.');
    } else {
        // Send response
        res.json({ status: 'ON', success: false });
        // Log
        logger.warning('Light of ' + req.params.mac + ' not turned ON.');
    }
});

// Control OFF
app.get('/control/:mac/light/off', function (req, res) {
    // Logs
    logger.debug('Turning OFF light of ' + req.params.mac + '...');
    // Check it client is set
    if (mqttClient) {
        // Send MQTT message
        mqttClient.publish('unishare/control/' + req.params.mac + '/light', '{value: false}');
        // Send response
        res.json({ status: 'OFF', success: true });
        // Log
        logger.info('Light of ' + req.params.mac + ' turned OFF.');
    } else {
        // Send response
        res.json({ status: 'OFF', success: false });
        // Log
        logger.warning('Light of ' + req.params.mac + ' not turned OFF.');
    }
});

// Listen
app.listen(3001);
