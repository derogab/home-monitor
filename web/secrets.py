import os
from dotenv import load_dotenv

# Take environment variables from .env
load_dotenv()

# Load all the useful variables

IOT_HOST = os.environ.get('IOT_HOST')

MQTT_BROKERIP = os.environ.get('MQTT_BROKERIP')
MQTT_ID = os.environ.get('MQTT_ID')
MQTT_PW = os.environ.get('MQTT_PW')

MYSQL_ID = os.environ.get('MYSQL_ID')
MYSQL_PW = os.environ.get('MYSQL_PW')
