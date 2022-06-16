// Logger
const logger = require('../utils/logger');

// Import dependencies
const redis = require('redis');

// Init ENVs
require('dotenv').config();

// Init data types
const FIRE = module.exports.FIRE = 'fire';
const LIGHT = module.exports.LIGHT = 'light';
const TEMPERATURE = module.exports.TEMPERATURE = 'temp';
const HUMIDITY = module.exports.HUMIDITY = 'hum';
const APPARENT_TEMPERATURE = module.exports.APPARENT_TEMPERATURE = 'atemp';

// Init redis client
const client = redis.createClient({ 
    url: 'redis://' + process.env.REDIS_HOST + ':' + process.env.REDIS_PORT,
});

// Init redis callback logger
client.on('error', (err) => logger.error('There was an error with Redis. ' + err));
client.on('connect', () => logger.info('Successfully connected to Redis!'));

// Connection
module.exports.connect = async function () {
    // Try to connect to redis
    logger.debug('Trying to connect to Redis (host = ' + process.env.REDIS_HOST + ', port = ' + process.env.REDIS_PORT + ')');
    // Redis connection
    await client.connect();
}

// Utils: generate multi key for redis
const generate_multi_key = function(key1, key2) {
    return key1 + '-' + key2;
};

// Set & Get for FIRE
module.exports.getFire = async function(device) {
    // Get from redis
    return (await client.get(generate_multi_key(device, FIRE)) == 'YES') ? true : false;
};
module.exports.setFire = async function(device, value) {
    // Save to redis
    await client.set(generate_multi_key(device, FIRE), value ? 'YES' : 'NO');
}

// Set & Get for LIGHT
module.exports.getLight = async function(device) {
    // Get from redis
    return (await client.get(generate_multi_key(device, LIGHT)) == 'YES') ? true : false;
};
module.exports.setLight = async function(device, value) {
    // Save to redis
    await client.set(generate_multi_key(device, LIGHT), value ? 'YES' : 'NO');
}

// Set & Get for TEMPERATURE
module.exports.getTemperature = async function(device) {
    // Get from redis
    const redisData = await client.get(generate_multi_key(device, TEMPERATURE));
    // Check if isset
    if (!redisData) return null; // return default
    // Return the parsed value
    return parseFloat(redisData);
};
module.exports.setTemperature = async function(device, value) {
    // Save to redis
    await client.set(generate_multi_key(device, TEMPERATURE), ''+value);
};

// Set & Get for APPARENT TEMPERATURE
module.exports.getApparentTemperature = async function(device) {
    // Get from redis
    const redisData = await client.get(generate_multi_key(device, APPARENT_TEMPERATURE));
    // Check if isset
    if (!redisData) return null; // return default
    // Return the parsed value
    return parseFloat(redisData);
};
module.exports.setApparentTemperature = async function(device, value) {
    // Save to redis
    await client.set(generate_multi_key(device, APPARENT_TEMPERATURE), ''+value);
};

// Set & Get for HUMIDITY
module.exports.getHumidity = async function(device) {
    // Get from redis
    const redisData = await client.get(generate_multi_key(device, HUMIDITY));
    // Check if isset
    if (!redisData) return null; // return default
    // Return the parsed value
    return parseFloat(redisData);
};
module.exports.setHumidity = async function(device, value) {
    // Save to redis
    await client.set(generate_multi_key(device, HUMIDITY), ''+value);
};
