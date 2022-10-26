import requests
with open('./sipm.bin', 'rb') as f:
    data = f.read()

print(data[1:8])
data = data[1:8]

res = requests.post(url='http://localhost:33232/content_receiver',
                    data=data,
                    headers={'Content-Type': 'application/octet-stream'})
