
class User():

    def __init__(self, id, username, email, password):
        self.id = id
        self.username = username
        self.email = email
        self.password = password
        self.role = 'admin'
    # Flask-Login 需要的加载函数
