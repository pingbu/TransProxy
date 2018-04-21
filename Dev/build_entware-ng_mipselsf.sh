#!/bin/bash

if [ ! -f Main/Entware\ Release/TransProxy ]; then
	echo "FAILED, please build 'Entware Release' first!!!"
	exit -1
fi

mkdir ~/build-transproxy.tmp

mkdir ~/build-transproxy.tmp/opt
mkdir ~/build-transproxy.tmp/opt/share
mkdir ~/build-transproxy.tmp/opt/share/transproxy
cp -r Files/* ~/build-transproxy.tmp/opt/share/transproxy/
cp Main/Entware\ Release/TransProxy ~/build-transproxy.tmp/opt/share/transproxy/transproxy
cd ~/build-transproxy.tmp
chown root:root -R opt
chmod 0644 -R opt
chmod 0755 opt/share/transproxy/transproxy
chmod 0755 opt/share/transproxy/*.sh
tar -czf data.tar.gz ./opt
rm -Rf opt
cd - > /dev/null

cp -r control_mipselsf ~/build-transproxy.tmp/control
cd ~/build-transproxy.tmp
chown root:root -R control
chmod 0644 -R control
tar -czf control.tar.gz ./control
rm -Rf control
cd - > /dev/null

echo "2.0" > ~/build-transproxy.tmp/debian-binary

cd ~/build-transproxy.tmp
chown root:root *
chmod 0644 *
tar -czf transproxy_entware-ng_mipselsf.ipk ./*
cd - > /dev/null

mv ~/build-transproxy.tmp/*.ipk ../Packages/
rm -Rf ~/build-transproxy.tmp
echo "Build OK."
