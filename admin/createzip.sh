echo "I assume, you already created the main patch manually."

find admin/othernew/ -type f |sed -e "s|admin/othernew/||"|xargs -n 1 -iFILE cp FILE admin/othernew/FILE
rm -f bc-source.zip
zip -r9q bc-source.zip admin/
