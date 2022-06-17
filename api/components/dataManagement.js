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

// Get & Set for FIRE
module.exports.getFire = async function(device) {
    // Get from redis
    return (await client.get(generate_multi_key(device, FIRE)) == 'YES') ? true : false;
};
module.exports.setFire = async function(device, value) {
    // Save to redis
    await client.set(generate_multi_key(device, FIRE), value ? 'YES' : 'NO');
}

// Get & Set for LIGHT
module.exports.getLight = async function(device) {
    // Get from redis
    return (await client.get(generate_multi_key(device, LIGHT)) == 'YES') ? true : false;
};
module.exports.setLight = async function(device, value) {
    // Save to redis
    await client.set(generate_multi_key(device, LIGHT), value ? 'YES' : 'NO');
}

// Get & Set for TEMPERATURE
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

// Get & Set for APPARENT TEMPERATURE
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

// Get & Set for HUMIDITY
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

// Get & Set last device for user
module.exports.getLastDeviceByUser = async function(user) {
    // Get from redis
    const device = await client.get(generate_multi_key('DEVICE', user));
    // Return the value
    return device || null;
};
module.exports.setLastDeviceByUser = async function(user, device) {
    // Save to redis
    await client.set(generate_multi_key('DEVICE', user), ''+device);
};

// Get & Set last message id for user
module.exports.getLastMessageIdByUser = async function(user) {
    // Get from redis
    const msg = await client.get(generate_multi_key('MSG', user));
    // Return the value
    return msg || null;
};
module.exports.setLastMessageIdByUser = async function(user, msg) {
    // Save to redis
    await client.set(generate_multi_key('MSG', user), ''+msg);
};

// Get & Set all users
module.exports.getAllUsers = async function() {
    // Get from redis
    const users_json = await client.get('USERS');
    // Parse users
    const users = JSON.parse(users_json);
    // Return the users
    return users || [];
};
module.exports.setAllUsers = async function(users) {
    // Convert to JSON
    const users_json = JSON.stringify(users);
    // Save to redis
    await client.set('USERS', users_json);
};

// Get & Set for alarm setup status
module.exports.getAlarmSetup = async function(user) {
    // Get from redis
    return (await client.get(generate_multi_key('ALARM', user)) == 'ENABLED') ? true : false;
};
module.exports.setAlarmSetup = async function(user, value) {
    // Save to redis
    await client.set(generate_multi_key('ALARM', user), value ? 'ENABLED' : 'DISABLED');
}

// Get & Set for telegram username
module.exports.getTelegramUsername = async function(user) {
    // Get from redis
    return await client.get(generate_multi_key('USERNAME', user));
};
module.exports.setTelegramUsername = async function(user, value) {
    // Save to redis
    await client.set(generate_multi_key('USERNAME', user), value);
}

// Get & Set for last alarm trigger time
module.exports.getLastAlarmTriggerTime = async function() {
    // Get from redis
    const isoStr = await client.get('ALARM-TIME');
    // Return the date object
    return isoStr ? new Date(isoStr) : null;
};
module.exports.setLastAlarmTriggerTime = async function() {
    // Save to redis
    await client.set('ALARM-TIME', new Date().toISOString());
}
