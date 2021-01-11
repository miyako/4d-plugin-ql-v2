//%attributes = {}
$path:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.key"

C_BLOB:C604($blob)
$info:=New object:C1471

$blob:=QL Create preview ($path;$info)
