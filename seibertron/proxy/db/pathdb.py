import pymysql
from common import Snowflake
import time
import json
from datetime import datetime

def get_db_connection():
    connection = pymysql.connect(host='127.0.0.1', port=3306, user='hms', password='edge@2021', db='seibertron')
    return connection

def get_paths(map_id):
    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute('select id, name, map_id, create_time from sys_path where map_id = %s', [map_id])
    paths = cur.fetchall()
    path_array = []
    for path in paths:
        path_data = {
            'id': path[0],
            'name': path[1],
            'map_id': path[2], 
            'create_time': path[3].strftime('%Y-%m-%d %H:%M:%S')
        }
        path_array.append(path_data)
    connection.close()
    return path_array

def get_path_json(id):

    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute('select path from sys_path where id = %s', [id])
    paths = cur.fetchall()
    path_data = []
    for path in paths:
        path_data = path[0]
    connection.close()
    return path_data

def insert_path(map_data):

    connection = get_db_connection()
    cur = connection.cursor()
    sql = "INSERT INTO sys_path (id, map_id, name, path, create_time) VALUES (%s, %s, %s, %s, %s)"
    cur.execute(sql, [map_data['id'], str(map_data['map_id']), map_data['name'], json.dumps(map_data['path']), datetime.now().strftime('%Y-%m-%d %H:%M:%S')])
    connection.commit()
    connection.close()
    return map_data


def delete_path(id):
    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute('delete from sys_path where id = %s', [id])
    connection.commit()
    connection.close()
    return True