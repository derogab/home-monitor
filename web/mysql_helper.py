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
        sql = "SELECT * FROM `home_monitor_devices`"
        cursor.execute(sql)
        result = cursor.fetchall()
        devices = []
        row: dict
        for row in result:
            devices.append((row['MAC_ADDRESS'], row['NAME'], row['TYPE']))
        return devices

def get_all_sensors():
    conn = connect_db()
    with conn.cursor() as cursor:
        sql = "SELECT * FROM `home_monitor_devices` WHERE (lower(TYPE) LIKE 'sensors');"
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
        sql = "SELECT MAC_ADDRESS FROM `home_monitor_devices` WHERE MAC_ADDRESS=%s"
        cursor.execute(sql, (mac_address,))
        result = cursor.fetchall()
        return bool(len(result) == 1)

def add_device_to_db(mac_address, name, dev_type):
    conn = connect_db()
    if exist_device_in_db(mac_address):
        print("Device ", mac_address, " already in db, with updating name ", name)
        with conn.cursor() as cursor:
            sql = "UPDATE `home_monitor_devices` SET `NAME` = %s WHERE `home_monitor_devices`.`MAC_ADDRESS` = %s;"
            cursor.execute(sql, (name, mac_address))
            conn.commit()
            return True
    else:
        print("Adding device ", mac_address, " with name ", name, " type ", dev_type)      
        with conn.cursor() as cursor:
            sql = "INSERT INTO `home_monitor_devices` VALUES(%s, %s, %s, TRUE)"
            cursor.execute(sql, (mac_address, name, dev_type,))
            conn.commit()
            return True

def update_device_status_db(mac_address, status):
    conn = connect_db()
    if exist_device_in_db(mac_address):
        print("Updating device ", mac_address, " status: ", status)
        with conn.cursor() as cursor:
            if (status):
                sql = "UPDATE `home_monitor_devices` SET `connected` = 1 WHERE `home_monitor_devices`.`MAC_ADDRESS` = %s;"
            else:
                sql = "UPDATE `home_monitor_devices` SET `connected` = 0 WHERE `home_monitor_devices`.`MAC_ADDRESS` = %s;"
            cursor.execute(sql, (mac_address))
            conn.commit()
            return True






