FROM python:3.7-alpine
RUN pip install influxdb-client
RUN pip install paho-mqtt
COPY . /code
WORKDIR /code
CMD ["python3", "-u", "mqtt_influx_bridge.py"]
