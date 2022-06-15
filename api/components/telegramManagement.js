// Logger
const logger = require('../utils/logger');

// Dependencies
const { Telegraf } = require('telegraf');

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

bot.start((ctx) => welcome(ctx));
bot.help((ctx) => help(ctx));

