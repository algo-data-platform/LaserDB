#
# 该脚本是安装 ansible 需要的 sshd
# 使用普通用户运行即可，不过需要 普通用户有 sudo 权限
# sshd 运行用户是 adbot

# 切换 adbot 用户
EXEC_USER=adbot
AUTO_RUN_COMMAND='/usr/local/adinf/openssh_adinf-7.4/sbin/sshd'
AUTO_RUN_FILE='/etc/rc.d/rc.local'

function check_return_exit ()
{
  if ! test 0 = $?; then
    echo -e "\n!!!!!!!!!! Error, this script will exit !!!!!!!!!!\n"
    exit 1
  fi
}

function auto_run_sshd ()
{
  grep -qF "$AUTO_RUN_COMMAND" "$AUTO_RUN_FILE" || echo sudo su - "$EXEC_USER" -c "$AUTO_RUN_COMMAND" | sudo tee -a "$AUTO_RUN_FILE"
}

if [ ! -f "/usr/bin/lsb_release" ]; then
     MAIN_VERSION=`cat /etc/issue |grep release | awk '{print $3}' |cut -d '.' -f 1`
     SUB_VERSION=`cat /etc/issue |grep release | awk '{print $3}' |cut -d '.' -f 2`
 else
     MAIN_VERSION=`lsb_release -a |grep 'Release' | cut -f2 | cut -d '.' -f 1`
     SUB_VERSION=`lsb_release -a |grep 'Release' | cut -f2 | cut -d '.' -f 2`
fi

echo $MAIN_VERSION.$SUB_VERSION
if [ ! -d "/usr/local/adinf/openssh_adinf-7.4" ]; then
    sudo rpm -ivh ./packages/openssh_adinf-7.4-el7.1.x86_64.rpm
fi

sudo sed -i 's/#Port 22/Port 30022/g' /usr/local/adinf/openssh_adinf-7.4/etc/sshd_config
sudo sed -i 's/\.ssh\/authorized_keys/\/usr\/local\/adinf\/ssh_key\/authorized_keys/g' /usr/local/adinf/openssh_adinf-7.4/etc/sshd_config


if [ `grep '^adbot' /etc/passwd | wc -l`"" == "0" ];then
	sudo useradd -s /bin/bash $EXEC_USER
fi

# 增加 adbot sudo 权限
SUDOER_FILENAME="/etc/sudoers.d/ad-core-inner"
sudo su root << EOT
whoami
if [ ! -f $SUDOER_FILENAME ]; then
  touch $SUDOER_FILENAME
fi
EOT
if [ "0" == `sudo cat $SUDOER_FILENAME |grep $EXEC_USER |wc -l`"" ]; then
sudo su root << EOT
  echo "$EXEC_USER ALL=(ALL) NOPASSWD: ALL" >> $SUDOER_FILENAME
EOT
fi

# 此处将会被 install 脚本替换
# `` 命令执行时使用的是当前用户权限
SSH_PUBLIC_KEY="PUBLIC_KEY_REPLACE"
SSH_KEY_PREFIX=/usr/local/adinf/ssh_key
if [ ! -d $SSH_KEY_PREFIX ]; then
  sudo mkdir $SSH_KEY_PREFIX
  sudo chown $EXEC_USER:$EXEC_USER $SSH_KEY_PREFIX
fi
SSH_AUTH_FILE=$SSH_KEY_PREFIX/authorized_keys
sudo su $EXEC_USER << EOT
whoami
if [ ! -f "$SSH_AUTH_FILE" ];then
	touch $SSH_AUTH_FILE
fi
EOT
check_return_exit

if [ "0" == `sudo cat $SSH_AUTH_FILE | grep "$SSH_PUBLIC_KEY" |wc -l` ]; then
sudo su $EXEC_USER << EOT
whoami
  echo "$SSH_PUBLIC_KEY" >> $SSH_AUTH_FILE
EOT
fi
check_return_exit

if [ "1" == `sudo ps -ef |grep sshd|grep adinf |wc -l` ]; then
  SSHD_PID=`sudo ps -ef |grep sshd|grep adinf |awk '{print $2}'`
  sudo kill $SSHD_PID
fi
sudo su $EXEC_USER << EOT
  sudo chown $EXEC_USER:$EXEC_USER /usr/local/adinf/openssh_adinf-7.4/etc/ssh_host_*
 	chmod 600 /usr/local/adinf/openssh_adinf-7.4/etc/ssh_host_*
	/usr/local/adinf/openssh_adinf-7.4/sbin/sshd
EOT
check_return_exit

auto_run_sshd
check_return_exit
