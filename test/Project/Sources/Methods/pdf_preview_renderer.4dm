//%attributes = {"invisible":true,"preemptive":"capable"}
#DECLARE($rootFolder : 4D:C1709.Folder; $blob : Blob; $info : Object) : 4D:C1709.File

ASSERT:C1129($info.PreviewContentType="com.adobe.pdf")

$rootFolder.create()

var $picture : Picture
BLOB TO PICTURE:C682($blob; $picture; $info.PreviewContentType)
WRITE PICTURE FILE:C680($rootFolder.file("preview.pdf").platformPath; $picture)

$file:=$rootFolder.file("preview.jpg")

TRANSFORM PICTURE:C988($picture; Scale:K61:2; 4; 4)
WRITE PICTURE FILE:C680($file.platformPath; $picture; "public.jpeg")

return $file