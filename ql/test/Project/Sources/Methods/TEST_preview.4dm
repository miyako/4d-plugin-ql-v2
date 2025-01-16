//%attributes = {}
$path:=Get 4D folder:C485(Current resources folder:K5:16)+"sample.key"

C_BLOB:C604($blob)
$info:=New object:C1471

$blob:=QL Create preview($path; $info)

Case of 
	: ($info.PreviewContentType="com.adobe.pdf")
		
		var $rootFolder : 4D:C1709.Folder
		$rootFolder:=Folder:C1567(fk desktop folder:K87:19).folder("test_pdf")
		
		$file:=pdf_preview_renderer($rootFolder; $blob; $info)
		
		OPEN URL:C673($file.platformPath; "Safari")
		
	: ($info.MimeType="text/html")
		
		var $rootFolder : 4D:C1709.Folder
		$rootFolder:=Folder:C1567(fk desktop folder:K87:19).folder("test_key")
		
		$file:=html_preview_renderer($rootFolder; $blob; $info)
		
		OPEN URL:C673($file.platformPath; "Safari")  //local path access must be enabled in safari
		
End case 
