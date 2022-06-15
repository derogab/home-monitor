// Logger
const logger = require('../utils/logger');

// Init global data variables defaults
const mqtt_data_fire_default = false;
const mqtt_data_light_default = false;
const mqtt_data_temperature_default = 0;
const mqtt_data_apparent_temperature_default = 0;
const mqtt_data_humidity_default = 0;
// Init global data variables array
let data = [];

// Init data types
const FIRE = module.exports.FIRE = 'fire';
const LIGHT = module.exports.LIGHT = 'light';
const TEMPERATURE = module.exports.TEMPERATURE = 'temp';
const HUMIDITY = module.exports.HUMIDITY = 'hum';
const APPARENT_TEMPERATURE = module.exports.APPARENT_TEMPERATURE = 'atemp';

// Get data
const _get = function(device, type) {
    
    // Search in data
    for (let i = 0; i < data.length; i++) {
        const element = data[i];
        // Return element if found
        if (element.type == type && element.device == device) return element.value;
    }

    // Return default data instead of null
    switch (type) {
        case FIRE:
            return mqtt_data_fire_default;
        case LIGHT:
            return mqtt_data_light_default;
        case TEMPERATURE:
            return mqtt_data_temperature_default;
        case APPARENT_TEMPERATURE:
            return mqtt_data_apparent_temperature_default;
        case HUMIDITY:
            return mqtt_data_humidity_default;
        default:
            return null;
    }
};

// Set data
const _set = function(device, type, value) {

    // Search in data
    for (let i = 0; i < data.length; i++) {
        const element = data[i];
        // If found update the value
        if (element.type == type && element.device == device) {
            // Update the value
            element.value = value;
            return;
        }
    }

    // Save data
    data.push({
        type: type,
        device: device,
        value: value, 
    });
    
};


// Export
module.exports.get = _get;
module.exports.set = _set;
