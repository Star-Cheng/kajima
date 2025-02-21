from flask import Blueprint, request, session
from service import userService, sysService
from common import result

sys_bp = Blueprint('sys', __name__)


@sys_bp.route('/wifi_scan', methods=['GET'])
def wifi_scan():
    return result.success(sysService.wifi_scan())


@sys_bp.route('/wifi_connect', methods=['POST'])
def connect_to_wifi():
    wifi_config = request.get_json()
    print(wifi_config)
    return result.success(wifi_config)


@sys_bp.route('/get_network', methods=['GET'])
def network_config():
    config = sysService.network_config()
    return result.success(config)


@sys_bp.route('/set_network', methods=['POST'])
def network_set_config():
    # ip gateway netmask dns
    network = request.get_json()
    return result.success(network)
