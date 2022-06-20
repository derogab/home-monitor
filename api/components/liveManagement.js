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
    mqttClient.publish(topic, message, {
        qos: 1,
        retain: true,
    });
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
const on_connected = async function() {
    
    // Get devices
    let result = null;
    do {
        // Get devices list
        result = await deviceManagement.getDevices(deviceManagement.DEVICE_TYPE_SENSOR);
    } while (!result || !result.success);
    const devices = result.devices || [];

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
        subscribe('unishare/devices/status/' + device.mac); 
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
    else if (topic.includes('status')) {
        // Get mac address 
        const mac = topic.split('/')[3];
        // Get device status data
        const status = data.connected || false;
        // Set device status data
        await dataManagement.setDeviceStatus(mac, status);
        // Log
        logger.debug('Set device ' + mac + ' status to ' + status);
    }
};




// Start listener
module.exports.startListener = async function() {

    // Connected to MQTT broker
    logger.debug('Try to subscribe to all useful MQTT topics.');
    
    // MQTT connection handler
    mqttClient.on('connect', on_connected);

    // MQTT message handler
    mqttClient.on('message', on_message);

};






