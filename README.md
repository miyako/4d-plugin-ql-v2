![version](https://img.shields.io/badge/version-19%2B-5682DF)
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
Height
PDFStyle (0=standard, 3=right, 4=left)
BaseBundlePath (converted to HFS)
TextEncoding
PageElementXPath
MimeType
Width
AllowJavascript
Attachments[]
Attachments[].cid
Attachments[].data
```

For documents with multiple pages, an HTML preview is generally returned.

If the preview document is a single page, the data is returned in ``preview``. You can use ``MimeType`` to find out its internal format. The generator decides what kind of preview to draw. For example, Microsoft Office generators tend to create JPG. Apple iWorks tend to create PDF.

If the preview document has several pages, a container HTML is returned in ``preview``. The ``cid`` and `data` (raw picture) of each page is returned in  ``options.Attachments[]``. 

Note that QuickLook is actually a **collection of generators and viewers**. A "preview" is not limited to image, it can be audio, video, etc. Likewise, the viewer could be a web browser engine, PDF or text viewer, even a media player or stream player (``BaseBundlePath`` indicates the type of viewer). 

