from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

def main():
    influxClient = InfluxDBClient.from_config_file("influx_config.ini")
    buckets_api = influxClient.buckets_api()

    bucket_name = "test-py-bucket"

    if buckets_api.find_bucket_by_name(bucket_name) is None:
        buckets_api.create_bucket(bucket_name=bucket_name)

    write_api = influxClient.write_api(write_options=SYNCHRONOUS)
    query_api = influxClient.query_api()
    p = Point("my_measurement").tag("location", "Prague").field("temperature", 28.3)
    write_api.write(bucket=bucket_name, record=p)



if __name__ == "__main__":
    main()