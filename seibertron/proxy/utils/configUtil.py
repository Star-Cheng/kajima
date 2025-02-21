import configparser
import os

# 配置文件工具类

class ConfigUtil:
    # 写入config文件
    def __init__(self, filename, filepath=r"./"):
        self.filename = filename
        os.chdir(filepath)
        self.cf = configparser.ConfigParser()
        self.cf.read(self.filename)  # 如果修改，则必须读原文件

    def _with_file(self):
        # 写入文件
        with open(self.filename, "w+") as f:
            self.cf.write(f)

    def add_section(self, section):
        """
        写入section值
        :param section: 字符串，section的名称
        :return:
        """
        self.cf.add_section(section)
        self._with_file()

    def set_options(self, section, option, value=None):
        """
        写入option值
        :param section:
        :param option:
        :param value:
        :return:
        """
        self.cf.set(section, option, value)
        self._with_file()

    def remove_section(self, section):
        """
        移除section值
        :param section:
        :return:
        """
        self.cf.remove_section(section)
        self._with_file()

    def remove_option(self, section, option):
        """
        移除option值
        :param section:
        :param option:
        :return:
        """
        self.cf.remove_option(section, option)
        self._with_file()

    def get_option(self, section, option):
        """
        获取option值
        :param section:
        :param option:
        :return:
        """
        return self.cf.get(section, option)

cn = ConfigUtil("config.ini")
