![version](https://img.shields.io/badge/version-17-3E8B93)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-ql-v2)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-ql-v2/total)

# 4d-plugin-ql-v2
Use QuickLook API to generate thumbnails and previews.

### Remarks

to dump "c" symbols from ``qlmanage``

```
nm -g /usr/bin/qlmanage
```

to dump "obj-c" symbols from ``qlmanage``

```
otool -ov /usr/bin/qlmanage
```

or use [class-dump](https://github.com/nygard/class-dump)

previous version was using a hidden function ``QLPreviewCopyData`` to generate preview.

this API has been removed on Catalina and Big Sur.

current version is using ``- synchronousGetData`` of the hidden ``QLPreview`` class.

### Syntax

```
thumbnail:=QL Create thumbnail (path;options)
```

Parameter|Type|Description
------------|------------|----
path|Text|
thumbnail|Picture|


```
preview:=QL Create preview (path;options)
```

Parameter|Type|Description
------------|------------|----
path|Text|
options|Object|
preview|BLOB|

Possible properties of ``options`` on output:

```
height
PDFStyle (0=standard, 3=right, 4=left)
baseBundlePath (converted to HFS)
textEncoding
pageElementXPath
mimeType
width
allowJavascript
```

For documents with multiple pages, an HTML preview is generally returned.

If the preview document is a single page, the data is returned in ``preview``. You can use ``mimeType`` to find out its internal format. The generator decides what kind of preview to draw. For example, Microsoft Office generators tend to create JPG. Apple iWorks tend to create PDF.

If the preview document has several pages, the data is returned in ``data`` and a container HTML is returned in ``preview``. The ``cid`` of each page is returned in  ``type``. 

Note that QuickLook is actually a **collection of generators and viewers**. A "preview" is not limited to image, it can be audio, video, etc. Likewise, the viewer could be a web browser engine, PDF or text viewer, even a media player or stream player (``baseBundlePath`` indicates the type of viewer). 

