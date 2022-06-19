// Import other components
const telegramManagement = require('./telegramManagement');

// Init interval
let interval = null;

// Start cron
const startCron = function(cronTime = 15000) {
    // Stop previous cron
    if (interval) stopCron();
    // Start the new interval
    interval = setInterval(telegramManagement.updateLiveStatus, cronTime);
};

// Stop cron
const stopCron = function() {
    // Stop the interval
    clearInterval(interval);
    // Set interval to NULL
    interval = null;
};

// Exports
exports.startCron = startCron;
exports.stopCron = stopCron;
