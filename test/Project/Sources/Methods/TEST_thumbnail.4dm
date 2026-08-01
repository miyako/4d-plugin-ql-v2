//%attributes = {}
$path:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.key"
$path:=System folder:C487(Current resources folder:K5:16)+"sample.png"

C_PICTURE:C286($picture)
For ($i; 1; 1000)
	$picture:=QL Create thumbnail($path)  //$2 obsolete; size is always 1024x1024
End for 

//SET PICTURE TO PASTEBOARD($picture)