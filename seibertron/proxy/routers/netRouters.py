from flask import Blueprint, request, session
from service import netService
from common import result

net_bp = Blueprint('network', __name__)


@net_bp.route('/wifi_scan', methods=['GET'])
def wifi_scan():
    return result.success(netService.wifi_scan())


@net_bp.route('/wifi_connect', methods=['POST'])
def connect_to_wifi():
    wifi_config = request.get_json()
    print(wifi_config)
    return result.success(netService.wifi_connect(wifi_config['ssid'], wifi_config['password']))


@net_bp.route('/get_network', methods=['GET'])
def network_config():
    config = netService.network_config()
    return result.success(config)

