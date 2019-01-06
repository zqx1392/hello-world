#!/bin/bash

script_log='script_log/message.log'

if [ ! -d "script_log" ]
then
    mkdir script_log
fi

#create log file
touch $script_log

#Check RHEL Version
echo "Checking RHEL version..."
cat /etc/redhat-release;uname -r
echo "if this version is lower than 3.10.0-54.0.1.el7.x86_64. Please update to newer version."

#check server swap ram
echo "Checking swap memory size..." | tee $script_log
totalRam=$(grep MemTotal /proc/meminfo | egrep -o '[0-9]+')
totalRam=$(($totalRam/1024))
if [ "$totalRam" -lt 1024 ]; then
	echo "Error. memory size is less than 1024MB which should be impossible" | tee $script_log
	exit 1;
elif [ "$totalRam" -lt 2049 ]; then
	swapRam=$(($totalRam*3/2))
elif [ "$totalRam" -lt 8193 ]; then
	swapRam=$totalRam
elif [ "$totalRam" -lt 14336 ]; then
	swapRam=$(($totalRam*3/4))
elif [ "$totalRam" -lt 32768 ]; then
	swapRam=$totalRam
else
	swapRam=32678
fi;
realSwapRam=$(grep SwapTotal /proc/meminfo | egrep -o '[0-9]+')
realSwapRam=$(($realSwapRam/1024))
if [ "$realSwapRam" -lt "$swapRam" ]; then
	echo "Error, swap memory is size insufficient! Please refer to Wiki for more information" | tee $script_log
	exit
fi;
echo "Swap memory size is sufficient!" | tee $script_log

#check tmp directory space
echo "verifying whether /tmp directory space is sufficient or not..."                       | tee $script_log
tmp_size=$(df -h | grep tmpfs | egrep -o '[0-9]+.[0-9]+.' | head -1)
echo $tmp_size
minimum_size=400
if [[ "$tmp_size" == *"G"* ]]; then
	echo "SUFFICIENT"                                                                       | tee $script_log
elif [[ "$tmp_size" == *"M"* ]]; then
	echo "Error, /tmp directory space is less than 1GB."                                    | tee $script_log
fi;
echo "/tmp directory space is sufficient! (1GB+)" | tee $script_log

#create Oracle user and related group
echo
groupadd -g 500 oinstall
groupadd -g 501 dba
groupadd -g 502 oper
groupadd -g 503 backupdba
groupadd -g 504 dgdba
groupadd -g 505 kmdba
useradd -g oinstall oracle
usermod -aG dba oracle
usermod -aG oper oracle
usermod -aG backupdba oracle
usermod -aG dgdba oracle
usermod -aG kmdba oracle
#set Oracle user password
./auto_set_oracle_password.sh

#/home/oracle/.bashrcにOracle12cサーバの環境変数設定を追記
# For Oracle12c
export ORACLE_BASE=/opt/oracle
export ORACLE_HOME=$ORACLE_BASE/product/12.1.0/db_1
export ORA_NLS10=$ORACLE_HOME/nls/data
export NLS_LANG=Japanese_Japan.AL32UTF8
export ORACLE_SID=sindy
export PATH=$PATH:$ORACLE_HOME/bin
export ORACLE_DOC=$ORACLE_HOME/doc
export ORASYM=$ORACLE_HOME/bin/oracle
CLASSPATH=$ORACLE_HOME/JRE:$ORACLE_HOME/jlib:$ORACLE_HOME/rdbms/jlib
CLASSPATH=$CLASSPATH:$ORACLE_HOME/network/jlib
CLASSPATH=$CLASSPATH:$ORACLE_HOME/jdbc/lib/classes12.zip
CLASSPATH=$CLASSPATH:$ORACLE_HOME/jdbc/lib/nls_charaset12.zip
export CLASSPATH
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ORACLE_HOME/lib
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ORACLE_HOME/ctx/lib:$ORACLE_HOME/jdbc/lib
export LD_LIBRARY_PATH
#export CVSROOT=:pserver:t_ohta@preon:/var/cvs //不要？

#/etc/security/limits.d/oracle12c-limits.confを以下のように作成
#Oracle12cR1
oracle soft nproc 2047
oracle hard nproc 16384
oracle soft nofile 1024
oracle hard nofile 65536
oracle soft stack 10240
oracle hard stack 10240

#pam認証でシェル制限を適用するために、/etc/pam.d/loginに一行追加。
echo "session required pam_limits.so" >> /etc/pam.d/login
tail -n 5 /etc/pam.d/login

#ulimitを増やすために、/etc/profile.d/oracle12c-profile.shを以下の内容で作成
#Oracle12cR1
if [ $USER = "oracle" ]; then
   if [ $SHELL = "/bin/ksh" ]; then
        ulimit -u 16384
        ulimit -n 65536
   else
       ulimit -u 16384 -n 65536
   fi
fi

#oracleユーザにログインして確認
# su - oracle
$ ulimit -a | grep -E "(^open.file|^stack.size|^max.user.processes)" | sort
//以下のようにでればOK。
max user processes              (-u) 16384
open files                      (-n) 65536
stack size              (kbytes, -s) 10240

#svnをインストール
# yum install subversion
#oracleユーザに切り替え
#su - oracle
#homeディレクトリにrelease/trunk/public/SiNDY-b/SiNDY-Server/sde/svnをチェックアウト
$ cd ~
$ svn co http://preon.mr.ipc.pioneer.co.jp/svn/release/trunk/public/SiNDY-b/SiNDY-Server/sde/svn
#homeディレクトリにrelease/trunk/public/SiNDY-b/SQL/SDEとSiNDY-bをチェックアウト
$ cd ~
$ mkdir SQL
$ cd SQL
$ svn co http://preon.mr.ipc.pioneer.co.jp/svn/release/trunk/public/SiNDY-b/SQL/SDE
$ svn co http://preon.mr.ipc.pioneer.co.jp/svn/release/trunk/public/SiNDY-b/SQL/SiNDY-b
#homeディレクトリにrelease/trunk/public/SiNDY-b/setting_fileをチェックアウト
$ cd ~
$ svn co http://preon.mr.ipc.pioneer.co.jp/svn/release/trunk/public/SiNDY-b/setting_file

#yumでインストール
# yum install cifs-utils
#格納先が\\magi1\sysmasterの場合のコマンド例
#※usernmameは作業者自身のwinドメインユーザを使うこと。
# mkdir /media/magi1
# mount -t cifs -o username=t_kubo //magi1/sysmaster /media/magi1
#コピー先作成
# mkdir /home1/oracle_install
#インストールファイルをコピー
# cp /media/magi1/sysmaster/03_software/<省略>/linuxamd64_12102_database_se2_*.zip /home1/oracle_install/.
#ZIPを解凍
# unzip /home1/oracle_install/linuxamd64_12102_database_se2_1of2.zip
# unzip /home1/oracle_install/linuxamd64_12102_database_se2_2of2.zip
#home1以下の所有者をoracleに変更しておく
# chown -R oracle:oinstall /home1
#cifsアンマウント
# umount /media/magi1
#Redhatのextras,optionalリポジトリを有効化
# yum-config-manager --enable rhel-7-server-extras-rpms rhel-7-server-optional-rpms
#初めから入っているパッケージもあるが、面倒なので一気にいれる。
# yum install \
binutils-2.*.el7*.x86_64 \
compat-libcap1-1.*.el7*.x86_64 \
compat-libstdc++-33-3.*.el7*.i686 \
compat-libstdc++-33-3.*.el7*.x86_64 \
gcc-4.*.el7*.x86_64 \
gcc-c++-4.*.el7*.x86_64 \
glibc-2.*.el7*.i686 \
glibc-2.*.el7*.x86_64 \
glibc-devel-2.*.el7*.i686 \
glibc-devel-2.*.el7*.x86_64 \
ksh-*.el7*.x86_64 \
libaio-0.*.el7*.i686 \
libaio-0.*.el7*.x86_64 \
libaio-devel-0.*.el7*.i686 \
libaio-devel-0.*.el7*.x86_64 \
libgcc-4.*.el7*.i686 \
libgcc-4.*.el7*.x86_64 \
libstdc++-4.*.el7*.i686 \
libstdc++-4.*.el7*.x86_64 \
libstdc++-devel-4.*.el7*.i686 \
libstdc++-devel-4.*.el7*.x86_64 \
libXi-1.*.el7*.i686 \
libXi-1.*.el7*.x86_64 \
libXtst-1.*.el7*.i686 \
libXtst-1.*.el7*.x86_64 \
make-3.*.el7*.x86_64 \
sysstat-10.*.el7*.x86_64 \
pam-1.*.el7*.x86_64
#もう一度上記コマンドを叩けば、全部入っているか確認できます。 以下のように出ればOK
パッケージ binutils-2.25.1-22.base.el7.x86_64 はインストール済みか最新バージョンです
パッケージ compat-libcap1-1.10-7.el7.x86_64 はインストール済みか最新バージョンです
パッケージ compat-libstdc++-33-3.2.3-72.el7.i686 はインストール済みか最新バージョンです
パッケージ compat-libstdc++-33-3.2.3-72.el7.x86_64 はインストール済みか最新バージョンです
パッケージ gcc-4.8.5-11.el7.x86_64 はインストール済みか最新バージョンです
パッケージ gcc-c++-4.8.5-11.el7.x86_64 はインストール済みか最新バージョンです
パッケージ glibc-2.17-157.el7_3.1.i686 はインストール済みか最新バージョンです
パッケージ glibc-2.17-157.el7_3.1.x86_64 はインストール済みか最新バージョンです
パッケージ glibc-devel-2.17-157.el7_3.1.i686 はインストール済みか最新バージョンです
パッケージ glibc-devel-2.17-157.el7_3.1.x86_64 はインストール済みか最新バージョンです
パッケージ ksh-20120801-26.el7.x86_64 はインストール済みか最新バージョンです
パッケージ libaio-0.3.109-13.el7.i686 はインストール済みか最新バージョンです
パッケージ libaio-0.3.109-13.el7.x86_64 はインストール済みか最新バージョンです
パッケージ libaio-devel-0.3.109-13.el7.i686 はインストール済みか最新バージョンです
パッケージ libaio-devel-0.3.109-13.el7.x86_64 はインストール済みか最新バージョンです
パッケージ libgcc-4.8.5-11.el7.i686 はインストール済みか最新バージョンです
パッケージ libgcc-4.8.5-11.el7.x86_64 はインストール済みか最新バージョンです
パッケージ libstdc++-4.8.5-11.el7.i686 はインストール済みか最新バージョンです
パッケージ libstdc++-4.8.5-11.el7.x86_64 はインストール済みか最新バージョンです
パッケージ libstdc++-devel-4.8.5-11.el7.i686 はインストール済みか最新バージョンです
パッケージ libstdc++-devel-4.8.5-11.el7.x86_64 はインストール済みか最新バージョンです
パッケージ libXi-1.7.4-2.el7.i686 はインストール済みか最新バージョンです
パッケージ libXi-1.7.4-2.el7.x86_64 はインストール済みか最新バージョンです
パッケージ libXtst-1.2.2-2.1.el7.i686 はインストール済みか最新バージョンです
パッケージ libXtst-1.2.2-2.1.el7.x86_64 はインストール済みか最新バージョンです
パッケージ 1:make-3.82-23.el7.x86_64 はインストール済みか最新バージョンです
パッケージ sysstat-10.1.5-11.el7.x86_64 はインストール済みか最新バージョンです
パッケージ pam-1.1.8-18.el7.x86_64 はインストール済みか最新バージョンです
#cvuqdisk RPMの導入
 # export CVUQDISK_GRP= oinstall
  ## rpm -iv /home1/oracle_install/database/rpm/cvuqdisk-1.0.9-1.rpm
#/etc/sysctl.conf*2に以下を追記
## Oracle12cR1
fs.file-max=6815744
fs.aio-max-nr=3145728
kernel.panic_on_oops = 1
kernel.sem=600 76800 100 128
kernel.shmall = ＜デフォルト値を設定　※Oracleインストーラの検査時に必要＞
kernel.shmmax = ＜デフォルト値を設定　※Oracleインストーラの検査時に必要＞
kernel.shmmni=4096
net.core.rmem_default=262144
net.core.rmem_max=4194304
net.core.wmem_default=262144
net.core.wmem_max=1048576
net.ipv4.ip_local_port_range=9000 65500
kernel.numa_balancing=0
#設定内容を反映
# sysctl -p
#設定内容を確認
/sbin/sysctl -a | grep -E "(^fs.file-max|^kernel.sem|^kernel.shmall|^kernel.shmmax|^kernel.shmmni|^net.core.rmem_default \
|^net.core.rmem_max|^net.core.wmem_default|^net.core.wmem_max|^fs.aio-max-nr|^net.ipv4.ip_local_port_range|^kernel.panic_on_oops|^kernel.numa_balancing)" | sort
#/etc/fstabに以下を追記
#size指定は値を適宜修正すること。
tmpfs           /dev/shm         tmpfs   rw,exec,size=<物理メモリの3/4 - 1>G     0 0
#以下コマンドで確認できる。
# df -h | grep /dev/shm
# findmnt /dev/shm
#THPの設定状況確認
# cat /sys/kernel/mm/transparent_hugepage/enabled
[always] madvise never //括弧内がalwaysのはず。RHEL7のDEFAULT
# cat /etc/grub2.cfg | grep huge //何も帰ってこないはず。RHEL7のDEFAULT
#/etc/default/grubのGRUB_CMDLINE_LINUXを編集
変更前:GRUB_CMDLINE_LINUX="rd.lvm.lv=rhel/root rd.lvm.lv=rhel/swap rhgb quiet"
変更後:GRUB_CMDLINE_LINUX="rd.lvm.lv=rhel/root rd.lvm.lv=rhel/swap transparent_hugepage=never rhgb quiet"
#設定変更を反映
grub2-mkconfig -o /boot/grub2/grub.cfg
#reboot
reboot
#カーネルパラメータの設定確認
# /sbin/sysctl -a | grep -E "(^fs.file-max|^kernel.sem|^kernel.shmall|^kernel.shmmax|^kernel.shmmni|^net.core.rmem_default \
#|^net.core.rmem_max|^net.core.wmem_default|^net.core.wmem_max|^fs.aio-max-nr|^net.ipv4.ip_local_port_range|^kernel.panic_on_oops|^kernel.numa_balancing)" | sort
//以下のようにでてくればOK
fs.aio-max-nr = 3145728
fs.file-max = 6815744
kernel.numa_balancing = 0
kernel.numa_balancing_scan_delay_ms = 1000
kernel.numa_balancing_scan_period_max_ms = 60000
kernel.numa_balancing_scan_period_min_ms = 1000
kernel.numa_balancing_scan_size_mb = 256
kernel.numa_balancing_settle_count = 4
kernel.panic_on_oops = 1
kernel.sem = 600        76800   100     128
kernel.sem_next_id = -1
kernel.shmall = 18446744073692774399
kernel.shmmax = 18446744073692774399
kernel.shmmni = 4096
net.core.rmem_default = 262144
net.core.rmem_max = 4194304
net.core.wmem_default = 262144
net.core.wmem_max = 1048576
net.ipv4.ip_local_port_range = 9000     65500
#/dev/shmの設定確認
# df -h | grep /dev/shm //サイズが指定どおり変わっていればOK
tmpfs 32G 224K 32G 1% /dev/shm
# findmnt /dev/shm
TARGET SOURCE FSTYPE OPTIONS //オプションが以下のようになっていればOK
/dev/shm tmpfs tmpfs rw,relatime
#THPの設定状況再確認
# cat /sys/kernel/mm/transparent_hugepage/enabled
always madvise [never] //neverに括弧が移っていればOK
# cat /etc/grub2.cfg | grep huge //以下のように出てくるればOK
linux16 /vmlinuz-3.10.0-514.6.1.el7.x86_64 root=/dev/mapper/rhel-root ro rd.lvm.lv=rhel/root rd.lvm.lv=rhel/swap transparent_hugepage=never rhgb quiet
linux16 /vmlinuz-3.10.0-514.el7.x86_64 root=/dev/mapper/rhel-root ro rd.lvm.lv=rhel/root rd.lvm.lv=rhel/swap transparent_hugepage=never rhgb quiet
linux16 /vmlinuz-0-rescue-b09caecf553b4527a5b4f7a926c7ff9a root=/dev/mapper/rhel-root ro rd.lvm.lv=rhel/root rd.lvm.lv=rhel/swap transparent_hugepage=never rhgb quiet
#以下コマンドで各種ディレクトリを作成する。(home1形式)
mkdir -p /home/oracle/utl_file
mkdir -p /home1/oradata
mkdir -p /home1/oraarch
mkdir -p /opt/oracle/flash_recovery_area
chown oracle.oinstall /home/oracle/utl_file
chown oracle.oinstall /home1/oradata
chown oracle.oinstall /home1/oraarch
chown -R oracle.oinstall /opt/oracle/
chmod -R 775 /opt/oracle
###Oracleインストール
#レスポンスファイルの修正
$ vi /home/oracle/setting_file/db_install.rsp
ORACLE_HOSTNAME=<ホスト名を入力>
#インストール
$  ./runInstaller -silent -responseFile /home/oracle/setting_file/db_install.rsp
#rootユーザで以下スクリプト実行が求められるので実施する。
$ su -
$ /opt/oracle/oraInventory/orainstRoot.sh
$ /opt/oracle/product/12.1.0/db_1/root.sh
#NETCAでlistenerを作成する。
##oracleユーザで行うこと！
$ netca -silent -responseFile /home/oracle/setting_file/netca.rsp
#

#

#

#

#
