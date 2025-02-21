import fnmatch
import threading
import os
import re
import ftplib

from camera.media import media_record

DEVICE_ID = '202407241806'
REMOTE_HOST = '61.169.115.124'
REMOTE_USER = 'edge'
REMOTE_PASS = '123456'
REMOTE_DIR = '/home/edge/ftp/'

lock = threading.Lock()

uploading = False
# ??????FTP??????


def upload_file(filename, formatted_time):
    thread = threading.Thread(target=_upload_file_ftp, args=(filename, formatted_time))
    thread.start()


def _upload_file_ftp(filename, formatted_time):
    global uploading
    remote_path = f"{REMOTE_DIR}{DEVICE_ID}/{formatted_time}/"
    file_name = os.path.basename(filename)
    remote_file_path = os.path.join(remote_path, file_name)
    print(000000000)
    uploading = True
    print(uploading)
    lock.acquire()

    try:
        with ftplib.FTP(REMOTE_HOST) as ftp:
            print(11111111111)
            ftp.login(user=REMOTE_USER, passwd=REMOTE_PASS)
            print(222222222222)
            # ??????????????????????????????
            try:
                ftp.cwd(remote_path)
            except ftplib.error_perm:
                # ??????????????????????????
                ftp.mkd(remote_path)
                ftp.cwd(remote_path)
            print(3333333333)
            # ????????
            with open(filename, 'rb') as f:
                ftp.storbinary(f'STOR {file_name}', f)
            print(4444444444)

            # ????????????????????????????????????????????????
            os.remove(filename)
            print(f"Uploaded and deleted: {filename}")
    finally:
        uploading = False
        print(uploading)
        lock.release()



def find_files(directory, pattern='*'):
    file_paths = []
    for root, _, files in os.walk(directory):
        for file in files:
            if fnmatch.fnmatch(file, pattern):
                file_paths.append(os.path.join(root, file))
    return file_paths


def file_is_open(filepath):
    try:
        with open(filepath, 'r'):
            return False
    except IOError:
        return True


def local_file_upload():
    print(uploading)
    if uploading:
    	return
    matching_files = find_files('./video/', '*.mp4')
    for file_path in matching_files:
        try:
            print(file_path)
            record_time = extract_substring(file_path)
            print(record_time)

            if media_record.formatted_time != record_time:
                upload_file(file_path, record_time)
        except Exception as e:
            print(f"???????? {file_path} ??????????: {e}")


def extract_substring(s):
    match = re.search(r'_([^_]*)_', s)
    return s.split('_')[-2]
