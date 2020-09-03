# converts a file from xlock life3d format to xl4d
sed "1,\$s/-8,/ 8/g" $1 > $1$$
sed "1,\$s/-7,/ 9/g" $1$$ > $1
sed "1,\$s/-6,/10/g" $1 > $1$$
sed "1,\$s/-5,/11/g" $1$$ > $1
sed "1,\$s/-4,/12/g" $1 > $1$$
sed "1,\$s/-3,/13/g" $1$$ > $1
sed "1,\$s/-2,/14/g" $1 > $1$$
sed "1,\$s/-1,/15/g" $1$$ > $1
sed "1,\$s/-0,/16/g" $1 > $1$$
sed "1,\$s/0,/16/g" $1$$ > $1
sed "1,\$s/1,/17/g" $1 > $1$$
sed "1,\$s/2,/18/g" $1$$ > $1
sed "1,\$s/3,/19/g" $1 > $1$$
sed "1,\$s/4,/20/g" $1$$ > $1
sed "1,\$s/5,/21/g" $1 > $1$$
sed "1,\$s/6,/22/g" $1$$ > $1
sed "1,\$s/        //g" $1 > $1$$
sed "/^$/d" $1$$ > $1
sed "1,\$s/	//g" $1 > $1$$
fold -w 9 $1$$ > $1
sed "1,\$s/ $//g" $1 > $1$$
#rm -f $1$$
mv $1$$ $1