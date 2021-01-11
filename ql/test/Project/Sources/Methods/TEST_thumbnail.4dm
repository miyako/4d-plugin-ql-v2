//%attributes = {}
$path:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.key"

C_PICTURE:C286($picture)
$picture:=QL Create thumbnail ($path)  //$2 obsolete; size is always 1024x1024

SET PICTURE TO PASTEBOARD:C521($picture)