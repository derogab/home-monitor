from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS


def getInfluxClient():
    influxClient = InfluxDBClient.from_config_file("influx_config.ini")
    return influxClient


def createBucketInflux(influxClient, bucket_name):
    buckets_api = influxClient.buckets_api()
    if buckets_api.find_bucket_by_name(bucket_name) is None:
        buckets_api.create_bucket(bucket_name=bucket_name)
    return

def writeDataToInflux(influxClient, bucket_name, mac, type, value):
    write_api = influxClient.write_api(write_options=SYNCHRONOUS)
    p = Point(mac).field(type, value)
    write_api.write(bucket=bucket_name, record=p)
