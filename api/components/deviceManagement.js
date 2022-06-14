// Logger
const logger = require('../utils/logger');

// Import MySQL
const mysql = require('mysql');

// Init ENVs
require('dotenv').config();

// Get configs
const mysql_host = process.env.MYSQL_HOST;
const mysql_port = process.env.MYSQL_PORT;
const mysql_user = process.env.MYSQL_USERNAME;
const mysql_pass = process.env.MYSQL_PASSWORD;
const mysql_database = process.env.MYSQL_DATABASE;

// Init MYSQL client 
const mysqlClient = mysql.createConnection({
    host     : mysql_host || 'localhost',
    port     : mysql_port || 3306,
    user     : mysql_user,
    password : mysql_pass,
    database : mysql_database
});


// Server connection
const _connect = async function() {
    // Log
    logger.debug('Connecting to MYSQL server...');
    // Resolve Promise
    return new Promise((resolve) => {

        // Try to connect to MYSQL server
        logger.debug('MYSQL server: ' + mysql_host + ':' + mysql_port);
        mysqlClient.connect(function(err) {
            // Failed
            if (err) {
                logger.error('MySQL Connecting Error: ' + err.stack);
                resolve(mysqlClient);
            }
            // Connected
            logger.info('Connected to MYSQL server as id ' + mysqlClient.threadId);
            resolve(mysqlClient);
        });
    });
};

// Devices
const _getDevices = async function() {

    // Connection
    while(!mysqlClient) {
        logger.debug('Connect to MYSQL server to download the devices...');
        // Try to connect
        mysqlClient = await _connect();
    }

    // Logs
    logger.debug('Trying to get the sensors list...');

    // Resolve Promise
    return new Promise((resolve) => {
        // Download devices
        mysqlClient.query("SELECT NAME AS name, MAC_ADDRESS as mac FROM home_monitor_devices WHERE TYPE = 'sensors'", function (error, results, fields) {
            if (error) {
                // Error
                logger.error('Error getting sensors list.');
                // Response
                resolve({ sensors: [], success: false });
            }
            // Log
            logger.debug((results ? results.length : 0) + ' sensor(s) found!');
            // Return data
            resolve({ sensors: results, success: true });
        });
    });

};


// Export
module.exports.getDevices = _getDevices;