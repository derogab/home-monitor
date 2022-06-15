// Import Express
const express = require('express');
const cors = require('cors');
const app = express();
// Import other components
const deviceManagement = require('./deviceManagement');

// Init CORS
app.use(cors({ origin: '*' }));

// Status
app.get('/', function (req, res) {
    res.status(200).send('Status: UP!');
});

// Devices
app.get('/devices', async function (req, res) {
    // Get devices list
    const result = await deviceManagement.getDevices(deviceManagement.DEVICE_TYPE_SENSOR);
    // Check if success
    const code = (result && result.success) ? 200 : 500;
    // Reply w/ results
    res.status(code).json(result);
});

// Listen
app.listen(3001);

// Export app
module.exports = app;
