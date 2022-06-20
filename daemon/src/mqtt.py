import paho.mqtt.client as mqtt
import secrets
import json

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("unishare/test")

def on_message(client, userdata, msg):
    print(msg.topic)
    x = json.loads(msg.payload.decode("utf-8"))
    print(x["ledstatus"])

def main():
    client = mqtt.Client()
    client.username_pw_set(secrets.MQTT_ID, password=secrets.MQTT_PW)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_URL, 1883, 60)
    client.loop_forever()

if __name__ == "__main__":
    main()