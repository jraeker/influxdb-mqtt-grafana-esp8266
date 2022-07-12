from paho.mqtt import client as mqtt_client
import json
import logging
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from datetime import datetime
import os

token = os.getenv("INFLUX_TOKEN", "error")
org = os.getenv("INFLUX_ORG", "error")
bucket = os.getenv("INFLUX_BUCKET", "error")
influx_address = "http://influxdb:8086"

topic = os.environ["MQTT_TOPIC"]
username = os.environ["MQTT_USER"]
password = os.environ["MQTT_PASS"]

print("Token:", token)
print("org", org)
print("bucket", bucket)
def sendDataToInflux(jmsg):
    with InfluxDBClient(url=influx_address, token=token, org=org) as influx_client:
        write_api = influx_client.write_api(write_options=SYNCHRONOUS)
        try:
            point = Point("data") \
                .tag("id", jmsg.get("id")) \
                .tag("env", jmsg.get("env")) \
                .field("lon", float(jmsg.get("lon"))) \
                .field("lat", float(jmsg.get("lat"))) \
                .field("temp", float(jmsg["dht"].get("temp"))) \
                .field("hum", float(jmsg["dht"].get("hum"))) \
                .time(datetime.utcfromtimestamp(jmsg.get("time")))
            if jmsg["sds011"].get("pm10") != -1:
                point.field("pm10", float(jmsg["sds011"].get("pm10")))
            if jmsg["sds011"].get("pm25") != -1:
                point.field("pm25", float(jmsg["sds011"].get("pm25")))
            print("Point:", point)
            write_api.write(bucket, org, point)
        except Exception as e:
            logging.error(e)
            logging.error(jmsg)

        influx_client.close()




def on_message(client, userdata, message):
    print("msg received:", message.payload)
    js_msg = json.loads(message.payload)
    sendDataToInflux(js_msg)

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT-Server")
    client.subscribe(topic)
    print("Subscribed to topic", topic)

client = mqtt_client.Client()
client.username_pw_set(username, password)
client.on_connect = on_connect
client.on_message = on_message

client.connect("mosquitto", 1883)
client.loop_forever()
