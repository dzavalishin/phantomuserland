#begin
echo "1#=$p0"
echo "p1=$p1" #check
$a="hello"
echo '$a=',$a
echo "starting testapp",exec("testapp")
echo "exit"
$a=3
echo "a = $a"
$b=1
l1:
	if($a==0) goto lus
	$b = $b * $a
	$a = $a - 1
	goto l1
lus:
echo "b = $b"
exit($a)
