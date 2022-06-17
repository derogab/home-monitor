// Dependencies
const got = require('got');
// Import utils
const logger = require('../utils/logger');
// Import other components
const dataManagement = require('./dataManagement');
const telegramManagement = require('./telegramManagement');

// Alarm callback
module.exports.trigger = async function(device) { // Alarm triggered
    // Search all users
    const users = await dataManagement.getAllUsers();
    // Get last triggered
    const lastTriggered = await dataManagement.getLastAlarmTriggerTime();
    // Check if trigged in the last 2 minutes
    const now = new Date();
    let isAlmostTwoMinutes = false;
    if (!lastTriggered) isAlmostTwoMinutes = true;
    else if(((now.getTime() - lastTriggered.getTime()) / 1000) > 120) isAlmostTwoMinutes = true;

    // If is almost two minutes after the last trigger
    if (isAlmostTwoMinutes || true) {
        // For each users, trigger chat alarm if enabled
        for (let i = 0; i < users.length; i++) {
            const user = users[i];
            // Trigger alarm only if enabled
            if (await dataManagement.getAlarmSetup(user)) {
                // Generate text
                let msg_text  = '*Fire detected!*\n\n';
                    msg_text += 'Device: _' + device + '_\n';
                    msg_text += 'Time: _' + (new Date().toISOString()) + '_';
                // Send simple message
                //await telegramManagement.sendFireAlarmMessage(user, msg_text);
            }
        }
        // For each users, trigger alarm if enabled
        for (let i = 0; i < users.length; i++) {
            const user = users[i];
            // Trigger alarm only if enabled
            if (await dataManagement.getAlarmSetup(user)) {
                // Get telegram username
                const username = await dataManagement.getTelegramUsername(user) || null;
                // Generate text
                const msg_voice = 'Attention please! Fire detected with a device. Control the chat for more information.';
                // Send call request
                if (username) {
                    // Log
                    logger.info('Send an Alarm Call to user ' + user + ' (@' + username + ') for fire in ' + device);
                    // Send
                    try {
                        const r = await got.get('https://api.callmebot.com/start.php?user=@' + username + '&text=' + encodeURI(msg_voice) + '&lang=en&cc=missed');
                        //console.log(r);
                    } catch (error) {
                        console.log(error);
                    }
                    // Save trigger time
                    await dataManagement.setLastAlarmTriggerTime();
                }
                else {
                    // Log
                    logger.warning('Impossible to send an Alarm Call to user ' + user + ' because there aren\'t a username.');
                }
            }
        }
    }
    else {
        // Log
        logger.debug('There is fire but users have been contacted in the last 2 minutes.');
    }
};
