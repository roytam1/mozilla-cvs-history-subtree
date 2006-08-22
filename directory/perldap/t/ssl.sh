#!/bin/sh

LD_LIBRARY_PATH=/home/rich/mozilla/dist/lib ; export LD_LIBRARY_PATH

#argsfile=.gdbinit
#echo "break main" > $argsfile
#echo "run -I../blib/arch -I../blib/lib ssl.pl -h localhost -p 10202 -P /home/rich/102srv/alias/slapd-localhost-cert8.db -N Server-Cert -W 723d1e6cf342e1d133c69876e938be08e41bad41 -b '' objectclass=*" >> $argsfile
#gdb -x $argsfile perl
#rm -f $argsfile
perl -d -I../blib/arch -I../blib/lib ssl.pl -h localhost -Z -p 10200 -P /home/rich/102srv/alias/slapd-localhost-cert8.db -N "Server-Cert" -W `cat /home/rich/102srv/alias/pwdfile.txt` -b "" "objectclass=*" 
#perl -d -I../blib/arch -I../blib/lib ssl.pl -h localhost -p 10202 -P /home/rich/102srv/alias/slapd-localhost-cert8.db -N "Server-Cert" -W `cat /home/rich/102srv/alias/pwdfile.txt` -b "" "objectclass=*" 
