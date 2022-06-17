// Import utils
const logger = require("./utils/logger");
// Import components management
const apiManagement = require("./components/apiManagement");
const dataManagement = require("./components/dataManagement");
const telegramManagement = require("./components/telegramManagement");
const cronManagement = require("./components/cronManagement");
const alarmManagement = require('./components/alarmManagement');

// Import MQTT
const mqtt = require('mqtt');

// Init ENVs
require('dotenv').config();

// Get configs
const mqtt_host = process.env.MQTT_HOST;
const mqtt_port = process.env.MQTT_PORT;
const mqtt_user = process.env.MQTT_USERNAME;
const mqtt_pass = process.env.MQTT_PASSWORD;

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
mqttClient.on('message', async function (topic, message) {
    // Get message
    const msg = message.toString();
    // Get JSON data
    const data = JSON.parse(msg);
    // Get device 
    const device = topic.split('/')[2];
    // Log message
    logger.debug('MQTT message: ' + message.toString() + ' from device ' +  device);
    // Check topic
    if (topic.includes('flame')) {
        // Get fire data
        const fire = data.value || false;
        // Set fire data
        await dataManagement.setFire(device, fire);
        // If there is fire, trigger the alarm
        if (fire) alarmManagement.trigger(device); // no await because it takes some time
    }
    else if (topic.includes('light')) {
        // Get light data
        const light = data.value || false;
        // Set light data
        await dataManagement.setLight(device, light);
    }
    else if (topic.includes('/temperature')) {
        // Get temperature data
        const temperature = data.value || 'N/A';
        // Set temperature data
        await dataManagement.setTemperature(device, temperature);
    }
    else if (topic.includes('apparent_temperature')) {
        // Get apparent temperature data
        const apparent_temperature = data.value || 'N/A';
        // Set apparent temperature data
        await dataManagement.setApparentTemperature(device, apparent_temperature);
    }
    else if (topic.includes('humidity')) {
        // Get humidity data
        const humidity = data.value || 'N/A';
        // Set humidity data
        await dataManagement.setHumidity(device, humidity);
    }
});

// Fire
apiManagement.webserver.get('/status/:device/fire', async function (req, res) {
    res.status(200).json({
        success: true,
        value: await dataManagement.getFire(req.params.device) || false,
    });
});
// Light
apiManagement.webserver.get('/status/:device/light', async function (req, res) {
    res.status(200).json({
        success: true,
        value: await dataManagement.getLight(req.params.device) || false,
    });
});
// Temperature
apiManagement.webserver.get('/status/:device/temperature', async function (req, res) {
    res.status(200).json({
        success: true,
        value: await dataManagement.getTemperature(req.params.device).toFixed(2) || 'N/A',
    });
});
// Apparent Temperature
apiManagement.webserver.get('/status/:device/apparent_temperature', async function (req, res) {
    res.status(200).json({
        success: true,
        value: await dataManagement.getApparentTemperature(req.params.device).toFixed(2) || 'N/A',
    });
});
// Humidity
apiManagement.webserver.get('/status/:device/humidity', async function (req, res) {
    res.status(200).json({
        success: true,
        value: await dataManagement.getHumidity(req.params.device).toFixed(0) || 'N/A',
    });
});

// Control LIGHT
// LIGHT: ON
apiManagement.webserver.get('/control/:mac/light/on', function (req, res) {
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
apiManagement.webserver.get('/control/:mac/light/off', function (req, res) {
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
apiManagement.webserver.get('/control/:mac/air/on', function (req, res) {
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
apiManagement.webserver.get('/control/:mac/air/off', function (req, res) {
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



// Start modules
dataManagement.connect();
apiManagement.startWebServer(3001);
telegramManagement.startBot();
cronManagement.startCron();
