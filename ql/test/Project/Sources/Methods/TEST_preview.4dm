//%attributes = {}
$path:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.key"
$path:=System folder:C487(Desktop:K41:16)+"sample.pdf"

C_BLOB:C604($blob)
$info:=New object:C1471

$blob:=QL Create preview($path; $info)

$rootPath:=System folder:C487(Desktop:K41:16)+Generate UUID:C1066+Folder separator:K24:12

//this is just a proof of concept

Case of 
	: ($info.PreviewContentType="com.adobe.pdf")
		
		CREATE FOLDER:C475($rootPath; *)
		BLOB TO PICTURE:C682($blob; $picture; $info.PreviewContentType)
		WRITE PICTURE FILE:C680($rootPath+"preview.pdf"; $picture)
		
	: ($info.MimeType="text/html")
		CREATE FOLDER:C475($rootPath; *)
		For each ($attachment; $info.Attachments)
			$cid:=$attachment.cid
			$data:=$attachment.data
			$pathComponents:=Split string:C1554($cid; "/")
			CREATE FOLDER:C475($rootPath+$pathComponents[0]+Folder separator:K24:12; *)
			WRITE PICTURE FILE:C680($rootPath+$pathComponents[0]+Folder separator:K24:12+$pathComponents[1]; $data)  //picture is raw data
		End for each 
		$html:=Convert to text:C1012($blob; "utf-8")
		$html:=Replace string:C233($html; "<img src=\"cid:"; "<img src=\"./"; *)
		TEXT TO DOCUMENT:C1237($rootPath+"index.html"; $html)
		SHOW ON DISK:C922($rootPath)  //local path access must be enabled
End case 
