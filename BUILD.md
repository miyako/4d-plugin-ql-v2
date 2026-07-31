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