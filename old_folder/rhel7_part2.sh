#!/bin/bash

#create log file
script_log='script_log/message.log'
if [ ! -d "script_log" ]
then
    mkdir script_log
fi
touch $script_log

#locale_lang='"ja_JP.UTF-8"'
#host_addr='192.0.0.1'
#host_name='localhost'
host_addr=$1
host_name=$2
locale_lang=$3

#if input is incomplete
if [ $# -ne 3 ]
   then
      printf "Usage:rhel7_install_part2.sh 'Host Address' 'Host Name' 'Locale Language'\
	    \nExample:rhel7_install_part2.sh 192.168.1.1 fakehostname ja_JP.UTF-8\n"
      exit
fi

#CONFIRM THE ARGUMENT
echo "\nStarting..."							| tee -a $script_log
date '+%D %T'								| tee -a $script_log
echo "######################################################"   | tee -a $script_log
echo "INPUT SETTING"                                            | tee -a $script_log
echo "######################################################"   | tee -a $script_log
echo "HOST ADDRESS    : $host_addr"                             | tee -a $script_log
echo "HOST NAME       : $host_name"                             | tee -a $script_log
echo "LOCALE LANGUAGE : $locale_lang"                           | tee -a $script_log
echo -n "Welcome to the second part of RHEL automatic setup script!"                   | tee -a $script_log
echo -n "Continue with this setting?[y/n] > "                   | tee -a $script_log
read answer
if [ "$answer" = "n" ]; then
	echo "User cancels the script. Exiting..."                                           | tee -a $script_log
	exit
fi;

#verify that SELinux has been disabled
echo "Verifying SELinux has been disabled"                									| tee -a $script_log
getenforce | grep -q "Disabled" || ( echo "Error, SELinux is not disabled." | tee -a $script_log && exit )

#set locale language
echo "setting locale language in /etc/locale.conf"											| tee -a $script_log
f_locale='/etc/locale.conf'
LANG='LANG='
grep -q "$LANG" $f_locale && sed -ie "s|\($LANG\).*|\1$locale_lang|" $f_locale || echo "$LANG$locale_lang" >> $f_locale

#unset autologout
echo "Setting parameters in /etc/bashrc"                        							| tee -a $script_log
f_bashrc='/etc/bashrc'
unset='unset autologout'
grep -q "$unset" $f_bashrc || echo "$unset" >> $f_bashrc

#change host and server config
echo "Setting parameters in /etc/hosts"                         | tee -a $script_log
f_hosts='/etc/hosts'
LOCALHOST='127.0.0.1'
vLOCALHOST='               localhost.localdomain localhost'
grep -q "$LOCALHOST" $f_hosts && sed -ie "s|\($LOCALHOST\).*|\1$vLOCALHOST|" $f_hosts || echo "$LOCALHOST$vLOCALHOST" >> $f_hosts
grep -q -P "$host_addr\t\t$host_name" $f_hosts || echo  -e "$host_addr\t\t$host_name" >> $f_hosts

#edit DNS server
echo "Opening NetworkManager TUI"								| tee -a $script_log
echo "After opening, please select 接続の編集⇒デバイス[ensxxx]⇒＜編集...＞"								| tee -a $script_log
echo "And set DNS server as below"								| tee -a $script_log
echo "172.25.100.1"									| tee -a $script_log
echo "172.25.100.88"								| tee -a $script_log
echo "172.25.100.89"								| tee -a $script_log
echo "Then close NetworkManager TUI"								| tee -a $script_log
echo -n "Press anykey to continue... "                   | tee -a $script_log
read answer2
nmtui

#setting networkManager config
echo "setting networkManager config in /etc/NetworkManager/NetworkManager.conf"             | tee -a $script_log
f_networkmanager='/etc/NetworkManager/NetworkManager.conf'
grep -q "dns=none" $f_networkmanager || sed -ie "s|\(^plugins=\)\(.*\)|\1\2\ndns=none|" $f_networkmanager
systemctl restart NetworkManager

#delete virbr0
echo "deleting virbr0 KVM"																	| tee -a $script_log
virsh net-destroy default
virsh net-autostart default --disable
virsh net-list | grep 'no' || echo "Error, virsh is not deleted."							| tee -a $script_log

#Edit grub setting
echo "Disabling IPv6"																		| tee -a $script_log
f_grub='/etc/default/grub'
IPV6_GRUB=' ipv6.disable=1'
grep -q "$IPV6_GRUB" $f_grub || sed -ie "s|\(GRUB_CMDLINE_LINUX=\)\(.*\)\"|\1\2 $IPV6_GRUB\"|" $f_grub
grub2-mkconfig -o /boot/grub2/grub.cfg
grub2-mkconfig -o /boot/efi/EFI/redhat/grub.cfg

#service setting
#cups
echo "Disabling cups service..."																| tee -a $script_log
systemctl stop cups
systemctl disable cups.service
echo "Checking whether cups service has been disabled properly..."                		| tee -a $script_log
if !( systemctl status cups | grep "dead" ) then
	echo "cups is not disabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable cups service has been disabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled cups | grep "disabled" ) then
	echo "cups auto-enable service is not disabled, please check the config again."		| tee -a $script_log
    exit
fi;
#auditd
echo "Disabling auditd service..."																| tee -a $script_log
service auditd stop
systemctl disable auditd.service
echo "Checking whether auditd service has been disabled properly..."                		| tee -a $script_log
if !( service auditd status | grep "dead" ) then
	echo "auditd is not disabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable auditd service has been disabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled auditd.service | grep "disabled" ) then
	echo "auditd auto-enable service is not disabled, please check the config again."		| tee -a $script_log
    exit
fi;
#telnet
echo "Installing telnet.."                                  | tee -a $script_log
yes | yum install telnet-server telnet
echo "Enabling telenet service..."																| tee -a $script_log
systemctl start telnet.socket
systemctl enable telnet.socket
echo "Checking whether telent service has been enabled properly..."                		| tee -a $script_log
if !( systemctl status telnet.socket | grep "active" ) then
	echo "telnet is not enabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable telnet service has been enabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled telnet.socket | grep "enabled" ) then
	echo "telnet auto-enable service is not enabled, please check the config again."		| tee -a $script_log
    exit
fi;
#vsftpd
echo "Installing vsftpd..."                                     | tee -a $script_log
yes | yum install vsftpd
echo "Enabling vsftpd service..."																| tee -a $script_log
f_vsftpd='/etc/vsftpd/vsftpd.conf'
grep -q "listen=" $f_vsftpd && sed -ie "s|\(listen=\).*|\1YES|" $f_vsftpd || echo "listen=YES" >> $f_vsftpd
grep -q "listen_ipv6=" $f_vsftpd && sed -ie "s|\(listen_ipv6=\).*|\1NO|" $f_vsftpd || echo "listen_ipv6=NO" >> $f_vsftpd
systemctl start vsftpd
systemctl enable vsftpd
echo "Checking whether vsftpd service has been enabled properly..."                		| tee -a $script_log
if !( systemctl status vsftpd | grep "active" ) then
	echo "vsftpd is not enabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable vsftpd service has been enabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled vsftpd | grep "enabled" ) then
	echo "vsftpd auto-enable service is not enabled, please check the config again."		| tee -a $script_log
    exit
fi;
#snmpd
echo "Installing snmpd..."                                     | tee -a $script_log
yes | yum install net-snmp
echo "Enabling snmpd service..."																| tee -a $script_log
systemctl start snmpd
systemctl enable snmpd
echo "Checking whether snmpd service has been enabled properly..."                		| tee -a $script_log
if !( systemctl status snmpd | grep "active" ) then
	echo "snmpd is not enabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable snmpd service has been enabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled snmpd | grep "enabled" ) then
	echo "snmpd auto-enable service is not enabled, please check the config again."		| tee -a $script_log
    exit
fi;
#ntpd
echo "Installing chrony..."                                     | tee -a $script_log
yes | yum install chrony
echo "Editing chrony.conf..."									| tee -a $script_log
f_chrony='/etc/chrony.conf'
sed -ie "s|\(^server 0\.\)\(.*\)|#\1\2|" $f_chrony
sed -ie "s|\(^server 1\.\)\(.*\)|#\1\2|" $f_chrony
sed -ie "s|\(^server 2\.\)\(.*\)|#\1\2|" $f_chrony
sed -ie "s|\(^server 3\.\)\(.*\)|#\1\2\nserver 172.25.100.1 iburst|" $f_chrony
echo "Editing chrony.conf..."									| tee -a $script_log
echo 'OPTIONS="-4"' >> /etc/sysconfig/chronyd
systemctl restart chronyd
systemctl enable chronyd
echo "Checking whether chronyd service has been enabled properly..."                		| tee -a $script_log
if !( systemctl status chronyd | grep "active" ) then
	echo "chronyd is not enabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable chronyd service has been enabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled chronyd | grep "enabled" ) then
	echo "chronyd auto-enable service is not enabled, please check the config again."		| tee -a $script_log
    exit
fi;
chronyc sources | grep 'pioneer' || echo "Error, ntpd service has not been setup properly." | tee -a $script_log && exit
#postfix
echo "Installing postfix..."                                    | tee -a $script_log
yes | yum install postfix
echo "Setting parameters in /etc/postfix/main.conf"             | tee -a $script_log
f_maincf='/etc/postfix/main.cf'
server_name='mr.ipc.pioneer.co.jp'
sed -ie "s|\(^myhostname\)\(.*\)|\1 = $host_name.$server_name|" $f_maincf
sed -ie "s|\(^mydomain\)\(.*\)|\1 = $server_name|" $f_maincf
sed -ie "s|\(^myorigin\)\(.*\)|\1 = \$mydomain|" $f_maincf
sed -ie "s|\(^mydestination\)\(.*\)|\1 = \$myhostname,localhost.\$mydomain,localhost|" $f_maincf
sed -ie "s|\(^relayhost\)\(.*\)|\1 = 172.25.100.1|" $f_maincf
sed -ie "s|\(^inet_protocols\)\(.*\)|\1 = ipv4|" $f_maincf
systemctl start postfix
systemctl enable postfix
echo "Checking whether postfix service has been enabled properly..."                		| tee -a $script_log
if !( systemctl status postfix | grep "active" ) then
	echo "postfix is not enabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable postfix service has been enabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled postfix | grep "enabled" ) then
	echo "postfix auto-enable service is not enabled, please check the config again."		| tee -a $script_log
    exit
fi;
#rsyslog
echo "Installing rsyslog..."                                    | tee -a $script_log
yes | yum install rsyslog
echo "Setting parameters in /etc/rsyslog.conf"                   | tee -a $script_log
f_syslog='/etc/rsyslog.conf'
sed -ie "s|\(^kern.\*\)\(.*\)|\1\t\t\t\t\t\t\t/var/log/kernel|" $f_syslog
sed -ie "s|\(^kern.crit\)\(.*\)|\1\t\t\t\t\t\t/dev/console|" $f_syslog
echo "Setting rsyslog config..."	                                | tee -a $script_log
echo 'if $programname == "systemd" and ($msg contains "Starting Session" or $msg contains "Started Session" or $msg contains "Created slice" or $msg contains "Starting user-") then stop' >/etc/rsyslog.d/ignore-systemd-session-slice.conf
systemctl restart rsyslog
systemctl enable rsyslog
echo "Checking whether rsyslog service has been enabled properly..."                		| tee -a $script_log
if !( systemctl status rsyslog | grep "active" ) then
	echo "rsyslog is not enabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable rsyslog service has been enabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled rsyslog | grep "enabled" ) then
	echo "rsyslog auto-enable service is not enabled, please check the config again."		| tee -a $script_log
    exit
fi;
echo "Setting parameters in /etc/logrotate.d/syslog"            | tee -a $script_log
f_rotatesyslog='/etc/logrotate.d/syslog'
grep -q "/var/log/kernel" $f_rotatesyslog || sed -ie "s|\(/var/log/cron\)|\1\n/var/log/kernel|" $f_rotatesyslog
echo "Log Rotation Test"                                        | tee -a $script_log
logrotate -dv /etc/logrotate.d/syslog

#vncserver setup
echo "Installing vncserver..."                                    | tee -a $script_log
yes | yum install tigervnc-server
echo "Copying default setting file to installed folder..."                                    | tee -a $script_log
cp -p /usr/lib/systemd/system/vncserver@.service /etc/systemd/system/vncserver@.service
echo "Setting parameters in /etc/systemd/system/vncserver@.service"                    | tee -a $script_log
f_vncserver='/etc/systemd/system/vncserver@.service'
sed -ie "s|^ExecStart=/usr/sbin/runuser -l <USER>\(.*\)|ExecStart=/usr/sbin/runuser -l root -c \"/usr/bin/vncserver %i -geometry 1280x1024\"|" $f_vncserver
sed -ie "s|^PIDFile=/home/<USER>|PIDFile=/root|" $f_vncserver
echo "applying setting..."										| tee -a $script_log
systemctl daemon-reload
echo "setting vnc login password..."							| tee -a $script_log
yes | yum install expect
./auto_set_vnc_password.sh

systemctl restart vncserver@:1.service
systemctl enable vncserver@:1.service
echo "Checking whether vncserver@:1.service service has been enabled properly..."                		| tee -a $script_log
if !( systemctl status vncserver@:1.service | grep "active" ) then
	echo "vncserver@:1.service is not enabled, please check the config again."							| tee -a $script_log
    exit
fi;
echo "Checking whether auto-enable vncserver@:1.service service has been enabled properly..."     		| tee -a $script_log
if !( systemctl is-enabled vncserver@:1.service | grep "enabled" ) then
	echo "vncserver@:1.service auto-enable service is not enabled, please check the config again."		| tee -a $script_log
    exit
fi;
#logwatch
echo "Installing logwatch..."                                    | tee -a $script_log
yes | yum install logwatch
echo "Copying default setting file to installed folder..."                                    | tee -a $script_log
cp -p /usr/share/logwatch/default.conf/logwatch.conf /etc/logwatch/conf
echo "verifying that logwatch is working properly..."
logwatch --output stdout
logwatch --output mail
hwclock

#other setting
echo "setting gnome config..."                                    | tee -a $script_log
LANG=C xdg-user-dirs-gtk-update
gsettings set org.gnome.desktop.session idle-delay 0

##END OF LINUX SETTINGS.
echo "LINUX configuration setup is completed!. Please refer to wiki for additional setup"                  | tee -a $script_log
