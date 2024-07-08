# On Jetson Terminal
sudo apt-get install gnupg
curl -s 'https://raw.githubusercontent.com/zerotier/ZeroTierOne/main/doc/contact%40zerotier.com.gpg' | gpg --import && \ 
if z=$(curl -s 'https://install.zerotier.com/' | gpg); then echo "$z" | sudo bash; fi
sudo zerotier-cli join 9e1948db6317a4d2
sudo zerotier-cli status
sudo zerotier-cli listnetworks