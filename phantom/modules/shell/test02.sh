echo "Calculate $p1!"
if($argc != 1) goto error
$no=number($p1)
$res=1
lus:
if($no==0) goto end
$res = $res * $no
$no = $no -1
goto lus
end:
echo "$p1!=$res"
exit(0)
error:
echo "parameters :"
echo "$p0 <number>"
exit(1)
