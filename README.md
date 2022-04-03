# MasterArbeitCode

## Installation
- Flashen von Raspberry Pi OS Lite (64bit) auf die eine SD-Karte
- Headless Installation laut https://www.tomshardware.com/reviews/raspberry-pi-headless-setup-how-to,6028.html
- Verbinden mittels ssh `ssh pi@raspberrypi` mit dem passwort "raspberry"
- Update des OS `sudo apt update && sudo apt dist-upgrade `
- Installation zusätzlicher Packages `sudo apt install git python3-pip ffmpeg libsm6 libxext6 -y` 
- Klonen des repos `git clone <url>`
- Installation wittypy `wget http://www.uugear.com/repo/WittyPi3/install.sh && sudo sh install.sh` anschließend aus- und einstecken.
- Installation der Pythondependencies `cd MasterArbeitCode/Basestation/raspberry_pi && pip install -r requirements.txt` 
- Download tensorflow wheel file `wget --load-cookies /tmp/cookies.txt "https://docs.google.com/uc?export=download&confirm=$(wget --quiet --save-cookies /tmp/cookies.txt --keep-session-cookies --no-check-certificate 'https://docs.google.com/uc?export=download&id=1YpxNubmEL_4EgTrVMu-kYyzAbtyLis29' -O- | sed -rn 's/.*confirm=([0-9A-Za-z_]+).*/\1\n/p')&id=1YpxNubmEL_4EgTrVMu-kYyzAbtyLis29" -O tensorflow-2.8.0-cp39-cp39-linux_aarch64.whl && rm -rf /tmp/cookies.txt` 
- Installation tensorflow `pip install tensorflow-2.8.0-cp39-cp39-linux_aarch64.whl`
- Download des weights-Files `wget https://github.com/Bleialf/MasterArbeitFiles/raw/main/yolov4-cars.tflite`
- Starten der Basestation mittels `python server.py <weightsfile> <wittipyfolder>` mehr Informationen mittels `python server.py -h`
- Beispiel `python server.py yolov4-cars.tflite ../../../wittypi/ --bootdelay 100 --initdelay 100 --sleepdelay 100`
