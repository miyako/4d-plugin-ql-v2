//%attributes = {}
var $PDF : 4D:C1709.File
$PDF:=File:C1566("/RESOURCES/test.pdf")

var $info : Object
var $blob : Blob
$info:={}
$blob:=QL Create preview($PDF.platformPath; $info)

var $rootFolder : 4D:C1709.Folder
$rootFolder:=Folder:C1567(fk desktop folder:K87:19).folder("test_pdf")

$file:=pdf_preview_renderer($rootFolder; $blob; $info)

OPEN URL:C673($file.platformPath; "Safari")