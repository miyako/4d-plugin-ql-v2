//%attributes = {}
$path:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.key"

C_BLOB:C604($blob)
$info:=New object:C1471

$blob:=QL Create preview($path; $info)

Case of 
	: ($info.PreviewContentType="com.adobe.pdf")
		
		//this is just a proof of concept
		$rootPath:=System folder:C487(Desktop:K41:16)+Generate UUID:C1066+Folder separator:K24:12
		CREATE FOLDER:C475($rootPath; *)
		BLOB TO PICTURE:C682($blob; $picture; $info.PreviewContentType)
		WRITE PICTURE FILE:C680($rootPath+"preview.pdf"; $picture)
		
		TRANSFORM PICTURE:C988($picture; Scale:K61:2; 4; 4)
		WRITE PICTURE FILE:C680($rootPath+"preview.jpg"; $picture; "public.jpeg")
		
	: ($info.MimeType="text/html")
		
		var $rootFolder : 4D:C1709.Folder
		$rootFolder:=Folder:C1567(fk desktop folder:K87:19).folder("test_xlsx")
		
		$file:=html_preview_renderer($rootFolder; $blob; $info)
		
		OPEN URL:C673($file.platformPath; "Safari")  //local path access must be enabled in safari
		
End case 
