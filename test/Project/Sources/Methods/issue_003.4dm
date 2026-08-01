//%attributes = {}
var $XLSX : 4D:C1709.File
$XLSX:=File:C1566("/RESOURCES/test.xlsx")

var $info : Object
var $blob : Blob
$info:={}
$blob:=QL Create preview($XLSX.platformPath; $info)

var $rootFolder : 4D:C1709.Folder
$rootFolder:=Folder:C1567(fk desktop folder:K87:19).folder("test_xlsx")

$file:=html_preview_renderer($rootFolder; $blob; $info)

OPEN URL:C673($file.platformPath; "Safari")  //local path access must be enabled in safari