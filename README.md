# net-suspend
Simple utility which puts your computer to sleep after some time if there is no activity on specified ports. Very handy in combination with automated WoL.

## Notice
* ❗️**socket statistics (ss)** must be installed
* if not run as systemd service, make sure it runs as root
* only tested on Ubuntu Server 20.04 LTS and Manjaro, no gurantees that it will work anywhere else whatsoever

## Getting started
Either download the [latest release](https://github.com/N0r1uno/net-suspend/releases/latest) binary and make it executable or build it yourself (`make`).

## Usage
./netsusp -l -d ***delay*** -***protocol*** ***port*** [...]
  * -l: write log file to `/var/log/netsusp.log` (optional)
  * delay: time in minutes until suspend after inactivity\
  * protocol: u(dp)/t(cp)\
  * port: 0-65535

eg: **netsusp -d 30 -t 25565 -t 22**
will not suspend if there are any established tcp connections on port 25565(⛏💎) and 22(ssh).
If idling for 30 minutes straight, the system will suspend.

## Run as systemd service (recommended)
* change the highlighted content of the given [netsusp.service](https://github.com/N0r1uno/net-suspend/blob/main/netsusp.service) file to fit your needs
* copy the edited file to `/lib/systemd/system/`.
* then issue the following:\
`systemctl daemon-reload`\
`systemctl enable netsusp.service`\
`systemctl start netsusp.service`
