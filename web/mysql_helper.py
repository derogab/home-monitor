import pymysql.cursors
import secrets

USER_DB = secrets.MYSQL_ID
PASSWORD_DB = secrets.MYSQL_PW
HOST_DB = secrets.IOT_HOST
PORT_DB = 3306
NAME_DB = secrets.MYSQL_ID

def connect_db():
    conn = pymysql.connect(user=USER_DB,
                           password=PASSWORD_DB,
                           host=HOST_DB,
                           port=int(PORT_DB),
                           database=NAME_DB,
                           cursorclass=pymysql.cursors.DictCursor)

    return conn

def get_all_devices():
    conn = connect_db()
    with conn.cursor() as cursor:
        sql = "SELECT * FROM `home-monitor-devices`"
        cursor.execute(sql)
        result = cursor.fetchall()
        devices = []
        row: dict
        for row in result:
            devices.append((row['MAC_ADDRESS'], row['NAME'], row['TYPE']))
        return devices

def exist_device_in_db(mac_address):
    conn = connect_db()
    with conn.cursor() as cursor:
        sql = "SELECT MAC_ADDRESS FROM `home-monitor-devices` WHERE MAC_ADDRESS=%s"
        cursor.execute(sql, (mac_address,))
        result = cursor.fetchall()
        return bool(len(result) == 1)

def add_device_to_db(mac_address, name, dev_type):
    if exist_device_in_db(mac_address):
        print("Device already in db")
        return
    conn = connect_db()
    with conn.cursor() as cursor:
        sql = "INSERT INTO `home-monitor-devices` VALUES(%s, %s, %s)"
        cursor.execute(sql, (mac_address, name, dev_type,))
        conn.commit()
        return






