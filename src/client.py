import os
import socket
import json

target_ip = "127.0.0.1"
target_port = 65500
buffer_size = 4096

home_dir = os.path.expanduser('~')

data = dict()
data['audio'] = [1,3,[1,2,3], [4,5,6], [7,8,9]]
data['files'] = [[0, home_dir], [1, home_dir], [3, home_dir]]
string = json.dumps(data).encode()
#print(string)
#print(type(string))

# 1.ソケットオブジェクトの作成
tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 2.サーバに接続
tcp_client.connect((target_ip,target_port))

# 3.サーバにデータを送信
tcp_client.send(string)

# 4.サーバからのレスポンスを受信
response = tcp_client.recv(buffer_size)
print("[*]Received a response : {}".format(response))

tcp_client.close()