from operator import add
from posixpath import split
import secrets
import mysql_helper
import paho.mqtt.client as mqtt
import json

client = mqtt.Client()

def json_all_sensors():
    devices = mysql_helper.get_all_sensors()
    jsondata = []
    
    for device in devices:
        device_dict = {}
        device_dict["MAC_ADDRESS"] = device[0]
        device_dict["NAME"] = device[1]
        device_dict["TYPE"] = device[2]
        jsondata.append(device_dict)

    jsondata = json.dumps(jsondata)
    return jsondata


def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("unishare/devices/setup", qos=1)
    client.subscribe("unishare/devices/sensors/#", qos = 1)


def on_message(client, userdata, msg):
    print(msg.topic)
    if msg.topic == "unishare/devices/setup":
        x = json.loads(msg.payload.decode("utf-8"))
        add_status = mysql_helper.add_device_to_db(x["mac_address"], x["name"], x["type"])
        if add_status == True:
            json_sensors = json_all_sensors()
            print(json_sensors)
            client.publish("unishare/devices/all_sensors", payload=json_sensors, qos=1, retain=True)
        return
    if msg.topic.startswith('unishare/sensors'):
        split_topic = msg.topic.split("/")
        mac = split_topic[2]
        data_type = split_topic[3]
        data_json = json.loads(msg.payload.decode("utf-8"))
        value = data_json["value"]
        print(mac)
        print(data_type)
        print(value)
        return

def main():
    client.username_pw_set(secrets.MQTT_ID, password=secrets.MQTT_PW)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(secrets.MQTT_BROKERIP, 1883, 60)
    json_sensors = json_all_sensors()
    client.publish("unishare/devices/all_sensors", payload=json_sensors, qos=1, retain=True)
    client.loop_forever()

if __name__ == "__main__":
    main()
