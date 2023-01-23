# htap
A hybrid transactional (RocksDB), analytical (Ceph) processing key/value store that is available, consistent, and scalable. The store is logically an LSM tree mapped in the system consisted of RocksDB instances and a Ceph storage cluster. 

## Level-0 MemTable

## Ceph Storage Cluster

### Deploying a Ceph storage cluster
#### prerequirement
Ceph requires nodes to have the following, among which on a typical Linux machine only Docker is missing. 
- Python 3
- Systemd
- NTP
- LVM2
- Docker
#### To install Docker on Ubuntu:
% apt-get update
% apt-get install ca-certificates curl gnupg lsb-release
% mkdir -p /etc/apt/keyrings
% curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
% echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
% apt-get update
% chmod a+r /etc/apt/keyrings/docker.gpg
% apt-get update
% apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin

#### install cephadm (as root):
% curl --silent --remote-name --location https://github.com/ceph/ceph/raw/quincy/src/cephadm/cephadm
% chmod +x cephadm
% ./cephadm add-repo --release quincy
% ./cephadm install

#### bootstrap 1st node:
% cephadm bootstrap --mon-ip *<mon-ip>*

#### enable ceph cli: 
% cephadm install ceph-common

#### add hosts:
% ssh-copy-id -f -i /etc/ceph/ceph.pub root@*<host-2>*
% ssh-copy-id -f -i /etc/ceph/ceph.pub root@*<host-3>*

% ceph orch host add *<host-2>* [*<ip-host-2>*] --labels _admin

#### create osd
% cephadm ceph-volume lvm list /dev/sdb

If the above returns nothing or an error, then need to do the following: 
% /usr/bin/ceph auth get client.bootstrap-osd > /var/lib/ceph/bootstrap-osd/ceph.keyring
% ceph-volume lvm prepare --bluestore --data /dev/sdb

Then:
% ceph orch apply osd --all-available-devices
% ceph orch daemon add osd *<host1>*:/dev/sdb