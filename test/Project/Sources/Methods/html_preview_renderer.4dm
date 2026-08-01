//%attributes = {"invisible":true,"preemptive":"capable"}
#DECLARE($rootFolder : 4D:C1709.Folder; $blob : Blob; $info : Object) : 4D:C1709.File

ASSERT:C1129($info.MimeType="text/html")

$rootFolder.create()

var $cid; $type : Text
var $data : Variant  //Blob or Text
var $pathComponents : Collection
var $attachment : Object
var $folder : 4D:C1709.Folder

For each ($attachment; $info.Attachments)
	
	$cid:=$attachment.cid
	$type:=$attachment.type
	$data:=$attachment.data
	$pathComponents:=Split string:C1554($cid; "/")
	
	$folder:=$rootFolder.folder($pathComponents[0])
	$folder.create()
	
	Case of 
		: ($cid="@.html")
			$data:=Replace string:C233($data; "src=\"cid:"; "src=\"../"; *)
			$data:=Replace string:C233($data; "href=\"cid:"; "href=\"../"; *)
			$folder.file($pathComponents[1]).setText($data)
		: ($cid="@.css")
			$data:=Replace string:C233($data; "url('cid:"; "url('../"; *)
			$folder.file($pathComponents[1]).setText($data)
		: ($cid="@.js")
			$folder.file($pathComponents[1]).setText($data)
		Else 
			WRITE PICTURE FILE:C680($folder.file($pathComponents[1]).platformPath; $data)
	End case 
End for each 

$data:=Convert to text:C1012($blob; "utf-8")
$data:=Replace string:C233($data; "src=\"cid:"; "src=\"./"; *)
$data:=Replace string:C233($data; "href=\"cid:"; "href=\"./"; *)

var $file : 4D:C1709.File
$file:=$rootFolder.file("index.html")
$file.setText($data)

return $file