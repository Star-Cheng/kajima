import pymysql
from common import Snowflake


def get_db_connection():
    connection = pymysql.connect(host='127.0.0.1', port=3306, user='hms', password='edge@2021', db='seibertron')
    return connection


def get_users():
    print(Snowflake.next_id())
    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute('select id, username, password, email, role from sys_user')
    users = cur.fetchall()
    user_array = []
    for user in users:
        user_data = {
            "id": user[0],
            "username": user[1],
            "password": user[3],
            "email": user[2],
            "role": user[4]
        }
        user_array.append(user_data)
    connection.close()
    return user_array

def insert_user(user):

    user['id'] = str(Snowflake.next_id())

    connection = get_db_connection()
    cur = connection.cursor()
    sql = "INSERT INTO sys_user (id, username, password, email, role) VALUES (%s, %s, %s, %s, %s)"
    cur.execute(sql, [user['id'], user['username'], user['password'], user['email'], user['role']])
    connection.commit()
    connection.close()
    return user

def update_user(user):
    connection = get_db_connection()
    cur = connection.cursor()
    sql = "update sys_user set username = %s, password = %s, email = %s, role = %s where id = %s"
    cur.execute(sql, [user['username'], user['password'], user['email'], user['role'], user['id']])
    connection.commit()
    connection.close()
    return user

def delete_user(id):
    connection = get_db_connection()
    cur = connection.cursor()
    cur.execute('delete from sys_user where id = %s', [id])
    connection.commit()
    connection.close()
    return True
