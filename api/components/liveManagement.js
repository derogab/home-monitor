// Logger
const logger = require('../utils/logger');
// Import dependencies
const mqtt = require('mqtt');
// Import other components
const deviceManagement = require('./deviceManagement');
const dataManagement = require('./dataManagement');
const alarmManagement = require('./alarmManagement');
const apiManagement = require('./apiManagement');
// Init ENVs
require('dotenv').config();

// Get configs
const mqtt_host = process.env.MQTT_HOST;
const mqtt_port = process.env.MQTT_PORT;
const mqtt_user = process.env.MQTT_USERNAME;
const mqtt_pass = process.env.MQTT_PASSWORD;

// Init MQTT client
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
// Connected to MQTT broker
logger.info('Connected to MQTT broker.');



// Utils
module.exports.publish = function(topic, message) {
    // Just send to MQTT
    mqttClient.publish(topic, message);
};

module.exports.areDeviceListening = function() {
    // Return TRUE if client is set up
    return mqttClient ? true : false;
};

const subscribe = module.exports.subscribe = function(topic) {
    // Subscribe to a topic
    if (mqttClient) mqttClient.subscribe(topic, function (err) {
        if (!err) logger.info('Subscribed to topic ' + topic + '.');
        else logger.warning('Failed to subscribe to topic ' + topic + '.');
    });
    else logger.warning('Impossible to subscribe to topic ' + topic + ' because MQTT client doesn\'t exist.');
};



// Callbacks
const on_connected = function() {
    // Connected to MQTT broker
    logger.debug('Subscribing to all MQTT topics for each devices...');
    // Subscribe to all topics for each devices
    for (let i = 0; i < devices.length; i++) {
        const device = devices[i];
        // Subscribe to MQTT topics 
        subscribe('unishare/sensors/' + device.mac + '/light');
        subscribe('unishare/sensors/' + device.mac + '/flame');
        subscribe('unishare/sensors/' + device.mac + '/temperature');
        subscribe('unishare/sensors/' + device.mac + '/apparent_temperature');
        subscribe('unishare/sensors/' + device.mac + '/humidity'); 
    }
};

const on_message = async function (topic, message) {
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
};




// Start listener
module.exports.startListener = async function() {

    // Connected to MQTT broker
    logger.debug('Try to subscribe to all useful MQTT topics.');
    
    // Get devices
    let result = null;
    do {
        // Get devices list
        result = await deviceManagement.getDevices(deviceManagement.DEVICE_TYPE_SENSOR);
    } while (!result || !result.success);
    const devices = result.devices || [];

    // MQTT connection handler
    mqttClient.on('connect', on_connected);

    // MQTT message handler
    mqttClient.on('message', on_message);

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


};






