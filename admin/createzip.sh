echo "I assume, you already created the main patch manually."

find zip/othernew/ -type f |sed -e "s|zip/othernew/||"|xargs -n 1 -iFILE cp FILE zip/othernew/FILE
rm -f bc-source.zip
zip -r9q bc-source.zip zip/
