import secrets
import mysql_helper
import paho.mqtt.client as mqtt
import json

client = mqtt.Client()

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("unishare/devices/connect")
    client.subscribe("unishare/devices/get_all")

def on_message(client, userdata, msg):
    print(msg.topic)
    if msg.topic == "unishare/devices/connect":
        x = json.loads(msg.payload.decode("utf-8"))
        mysql_helper.add_device_to_db(x["MAC_ADDRESS"], x["NAME"], x["TYPE"])
        return
    if msg.topic == "unishare/devices/get_all":
        devices = mysql_helper.get_all_devices()

        jsondata = []
        for device in devices:
            device_dict={}
            device_dict["MAC_ADDRESS"] = device[0]
            device_dict["NAME"] = device[1]
            device_dict["TYPE"] = device[2]
            jsondata.append(device_dict)
        
        jsondata = json.dumps(jsondata)
        client.publish("unishare/devices/all_devices", payload=jsondata, qos=2, retain=True)
        
def main():
    client.username_pw_set(secrets.MQTT_ID, password=secrets.MQTT_PW)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(secrets.IOT_HOST, 1883, 60)
    client.loop_forever()

if __name__ == "__main__":
    main()