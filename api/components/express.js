// Import Express
const express = require('express');
const cors = require('cors');
const app = express();

// Init CORS
app.use(cors({ origin: '*' }));

// Status
app.get('/', function (req, res) {
    res.status(200).send('Status: UP!');
});

// Listen
app.listen(3001);

// Export app
module.exports = app;
