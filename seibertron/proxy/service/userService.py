from db import userdb

def userList():
    return userdb.get_users()

def user_inster(user):
    user = userdb.insert_user(user)
    return user

def user_update(user):
    user = userdb.update_user(user)
    return user

def user_delete(id):
    result = userdb.delete_user(id)
    return result