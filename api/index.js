// Import utils
const logger = require("./utils/logger");
// Import components management
const apiManagement = require("./components/apiManagement");
const dataManagement = require("./components/dataManagement");
const telegramManagement = require("./components/telegramManagement");
const cronManagement = require("./components/cronManagement");
const listenerManagement = require('./components/listenerManagement');

// Init ENVs
require('dotenv').config();

// Starting 
logger.info('Starting API...');

// Start modules
dataManagement.connect();
apiManagement.startWebServer(3001);
telegramManagement.startBot();
cronManagement.startCron();
listenerManagement.startListener();
