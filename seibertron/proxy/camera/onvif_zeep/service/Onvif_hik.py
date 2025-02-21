import time
from datetime import datetime
import requests
import zeep
from camera.onvif_zeep.onvif.client import ONVIFCamera
from requests.auth import HTTPDigestAuth


def zeep_pythonvalue(self, xmlvalue):
    return xmlvalue


class Onvif_hik(object):

    def __init__(self, ip: str, username: str, password: str):
        self.ip = ip
        self.username = username
        self.password = password
        self.save_path = "../picture/" + self.ip + "_{}.jpg"  # 截图保存路径
        self.content_cam()

    def content_cam(self):
        """
        链接相机地址
        :return:
        """
        try:
            self.mycam = ONVIFCamera(self.ip, 80, self.username, self.password)
            self.media = self.mycam.create_media_service()  # 创建媒体服务
            # 得到目标概要文件
            zeep.xsd.simple.AnySimpleType.pythonvalue = zeep_pythonvalue
            self.media_profile = self.media.GetProfiles()[0]  # 获取配置信息
            self.ptz = self.mycam.create_ptz_service()  # 创建控制台服务
            return True
        except Exception as e:
            return False

    def Snapshot(self):
        """
        截图
        :return:
        """
        res = self.media.GetSnapshotUri({'ProfileToken': self.media_profile.token})

        response = requests.get(res.Uri, auth=HTTPDigestAuth(self.username, self.password))
        # 检查是否需要存储图片
        current_time = time.time()

        # 格式化 datetime 对象为字符串
        now_time = datetime.fromtimestamp(current_time).strftime('%Y%m%d%H%M%S')
        save_path = self.save_path.format(now_time)  # 截图保存路径
        with open(save_path, 'wb') as f:  # 保存截图
            f.write(response.content)

    def get_presets(self):
        """
        获取预置点列表
        :return:预置点列表--所有的预置点
        """
        presets = self.ptz.GetPresets({'ProfileToken': self.media_profile.token})  # 获取所有预置点,返回值：list
        return presets

    def set_presets(self, preset_name, preset_token: int):
        """
        设置预置点
        """
        # self.ptz.SetPreset({'ProfileToken': self.media_profile.token, 'Name': preset_name, 'token': preset_token})
        self.ptz.SetPreset({
            'ProfileToken': self.media_profile.token,  # Profile的token
            'PresetToken': preset_token,  # 这里传入预设点的名称或唯一标识符
            'PresetName': preset_name  # 预设点的名称，有的设备可能不需要这个参数
        })
    def goto_preset(self, presets_token: int):
        """
        移动到指定预置点
        :param presets_token: 目的位置的token，获取预置点返回值中
        :return:
        """
        try:
            # self.ptz.GotoPreset(
            #     {'ProfileToken': self.media_profile.token, "PresetToken": presets_token})  # 移动到指定预置点位置
            params = self.ptz.create_type('GotoPreset')
            params.ProfileToken = self.media_profile.token
            params.PresetToken = presets_token
            self.ptz.GotoPreset(params)
        except Exception as e:
            print(e)

    def move(self, x: str, y: str, timeout: int = 1):
        # x = str(-int(x))
        # y = str(-int(y))
        """
         移动
         :param y:
         :param x:
         :param timeout: 生效时间
         :return:bbbbb
         """
        try:
            request = self.ptz.create_type('ContinuousMove')
            request.Velocity = {"PanTilt": {"x": x, "y": y}}
            request.ProfileToken = self.media_profile.token
            self.ptz.ContinuousMove(request)
            # time.sleep(0.5)
            # # 发送停止请求
            # stop_request = self.ptz.create_type('Stop')
            # stop_request.ProfileToken = self.media_profile.token
            # self.ptz.Stop(stop_request)
            print("Stop requested")

        except Exception as e:
            print(f"Failed to stop camera movement: {e}")

    def stop(self):
        request = self.ptz.create_type('ContinuousMove')
        request.Velocity = {"PanTilt": {"x": '0', "y": '0'}}
        request.ProfileToken = self.media_profile.token
        self.ptz.ContinuousMove(self.media_profile.token)

    def zoom(self, zoom: str, timeout: int = 1):
        """
        变焦
        :param zoom: 1为拉近或-1为远离
        :param timeout: 生效时间
        :return:bbbbb
        """
        request = self.ptz.create_type('ContinuousMove')
        request.ProfileToken = self.media_profile.token
        request.Velocity = {"Zoom": zoom}
        self.ptz.ContinuousMove(request)
        # time.sleep(timeout)
        # res = self.ptz.Stop({'ProfileToken': request.ProfileToken})
        # print(res)

    def stop_zoom(self):
        
        self.ptz.Stop({'ProfileToken': self.media_profile.token})

    def get_status(self):
        """
        获取当前预置点的信息
        :return:
        """
        params = self.ptz.create_type('GetStatus')
        params.ProfileToken = self.media_profile.token
        res = self.ptz.GetStatus(params)
        # print(res)
        return res


if __name__ == '__main__':
    o = Onvif_hik(ip="192.168.13.213", username="admin", password="edge2021")
    preset = o.get_presets()
    print(preset)
    # o.goto_preset(4)
    o.Snapshot()

    # preset_name = "预置点 2"
    # preset_token = '2'
    # o.set_presets(preset_name, preset_token)
