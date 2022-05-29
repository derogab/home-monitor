// Import Express
const express = require('express');
const app = express();
// Import MQTT
const mqtt = require('mqtt');

// Init ENVs
require('dotenv').config();

// Get configs
const host = process.env.HOST;
const port = process.env.PORT;
const user = process.env.USERNAME;
const pass = process.env.PASSWORD;

// Starting 
console.log('Starting server...');

// Init MQTT client
// Try to connect to MQTT broker
console.log('Connecting to MQTT broker...');
console.log('MQTT broker: ' + host + ':' + port);
const mqttClient = mqtt.connect('mqtt://' + host + ':' + port, {
    protocol: 'mqtt',
    clientId: 'api',
    clean: true,
    connectTimeout: 4000,
    username: user,
    password: pass,
    reconnectPeriod: 1000,
});

// Connect to MQTT broker
mqttClient.on('connect', function () {
    // Connected to MQTT broker
    console.log('Connected to MQTT broker.');
});

// Root
app.get('/', function (req, res) {
    res.send('Status: UP!');
});

// Fun ON
app.get('/control/:mac/light/on', function (req, res) {
    // Logs
    console.log('Turning ON light of ' + req.params.mac + '.');
    // Check it client is set
    if (mqttClient) {
        // Send MQTT message
        mqttClient.publish('unishare/control/' + req.params.mac + '/light', '{value: true}');
        // Send response
        res.json({ status: 'ON', success: true });
    } else {
        // Send response
        res.json({ status: 'ON', success: false });
    }
});

// Fun OFF
app.get('/control/:mac/light/off', function (req, res) {
    // Logs
    console.log('Turning OFF light of ' + req.params.mac + '.');
    // Check it client is set
    if (mqttClient) {
        // Send MQTT message
        mqttClient.publish('unishare/control/' + req.params.mac + '/light', '{value: false}');
        // Send response
        res.json({ status: 'OFF', success: true });
    } else {
        // Send response
        res.json({ status: 'OFF', success: false });
    }
});

// Listen
app.listen(3001);
