import pymysql
from common import Snowflake


def get_db_connection():
    connection = pymysql.connect(host='127.0.0.1', port=3306, user='hms', password='edge@2021', db='seibertron')
    return connection

def load_user(username):
    connection = get_db_connection()
    cursor = connection.cursor()
    cursor.execute('select * from sys_user where username = %s', (username,))
    result = cursor.fetchone()
    if result is None:
        return None
    user = {
        'id': result[0],
        'username': result[1],
        'password': result[2],
        'email': result[3],
        'role': result[4]
    }
    return user
