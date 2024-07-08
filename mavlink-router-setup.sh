#mavlink-router
#https://github.com/mavlink-router/mavlink-router 

#install meson 0.55.0 and make sure in directory /usr/lib/python3/dist-packages 
sudo pip3 install meson==0.55 

meson setup --cross-file meson-cross-aarch64.ini build-aarch64


ninja -C build-aarch64
sudo ninja -C build install 

#to run (sub with correct IP):
#mavlink-routerd -e 192.168.7.1:14550 -e 127.0.0.1:14550 /dev/ttyACM0
