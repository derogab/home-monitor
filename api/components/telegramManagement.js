// Dependencies
const { Telegraf, Markup } = require('telegraf');
// Import utils
const logger = require('../utils/logger');
// Import other components
const deviceManagement = require('./deviceManagement');

// Init ENVs
require('dotenv').config();

// Get configs
const bot_token = process.env.TELEGRAM_BOT_TOKEN;

// Init telegram bot
const bot = new Telegraf(bot_token);

// Starting bot
module.exports.startBot = function() {
    // Log
    logger.debug('Starting telegram bot...');
    // Launch the bot
    bot.launch();
};

// Simple conversational commands
const help_message = function() {
    // Write help message
    msg  = "All available commands: \n\n";
    msg += "/start - the welcome command\n";
    msg += "/help - the help command\n";
    msg += "/devices - all the available devices\n";
    // Return the help message
    return msg;
}
const welcome = function(ctx) {
    // Reply w/ welcome message
    ctx.reply("Welcome to *Home Monitor*!", { parse_mode: 'Markdown' });
    // Send also the help message
    ctx.reply(help_message(), { parse_mode: 'Markdown' });
}
const help = function(ctx) {
    // Reply w/ help message
    ctx.reply(help_message(), { parse_mode: 'Markdown' });
}

// Devices
const get_devices = async function(ctx) {
    // Get devices list
    const result = await deviceManagement.getDevices(deviceManagement.DEVICE_TYPE_SENSOR);
    // Check if not success
    if (!result || !result.success) {
        // Return the fail result
        return ctx.reply('Something went wrong. Retry later...');
    }
    // If there are no devices
    if (!result || !result.success) {
        // Return the fail result
        return ctx.reply('No device found!');
    }
    // If success and there is almost one device, list them
    // Prepare keyboard w/ list
    const keyboard = [];
    for (let i = 0; i < result.devices.length; i++) {
        const device = result.devices[i];
        keyboard.push([
            { text: device.name, callback_data: device.mac }
        ]);
    }
    // Reply with list
    return ctx.reply('Select a device:', {
            reply_markup: JSON.stringify({ inline_keyboard: keyboard })
        }
    );
}



// Router
bot.start((ctx) => welcome(ctx));
bot.help((ctx) => help(ctx));
bot.command('devices', (ctx) => get_devices(ctx));