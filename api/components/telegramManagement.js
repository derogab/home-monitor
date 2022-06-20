// Dependencies
const { Telegraf } = require('telegraf');
const sleep = require('sleep');
// Import utils
const logger = require('../utils/logger');
// Import other components
const deviceManagement = require('./deviceManagement');
const dataManagement = require('./dataManagement');

// Init ENVs
require('dotenv').config();
// Get configs
const bot_token = process.env.TELEGRAM_BOT_TOKEN;
// Constant
const CALLBACK_PREFIX_DEVICE = 'device_';
const CALLBACK_PREFIX_ALARM = 'alarm_';

// Init telegram bot
const bot = new Telegraf(bot_token);

// Starting bot
const startBot = function() {
    // Log
    logger.debug('Starting telegram bot...');
    // Launch the bot
    bot.launch();
};

// Set live data
const setLiveData = async function(user, device, message_id) {
    // Search if user is in users
    const users = await dataManagement.getAllUsers();
    let found = false;
    for (let i = 0; i < users.length; i++) if (users[i] == user) found = true;
    if (!found) {
        // Add new user
        users.push(user);
        // Update users
        await dataManagement.setAllUsers(users);
    }
    // Add / Update other data
    await dataManagement.setLastDeviceByUser(user, device);
    await dataManagement.setLastMessageIdByUser(user, message_id);
};

// Generate device status message
const generate_device_status_message = async function(device) {
    // Get float data
    const temperature = await dataManagement.getTemperature(device);
    const humidity = await dataManagement.getHumidity(device);
    const apparent_temperature = await dataManagement.getApparentTemperature(device);

    // Generate message
    let device_data_msg  = '🌐 MAC Address: *' + device + '*\n\n';
        device_data_msg += '🔥 Fire: ' + ((await dataManagement.getFire(device) ? 'YES' : 'NO')) + '\n';
        device_data_msg += '🕯 Light: ' + ((await dataManagement.getLight(device) ? 'YES' : 'NO')) + '\n\n';
        device_data_msg += '🌡 Temperature: ' + (temperature ? (temperature + '° C') : 'N/A') + '\n';
        device_data_msg += '💧 Humidity: ' + (humidity ? (humidity + '%') : 'N/A') + '\n';
        device_data_msg += '🥵 Apparent Temperature: ' + (apparent_temperature ? (apparent_temperature + '° C') : 'N/A') + '\n\n';
        device_data_msg += '🕙 Last update: ' + new Date().toISOString();
    
    // Return the generated message
    return device_data_msg;
}

// Simple conversational commands
const help_message = function() {
    // Write help message
    let msg = "";
    msg += "📃 All available commands: \n\n";
    msg += "/start - the welcome command\n";
    msg += "/help - the help command\n";
    msg += "/get\_devices - get all the available devices\n";
    msg += "/get\_device\_info - get info about a selectable device\n";
    msg += "/setup\_alarm - setup the alarm\n";
    // Return the help message
    return msg;
}
const welcome = async function(ctx) {
    // Reply w/ welcome message
    await ctx.reply("👋 Welcome to *Home Monitor*!", { parse_mode: 'Markdown' });
    // Sleep
    sleep.sleep(2);
    // Send also the help message
    await ctx.reply(help_message());
}
const help = async function(ctx) {
    // Reply w/ help message
    await ctx.reply(help_message());
}

// Get all devices
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
    let reply_msg = "📃 Available devices: \n\n";
    // Prepare keyboard w/ list
    for (let i = 0; i < result.devices.length; i++) {
        const device = result.devices[i];
        reply_msg += "• " + device.name + " - " + device.mac + "\n";
    }
    // Reply with list
    await ctx.reply(reply_msg);
    // And then extra info
    return ctx.reply("If you want to see specific informations about a device use /get\_device\_info");
}

// Get specific device info
const get_device_info = async function(ctx) {
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
            { text: device.name, callback_data: (CALLBACK_PREFIX_DEVICE + device.mac) }
        ]);
    }
    // Reply with list
    return ctx.reply('Select a device:', {
            reply_markup: JSON.stringify({ inline_keyboard: keyboard })
        }
    );
}

// Setup alarm mode
const setup_alarm = async function(ctx) {
    // Generate message
    let msg  = "🔔 *Alarm Setup*\n\n";
        msg += "If the alarm is enabled you will receive notifications whenever a fire is detected."
    // Generate buttons
    const keyboard = [[
        { text: "🔔 Enable", callback_data: CALLBACK_PREFIX_ALARM + 'on' },
        { text: "🔕 Disable", callback_data: CALLBACK_PREFIX_ALARM + 'off' },
    ]];
    // Reply with list
    return ctx.reply(msg, {
            parse_mode: 'Markdown',
            reply_markup: JSON.stringify({ inline_keyboard: keyboard })
        }
    );
}

// Update all live status 
const updateLiveStatus = async function() {
    // Live status counter
    let cont = 0;
    // Get users
    const users = await dataManagement.getAllUsers();
    // Search live status to update
    for (let i = 0; i < users.length; i++) {
        // Get data
        const user = users[i];
        const device = await dataManagement.getLastDeviceByUser(user);
        const message_id = await dataManagement.getLastMessageIdByUser(user);

        // Generate message
        const msg_text = await generate_device_status_message(device);

        // Update live status
        try {
            bot.telegram.editMessageText(user, message_id, message_id, msg_text, { parse_mode: 'Markdown' });
        } catch (error) {
            logger.error('Failed to update live status for user ' + user);
        }
        // Update counter 
        cont += 1;
    }
    // Log
    if (cont > 0) logger.debug('Live status (telegram) updated: ' + cont + ' active.');
}

// Send FIRE alarm message to user
const sendFireAlarmMessage = async function(user, msg) {
    // Check if bot is OK
    if (!bot) return;
    // Send message
    await bot.telegram.sendMessage(user, '🔥', { parse_mode: 'Markdown' });
    await bot.telegram.sendMessage(user, msg, { parse_mode: 'Markdown' });
}

// Router
bot.start((ctx) => welcome(ctx));
bot.help((ctx) => help(ctx));
bot.command('get_devices', (ctx) => get_devices(ctx));
bot.command('get_device_info', (ctx) => get_device_info(ctx));
bot.command('setup_alarm', (ctx) => setup_alarm(ctx));

// Callbaks
bot.on('callback_query', async (ctx) => {
    // Get callback query
    const query = ctx.callbackQuery;
    // Get callback value of button clicked
    const op = query.data;
    // Get user
    const user = query.from.id;
    const username = query.from.username || null;
    // Search type of value
    if (op.includes(CALLBACK_PREFIX_DEVICE)) {
        // Get device
        const device = op.replace(CALLBACK_PREFIX_DEVICE, '');
        // Delete selection message
        try {
            await ctx.deleteMessage();
        } catch (error) {
            logger.error('Impossible to delete old telegram message w/ selectors');
        }
        // Selected device popup
        await ctx.answerCbQuery('Device ' + device + ' selected!');
        // Generate message
        const msg_text = await generate_device_status_message(device);
        // Reply w/ selected device data status
        const msg = await ctx.reply(msg_text, { parse_mode: 'Markdown' });
        // Set live data
        await setLiveData(msg.chat.id, device, msg.message_id);
    }
    else if (op.includes(CALLBACK_PREFIX_ALARM)) {
        // Get status
        const status = op.replace(CALLBACK_PREFIX_ALARM, '');
        // Get text status
        const status_text = status == 'on' ? 'enabled' : 'disabled';
        // Status popup
        await ctx.answerCbQuery('Alarm ' + status_text + '!');
        // Get status as boolean
        const statusBool = (status == 'on') ? true : false;
        // Set selected status
        dataManagement.setAlarmSetup(user, statusBool);
        // Reply
        await ctx.editMessageText('🔔 Alarm ' + status_text + '!', { parse_mode: 'Markdown' });
        // Log 
        logger.info('User ' + user + ' ' + status_text + ' the alarm.');
        // Sleep
        sleep.sleep(2);
        // If enabled, recommends enabling the other plugin
        if (statusBool) await ctx.reply('Alarm calls can also be enabled. Authorize the _calling bot_ [by clicking here](https://api2.callmebot.com/txt/login.php) and connecting your telegram account.', { parse_mode: 'Markdown' });
        // Save telegram user
        if (username) await dataManagement.setTelegramUsername(user, username);
    }
});

// Exports
module.exports.startBot = startBot;
module.exports.updateLiveStatus = updateLiveStatus;
module.exports.sendFireAlarmMessage = sendFireAlarmMessage;
