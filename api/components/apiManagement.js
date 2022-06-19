// Import Express
const express = require('express');
const cors = require('cors');
const app = express();
// Import utils
const logger = require('../utils/logger');
// Import other components
const dataManagement = require('./dataManagement');
const deviceManagement = require('./deviceManagement');
const liveManagement = require('./liveManagement');
const historyManagement = require('./historyManagement');

// Init CORS
app.use(cors({ origin: '*' }));

// ROOT
// -------------------------------------
// Generic status
app.get('/', function (req, res) {
    res.status(200).send('Status: UP!');
});

// DEVICES
// -------------------------------------
// List of all devices
app.get('/devices', async function (req, res) {
    // Get devices list
    const result = await deviceManagement.getDevices(deviceManagement.DEVICE_TYPE_SENSOR);
    // Check if success
    const code = (result && result.success) ? 200 : 500;
    // Reply w/ results
    res.status(code).json(result);
});

// INFO (READ-ONLY)
// -------------------------------------
// Status informations about FIRE
app.get('/status/:device/fire', async function (req, res) { // Fire
    res.status(200).json({
        success: true,
        value: await dataManagement.getFire(req.params.device) || false,
    });
});
// Status informations about LIGHT
app.get('/status/:device/light', async function (req, res) { // Light
    res.status(200).json({
        success: true,
        value: await dataManagement.getLight(req.params.device) || false,
    });
});
// Status informations about TEMPERATURE
app.get('/status/:device/temperature', async function (req, res) { // Temperature
    res.status(200).json({
        success: true,
        value: await dataManagement.getTemperature(req.params.device).toFixed(2) || 'N/A',
    });
});
// Status informations about APPARENT TEMPERATURE
app.get('/status/:device/apparent-temperature', async function (req, res) { // Apparent Temperature
    res.status(200).json({
        success: true,
        value: await dataManagement.getApparentTemperature(req.params.device).toFixed(2) || 'N/A',
    });
});
// Status informations about HUMIDITY
app.get('/status/:device/humidity', async function (req, res) { // Humidity
    res.status(200).json({
        success: true,
        value: await dataManagement.getHumidity(req.params.device).toFixed(0) || 'N/A',
    });
});


// CONTROL (WRITE)
// -------------------------------------
// Control LIGHT: ON
app.get('/control/:mac/light/on', function (req, res) {
    // Logs
    logger.debug('Turning ON light of ' + req.params.mac + '...');
    // Check it client is set
    if (liveManagement.areDeviceListening()) {
        // Send MQTT message
        liveManagement.publish('unishare/control/' + req.params.mac + '/light', '{"control": "on"}');
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
// Control LIGHT: OFF
app.get('/control/:mac/light/off', function (req, res) {
    // Logs
    logger.debug('Turning OFF light of ' + req.params.mac + '...');
    // Check it client is set
    if (liveManagement.areDeviceListening()) {
        // Send MQTT message
        liveManagement.publish('unishare/control/' + req.params.mac + '/light', '{"control": "off"}');
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
// Control AIR: ON
app.get('/control/:mac/air/on', function (req, res) {
    // Logs
    logger.debug('Turning ON air of ' + req.params.mac + '...');
    // Check it client is set
    if (liveManagement.areDeviceListening()) {
        // Send MQTT message
        liveManagement.publish('unishare/control/' + req.params.mac + '/ac', '{"control": "on"}');
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
// Control AIR: OFF
app.get('/control/:mac/air/off', function (req, res) {
    // Logs
    logger.debug('Turning OFF air of ' + req.params.mac + '...');
    // Check it client is set
    if (liveManagement.areDeviceListening()) {
        // Send MQTT message
        liveManagement.publish('unishare/control/' + req.params.mac + '/ac', '{"control": "off"}');
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


// HISTORY (READ-ONLY)
// -------------------------------------
// Status informations about FIRE
app.get('/history/:device/fire', async function (req, res) { // Fire

    // Error if INFLUX is not connected
    if (!historyManagement.isConnected()) return res.status(404).json({
        success: false,
        value: null
    });

    // Get fire history
    const response = await historyManagement.getFireHistory(req.params.device);
    // Get code and success
    const code = response ? 200 : 404;
    
    // Reply
    return res.status(code).json({
        success: true,
        value: response,
    });
});
// Status informations about LIGHT
app.get('/history/:device/light', async function (req, res) { // Light

    // Error if INFLUX is not connected
    if (!historyManagement.isConnected()) return res.status(404).json({
        success: false,
        value: null
    });

    // Get light history
    const response = await historyManagement.getLightHistory(req.params.device);
    // Get code and success
    const code = response ? 200 : 404;
    
    // Reply
    return res.status(code).json({
        success: true,
        value: response,
    });
});
// Status informations about TEMPERATURE
app.get('/history/:device/temperature', async function (req, res) { // Temperature

    // Error if INFLUX is not connected
    if (!historyManagement.isConnected()) return res.status(404).json({
        success: false,
        value: null
    });

    // Get temperature history
    const response = await historyManagement.getTemperatureHistory(req.params.device);
    // Get code and success
    const code = response ? 200 : 404;
    
    // Reply
    return res.status(code).json({
        success: true,
        value: response,
    });
});
// Status informations about APPARENT TEMPERATURE
app.get('/history/:device/apparent-temperature', async function (req, res) { // Apparent Temperature

    // Error if INFLUX is not connected
    if (!historyManagement.isConnected()) return res.status(404).json({
        success: false,
        value: null
    });

    // Get apparent temperature history
    const response = await historyManagement.getApparentTemperatureHistory(req.params.device);
    // Get code and success
    const code = response ? 200 : 404;
    
    // Reply
    return res.status(code).json({
        success: true,
        value: response,
    });
});
// Status informations about HUMIDITY
app.get('/history/:device/humidity', async function (req, res) { // Humidity

    // Error if INFLUX is not connected
    if (!historyManagement.isConnected()) return res.status(404).json({
        success: false,
        value: null
    });

    // Get humidity history
    const response = await historyManagement.getHumidityHistory(req.params.device);
    // Get code and success
    const code = response ? 200 : 404;
    
    // Reply
    return res.status(code).json({
        success: true,
        value: response,
    });
});

// Export app
module.exports.webserver = app;
module.exports.startWebServer = function(port = 3000) { // Start webserver
    // Listen
    app.listen(port);
};
