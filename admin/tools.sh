#various useful commands:

#list all dynamic libraries that mozilla binaries depend on
LD_LIBRARY_PATH="." find . -name "*.so" -or -name "mozilla-bin"|xargs -n 1 ldd|sed -e "s/ (.*//"|grep -v "=> ./"|sort|uniq
gpg --detach-sign --local-user 136A1D4D filename
scp -F /etc/ssh/ssh_proxy filename ben@firespit.beonex.com:/data/fileshare/rsync/beonex/communicator/0.8/
scp filename benb@login.ibiblio.org:/pub/packages/infosystems/WWW/clients/beonex/communicator/0.8/
find . -name "Entries*" -path "*/CVS/*" -print0|xargs -0 -n 1 ssed -i -e "s|//TMOZILLA_1_0_BRANCH|//TBEONEX_0_8_BRANCH|"
