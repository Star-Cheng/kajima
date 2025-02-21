from utils import netUtil
import subprocess

def wifi_scan():
    return netUtil.wifi_scan()

def wifi_connect(ssid, password):
    # 示例：修改 wlan0 接口的配置
    wlan_dhcp = True  # yes or no
    wlan_ssid = ssid
    wlan_password = password
    wlan_interface = 'wlp3s0'
    wlan_new_config = {
        
        'access-points': {
            f'{wlan_ssid}': {
                'password': f'{wlan_password}'
            }
        },
        'dhcp4': 'yes'
    }
    try:
        netUtil.modify_network_config(wlan_interface, wlan_new_config, 'wifis')
        print(f"{wlan_interface} 接口的配置已修改")
        print(netUtil.read_netplan_config())
        subprocess.Popen(f"sudo netplan apply", shell=True)
    except FileNotFoundError as e:
        print(e)
    except Exception as e:
        print(f"发生错误：{e}")
    return True

def network_config():
    # 获取所有网络接口的信息
    network_array = netUtil.get_network_info()
    print(network_array)
    network_list = []
    network_array['enp2s0']['interface'] = 'ETH'
    network_array['wlp3s0']['interface'] = 'WLAN'
    config = netUtil.read_netplan_config()
    wifi = config['network']['wifis']['wlp3s0']['access-points']

    for k, v in wifi.items():
        network_array['wlp3s0']['ssid'] = k
        network_array['wlp3s0']['password'] = v['password']

    network_list.append(network_array['enp2s0'])
    network_list.append(network_array['wlp3s0'])
    return network_list
