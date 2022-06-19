// Import dependencies
const {InfluxDB, FluxTableMetaData} = require('@influxdata/influxdb-client');
// Import utils
const logger = require('../utils/logger');

// Init ENVs
require('dotenv').config();

// Get configs
const influx_host = process.env.INFLUX_HOST;
const influx_port = process.env.INFLUX_PORT;
const influx_token = process.env.INFLUX_TOKEN;
const influx_org = process.env.INFLUX_ORG;

// Init influx
let queryApi = null;

// Connect to influx
module.exports.connectToDatabase = function() {
    // Log
    logger.debug('Connecting to Influx DB...');
    // Get URL
    const influx_url = 'http://' + influx_host + ':' + influx_port;
    logger.debug('Influx URL: ' + influx_url);
    // Init influx connection
    queryApi = new InfluxDB({
        url: influx_url, 
        token: influx_token
    }).getQueryApi(influx_org);
    // Log
    logger.info('Connected to Influx DB!');
};

// Check if is connected
module.exports.isConnected = function() {
    return queryApi ? true : false;
};

// Download fire data for a device
const getGenericHistory = async function(device, what) {
    // Write query
    const fluxQuery = 'from(bucket: "home-monitor-logs")  |> range(start: -60m, stop: now())  |> filter(fn: (r) => r["_measurement"] == "'+device+'")  |> filter(fn: (r) => r["_field"] == "'+what+'")  |> keep(columns: ["_time", "_value"])';
    // Get data
    // Resolve Promise
    return new Promise((resolve) => {

        queryApi
        .collectRows(fluxQuery /*, you can specify a row mapper as a second arg */)
        .then(data => {
            let result_data = [];
            let cont = 0;
            for (let i = 0; i < data.length; i++) {
                const element = data[i];
                result_data.push({
                    time: element._time,
                    value: element._value,
                });
                cont += 1;
            }
            // Log
            logger.debug('InFlux Query downloaded ' + cont + ' rows.');
            // Resolve data
            resolve(result_data);
        })
        .catch(error => {
            // Log
            logger.error('InFlux Query Error: ' + error);
            // No data downloaded
            resolve([]);
        });

    });
};


module.exports.getFireHistory = async function(device) {
    // Use generic function to reply
    return await getGenericHistory(device, 'flame');
}
module.exports.getLightHistory = async function(device) {
    // Use generic function to reply
    return await getGenericHistory(device, 'light');
}
module.exports.getTemperatureHistory = async function(device) {
    // Use generic function to reply
    return await getGenericHistory(device, 'temperature');
}
module.exports.getApparentTemperatureHistory = async function(device) {
    // Use generic function to reply
    return await getGenericHistory(device, 'apparent_temperature');
}
module.exports.getHumidityHistory = async function(device) {
    // Use generic function to reply
    return await getGenericHistory(device, 'humidity');
}
