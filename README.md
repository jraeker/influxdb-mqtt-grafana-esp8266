# Influx-MQTT-Grafana Stack for ESP8266-Sensor


Docker-Compose Stack with a InfluxDB and MQTT-Server (Mosquitto) and Grafana for monitoring the data. In this project, an ESP8266 was used to measure **temperature** and **humidity** using DHT and **air quality** (PM 10 and PM 2.5) using SDS011. The code for the ESP can be found in the "sensor" directory.

The picture below shows how all the components work together.

![diagram](./diagram.png?raw=true "diagram")

---
# Setup
For the complete stack to work reasonably, it must first be configured.
The following steps must be followed:

### Step 1: Start docker compose
- run ```docker-compose up``` in this directory

### Step 2: Mosquitto configuration:
- add the *mosquitto.conf* (see below) config to ./mosquitto/config/mosquitto.conf
- create the user "admin" and "guest"
  1. ```docker exec -it {MOS_CONTAINER} sh```
  2. ```mosquitto_passwd /mosquitto/config/pwfile {USERNAME}```
       - use flag ```-c ```, if file "pwfile" does not exist yet
- add the acl_file *user_perm* (see below) to configure the permissions. Change {TOPIC_NAME} to the topic you want to write on
- restart mosquitto
  - ```docker restart mosquitto```


##### mosquitto.conf:
```
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
acl_file /mosquitto/config/user_perm

password_file /mosquitto/config/pwfile
allow_anonymous false
listener 1883

```

##### user_perm:
```
user admin
topic readwrite {TOPIC_NAME}

user guest
topic read {TOPIC_NAME}
```


### Step 3: Influx configuration:
1. open {host}:8086 in browser
2. create a user
3. create a bucket

### Step 4: Add environmental variables to *docker-compose.yml*
1. MQTT_USER and MQTT_PASS are the created guest user with the password
2. MQTT_TOPIC is the topic you want to write data to
3. Add the Influx organization and bucket you created in step 3
4. add the influx token, to give the mqtt-bridge permission to write
   - can be found in the influx-dashboard (data -> API Tokens)

### Step 5: Restart docker-compose
- run ```docker-compose restart```

### Step 6: Configure Grafana
To display the data in grafana you need to your influxdb as datasource
1. open grafana in your browser (on port 3000)
2. configuration -> Data sources -> Add data source
3. search for "InfluxDB"
4. use "Flux" as query language
5. add "URL"
6. add your InfluxDB details (Organization, Token and Bucket)

---

### mqtt sensor data format
The sensordata are send as json to the mqtt broker. The data will look like this:
```
{
   "id":"testid12356",
   "lon":123.123456,
   "lat":123.123456,
   "time":1654174824,
   "sds011":{
      "pm10":4.400000095,
      "pm25":0.899999976
   },
   "dht":{
      "temp":22.10000038,
      "hum":76.80000305
   }
}

```
