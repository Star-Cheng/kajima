import pymysql
from common import Snowflake
import time
from datetime import datetime

def get_db_connection():
    connection = pymysql.connect(host='127.0.0.1', port=3306, user='hms', password='edge@2021', db='seibertron')
    return connection

def get_maps():
    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute('select id, name, path, create_time, type, is_delete from sys_map')
    maps = cur.fetchall()
    print(maps)
    map_array = []
    for map in maps:
        map_data = {
            'id': map[0],
            'name': map[1],
            'path': map[2],
            'create_time': map[3].strftime('%Y-%m-%d %H:%M:%S'),
            'type': map[4],
            'is_delete': map[5]
        }
        map_array.append(map_data)
    connection.close()
    return map_array


def insert_map(map_data):
    delete_map(map_data['id'])
    connection = get_db_connection()
    cur = connection.cursor()
    sql = "INSERT INTO sys_map (id, name, path, create_time, type, is_delete) VALUES (%s, %s, %s, %s, %s, %s)"
    cur.execute(sql, [map_data['id'], str(map_data['id']) + '号地图', map_data['path'], datetime.now().strftime('%Y-%m-%d %H:%M:%S'), '1', '0'])
    connection.commit()
    connection.close()

def update_map_type(map_number, type):
    connection = get_db_connection()
    cur = connection.cursor()
    sql = "UPDATE sys_map SET type = %s WHERE id = %s"
    cur.execute(sql, [type, map_number])
    connection.commit()
    connection.close()


def delete_map(id):
    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute("DELETE FROM sys_map WHERE id = %s", [id])
    connection.commit()
    connection.close()
    return True

