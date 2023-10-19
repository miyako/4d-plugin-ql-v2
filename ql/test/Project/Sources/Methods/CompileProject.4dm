//%attributes = {}
var $oOptionObject : Object
$oOptionObject:=New object:C1471()
$oOptionObject.targets:=New collection:C1472()  // only syntax check
$oOptionObject.typeInference:="none"  // all vars are typedvar $oOptionObject : Object
$oOptionObject:=New object:C1471()
$oOptionObject.targets:=New collection:C1472()  // only syntax check
$oOptionObject.typeInference:="none"  // all vars are typed

$oCompileResult:=Compile project:C1760($oOptionsObject)

LOG EVENT:C667(Into system standard outputs:K38:9; "Compilation returns:\n")
LOG EVENT:C667(Into system standard outputs:K38:9; JSON Stringify:C1217($oCompileResult; *))

QUIT 4D:C291