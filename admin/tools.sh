#various useful commands:

#list all dynamic libraries that mozilla binaries depend on
LD_LIBRARY_PATH="." find . -name "*.so" -or -name "mozilla-bin"|xargs -n 1 ldd|sed -e "s/ (.*//"|grep -v "=> ./"|sort|uniq
gpg --detach-sign --local-user 136A1D4D filename
scp filename benb@spruce.he.net:/home/ftp/pub/benb/master/communicator/0.8/
scp filename benb@login.ibiblio.org:/pub/packages/infosystems/WWW/clients/beonex/communicator/0.8/
