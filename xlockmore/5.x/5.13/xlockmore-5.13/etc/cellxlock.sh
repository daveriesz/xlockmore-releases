# converts a file from xl4d format to xlock life3d format
sed "1,\$s/10/-6,/g" $1 > $1$$
sed "1,\$s/11/-5,/g" $1$$ > $1
sed "1,\$s/12/-4,/g" $1 > $1$$
sed "1,\$s/13/-3,/g" $1$$ > $1
sed "1,\$s/14/-2,/g" $1 > $1$$
sed "1,\$s/15/-1,/g" $1$$ > $1
sed "1,\$s/16/0,/g" $1 > $1$$
sed "1,\$s/17/1,/g" $1$$ > $1
sed "1,\$s/18/2,/g" $1 > $1$$
sed "1,\$s/19/3,/g" $1$$ > $1
rm -f $1$$
