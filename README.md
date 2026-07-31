![version](https://img.shields.io/badge/version-19%2B-5682DF)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-ql-v2)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-ql-v2/total)

# 4d-plugin-ql-v2

A 4D plugin that lets your 4D code generate **thumbnails** and **full previews** for (almost) any file on disk — PDFs, Office documents, images, Keynote/Pages/Numbers files, source code, archives, and anything else your Mac has a QuickLook generator for — without writing a single line of file-format-specific parsing code.

Under the hood it drives macOS's own **QuickLook** engine (the same one behind the Spacebar preview in the Finder), so if Quick Look can show it, this plugin can hand it to you as a `Picture` or a `Blob`.

| Command | Returns | Purpose |
|---|---|---|
| [`QL Create thumbnail`](#ql-create-thumbnail) | Picture | Generates an icon-sized thumbnail for a file |
| [`QL Create preview`](#ql-create-preview) | Blob | Generates a full, rich preview for a file, plus metadata describing it |

**Platforms:** macOS only (Intel & Apple Silicon).

---

## Requirements & platform notes

Read this before using either command — results depend entirely on what QuickLook itself can do on the machine running the code.

- Both commands rely on **Apple's QuickLook engine**, so results depend on which QuickLook generators are installed — built-in system generators, plus any third-party ones the user has (e.g. for `.key`/`.pages`/`.numbers`, `.psd`, source code, etc.). The same file can produce a richer or plainer preview on different machines depending on what's installed.
- **macOS 11 (Big Sur) or later** is required — these commands rely on internal QuickLook APIs that Apple has changed across OS releases, and behave differently (or aren't available) on earlier macOS versions or on Windows.
- Neither command raises a 4D error on failure — see [Error handling & troubleshooting](#error-handling--troubleshooting) for how to guard against that.

---

## QL Create thumbnail

**QL Create thumbnail ( path ) → Picture**

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | Full path of the file to generate a thumbnail for. |
| *(2nd parameter)* | — | **Obsolete.** Previously accepted a size; it is now ignored. Thumbnails are always generated at a maximum of **1024 × 1024 px**. Passing it does nothing and can be omitted. |
| Result | Picture | The generated thumbnail. If no thumbnail could be generated at all, an empty picture is returned. |

### Description

`QL Create thumbnail` asks QuickLook for a thumbnail of the file at `path`. If the file type has no QuickLook thumbnail generator (or generation fails for some other reason), the command **automatically falls back to the file's Finder icon** instead, so it practically always returns *some* image for a valid, existing file.

The returned picture is always encoded as **TIFF**, sized to fit within 1024 × 1024 px (aspect ratio preserved by QuickLook itself).

### Example

```4d
$path:=Folder(fk desktop folder).file("sample.png").platformPath
C_PICTURE($picture)
$picture:=QL Create thumbnail($path)
FORM SET INPUT($picture)  // e.g. drop into a picture variable/field
```

Batch/stress-test style usage (from the plugin's own test suite):

```4d
$path:=System folder(Current resources folder)+"sample.png"
C_PICTURE($picture)
For ($i; 1; 1000)
	$picture:=QL Create thumbnail($path)  //$2 obsolete; size is always 1024x1024
End for
```

---

## QL Create preview

**QL Create preview ( path ; info ) → Blob**

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | Full path of the file to preview. |
| `info` | Object | Pass an **empty object** (`New object` or `{}`). The command fills it in (objects are shared by reference in 4D, so you don't need a special "by reference" syntax — just pass the variable). See [The `info` object](#the-info-object) below. |
| Result | Blob | The raw preview payload. Its format depends on the file type — see below. If preview generation fails, an empty blob is returned. |

### Description

Depending on the source file type, QuickLook returns the preview in one of two shapes. Always check `info.PreviewContentType` / `info.MimeType` to know which one you got:

- **PDF-style preview** (`info.PreviewContentType = "com.adobe.pdf"`)
  The `Blob` **is** a PDF file's raw bytes. Typical files that produce this: PDFs, and some other document types QuickLook chooses to render as PDF.
- **HTML-style preview** (`info.MimeType = "text/html"`)
  The `Blob` is a **UTF-8 HTML document**. This is what you'll get for most Office documents (`.docx`, `.xlsx`, `.pptx`), iWork documents (`.key`, `.pages`, `.numbers`), and many others. The HTML references embedded resources (images, CSS, JS) via `cid:` URLs; those resources are **not** inside the blob — they're returned separately, one entry per resource, in `info.Attachments` (see below). You are responsible for writing the HTML and its attachments to disk (or otherwise resolving the `cid:` references) before you can display it — see [`html_preview_renderer`](#html_preview_renderer) below for a ready-made implementation.

### The `info` object

After the call, `info` is populated with whichever of the following properties the QuickLook generator reported for that file (not all properties are present for every file type):

| Property | Type | Description |
|---|---|---|
| `DisplayBundleID` | Text | Bundle identifier of the QuickLook generator/app that produced the preview (e.g. `com.apple.quicklook.???`). Useful for diagnostics/logging. |
| `PreviewContentType` | Text | UTI of the preview payload — e.g. `"com.adobe.pdf"` for a PDF-style preview. |
| `MimeType` | Text | MIME type of the preview payload — e.g. `"text/html"` for an HTML-style preview. |
| `TextEncoding` | Text | Text encoding of the HTML/text payload, when applicable. |
| `PageElementXPath` | Text | Internal XPath QuickLook uses to mark the current page/element (rarely populated). |
| `BaseBundlePath` | Text | Path to the QuickLook generator bundle that was used. |
| `Height` / `Width` | Real | Native pixel dimensions of the preview content, when reported. |
| `PDFStyle` | Real | PDF paging style identifier reported by the generator (internal/rarely used). |
| `AllowJavascript` | Boolean | Whether the HTML preview is permitted to run JavaScript. |
| `AllowNetworkAccess` | Boolean | Whether the HTML preview is permitted to access the network. |
| `Attachments` | Collection | One element per embedded resource referenced from the HTML/text preview (see table below). Empty/absent for previews that don't reference any resource, such as a plain PDF preview. |

Each element of `Attachments` is an object:

| Property | Type | Description |
|---|---|---|
| `cid` | Text | The resource's content identifier, as referenced in the main document via `cid:...` links — typically looks like a relative path, e.g. `"index/style.css"`. Use it to reconstruct a folder layout and to rewrite `cid:` links into real relative paths. |
| `type` | Text | MIME type of the resource, e.g. `"text/css"`, `"text/javascript"`, `"image/png"`. |
| `encoding` | Text | Text encoding of the resource, when `type` is a text-based type. |
| `data` | Text or Picture | The resource's payload — `Text` for text-based resources (html/css/js), `Picture` for image resources. |

### Example

```4d
$path:=Get 4D folder(Current resources folder)+"sample.key"
C_BLOB($blob)
$info:=New object
$blob:=QL Create preview($path; $info)
Case of
	: ($info.PreviewContentType="com.adobe.pdf")
		// PDF-style preview -> $blob is raw PDF bytes
		var $rootFolder : 4D.Folder
		$rootFolder:=Folder(fk desktop folder).folder("test_pdf")
		$file:=pdf_preview_renderer($rootFolder; $blob; $info)
		OPEN URL($file.platformPath; "Safari")
	: ($info.MimeType="text/html")
		// HTML-style preview -> $blob is HTML text + $info.Attachments
		var $rootFolder : 4D.Folder
		$rootFolder:=Folder(fk desktop folder).folder("test_key")
		$file:=html_preview_renderer($rootFolder; $blob; $info)
		OPEN URL($file.platformPath; "Safari")  //local path access must be enabled in Safari
End case
```

---

## Reference helper methods

`QL Create preview`'s result is intentionally "raw" — a blob plus metadata — so it can be consumed however you like (saved, streamed, inspected, etc.). In practice you'll almost always want to turn it into something you can actually look at. The two project methods below are ready-to-use implementations of that step for the two payload shapes described above; drop them into your project as-is, or use them as a starting point.

### `pdf_preview_renderer`

Turns a PDF-style preview blob into a real `.pdf` file on disk, plus a scaled-down `.jpg` for quick display.

**pdf_preview_renderer ( rootFolder ; blob ; info ) → 4D.File**

| Parameter | Type | Description |
|---|---|---|
| `rootFolder` | 4D.Folder | Destination folder (created if it doesn't exist). |
| `blob` | Blob | The blob returned by `QL Create preview` — must be a PDF-style preview (`info.PreviewContentType = "com.adobe.pdf"`). |
| `info` | Object | The `info` object filled in by `QL Create preview` for the same call. |
| Result | 4D.File | The generated `preview.jpg` file. |

```4d
#DECLARE($rootFolder : 4D.Folder; $blob : Blob; $info : Object) : 4D.File
ASSERT($info.PreviewContentType="com.adobe.pdf")
$rootFolder.create()
var $picture : Picture
BLOB TO PICTURE($blob; $picture; $info.PreviewContentType)
WRITE PICTURE FILE($rootFolder.file("preview.pdf").platformPath; $picture)
$file:=$rootFolder.file("preview.jpg")
TRANSFORM PICTURE($picture; Scale; 4; 4)
WRITE PICTURE FILE($file.platformPath; $picture; "public.jpeg")
return $file
```

### `html_preview_renderer`

Turns an HTML-style preview (blob + `info.Attachments`) into a browsable folder: writes `index.html`, recreates the attachment folder structure from each attachment's `cid`, writes each attachment to disk (text files as text, images via `WRITE PICTURE FILE`), and rewrites every `cid:` link in the HTML/CSS to a relative path.

**html_preview_renderer ( rootFolder ; blob ; info ) → 4D.File**

| Parameter | Type | Description |
|---|---|---|
| `rootFolder` | 4D.Folder | Destination folder (created if it doesn't exist). |
| `blob` | Blob | The blob returned by `QL Create preview` — must be an HTML-style preview (`info.MimeType = "text/html"`). |
| `info` | Object | The `info` object filled in by `QL Create preview` for the same call (its `Attachments` collection is what gets written to disk). |
| Result | 4D.File | The generated `index.html` file. |

```4d
#DECLARE($rootFolder : 4D.Folder; $blob : Blob; $info : Object) : 4D.File
ASSERT($info.MimeType="text/html")
$rootFolder.create()
var $cid; $type : Text
var $data : Variant  //Blob or Text
var $pathComponents : Collection
var $attachment : Object
var $folder : 4D.Folder
For each ($attachment; $info.Attachments)
	$cid:=$attachment.cid
	$type:=$attachment.type
	$data:=$attachment.data
	$pathComponents:=Split string($cid; "/")
	$folder:=$rootFolder.folder($pathComponents[0])
	$folder.create()
	Case of
		: ($cid="@.html")
			$data:=Replace string($data; "src=\"cid:"; "src=\"../"; *)
			$data:=Replace string($data; "href=\"cid:"; "href=\"../"; *)
			$folder.file($pathComponents[1]).setText($data)
		: ($cid="@.css")
			$data:=Replace string($data; "url('cid:"; "url('../"; *)
			$folder.file($pathComponents[1]).setText($data)
		: ($cid="@.js")
			$folder.file($pathComponents[1]).setText($data)
		Else
			WRITE PICTURE FILE($folder.file($pathComponents[1]).platformPath; $data)
	End case
End for each
$data:=Convert to text($blob; "utf-8")
$data:=Replace string($data; "src=\"cid:"; "src=\"./"; *)
$data:=Replace string($data; "href=\"cid:"; "href=\"./"; *)
var $file : 4D.File
$file:=$rootFolder.file("index.html")
$file.setText($data)
return $file
```

---

## Full worked examples

### Preview any file and open it in Safari

```4d
var $PDF : 4D.File
$PDF:=File("/RESOURCES/test.pdf")
var $info : Object
var $blob : Blob
$info:={}
$blob:=QL Create preview($PDF.platformPath; $info)
var $rootFolder : 4D.Folder
$rootFolder:=Folder(fk desktop folder).folder("test_pdf")
$file:=pdf_preview_renderer($rootFolder; $blob; $info)
OPEN URL($file.platformPath; "Safari")
```

The exact same pattern works unchanged for `.xlsx`, `.pptx`, `.docx`, `.key`, `.numbers`, etc. — just swap in `html_preview_renderer` since those file types produce an HTML-style preview rather than a PDF-style one:

```4d
var $XLSX : 4D.File
$XLSX:=File("/RESOURCES/test.xlsx")
var $info : Object
var $blob : Blob
$info:={}
$blob:=QL Create preview($XLSX.platformPath; $info)
var $rootFolder : 4D.Folder
$rootFolder:=Folder(fk desktop folder).folder("test_xlsx")
$file:=html_preview_renderer($rootFolder; $blob; $info)
OPEN URL($file.platformPath; "Safari")  //local path access must be enabled in Safari
```

### Generic dispatcher (don't assume the content type ahead of time)

If you don't know in advance whether a file will render as a PDF-style or HTML-style preview, branch on `info` after the call, as in the plugin's own test method:

```4d
$path:=Get 4D folder(Current resources folder)+"sample.key"
C_BLOB($blob)
$info:=New object
$blob:=QL Create preview($path; $info)
Case of
	: ($info.PreviewContentType="com.adobe.pdf")
		var $rootFolder : 4D.Folder
		$rootFolder:=Folder(fk desktop folder).folder("test_pdf")
		$file:=pdf_preview_renderer($rootFolder; $blob; $info)
		OPEN URL($file.platformPath; "Safari")
	: ($info.MimeType="text/html")
		var $rootFolder : 4D.Folder
		$rootFolder:=Folder(fk desktop folder).folder("test_key")
		$file:=html_preview_renderer($rootFolder; $blob; $info)
		OPEN URL($file.platformPath; "Safari")  //local path access must be enabled in Safari
End case
```

---

## Error handling & troubleshooting

Neither command reports failure with a 4D error signal — a failed or empty result simply comes back empty. Practical guidance:

- **Failure is silent, not an error.** You get an empty `Picture` back from `QL Create thumbnail`, or an empty `Blob` back from `QL Create preview`, rather than a 4D error. Always check the result (e.g. `Length(blob)=0`, or `Height(picture)+Width(picture)=0`) before acting on it.
- **`info` must exist before the call.** Pass an initialized object (`New object` or `{}`) as the second parameter to `QL Create preview` — a `Null` object will not be populated.
- **Not every `info` property is always present.** Only the properties the underlying QuickLook generator actually reports for that file type get set; a plain PDF, for example, will not have an `Attachments` collection.
- **Safari + local files.** When opening a generated `index.html`/`preview.jpg` with `OPEN URL(...; "Safari")`, Safari's *"Allow local file access"* / *Disable Local File Restrictions* setting must be enabled, or the page (and its relative-path attachments) won't load.
- **Thumbnails always fall back to the Finder icon.** If you specifically need to know whether QuickLook actually rendered the file versus just returning the generic file icon, use `QL Create preview` instead and inspect `info` — `QL Create thumbnail` doesn't expose that distinction.
- **Results vary by machine.** Since generation depends on installed QuickLook generators, don't assume a preview that works on your development Mac will look identical (or work at all) on a machine with different generators installed.

---

## Quick reference

```4d
 // Icon-sized thumbnail
$thumb:=QL Create thumbnail($path)

 // Full preview + metadata
$info:=New object
$blob:=QL Create preview($path; $info)
Case of
	: ($info.PreviewContentType="com.adobe.pdf")
		$file:=pdf_preview_renderer($rootFolder; $blob; $info)
	: ($info.MimeType="text/html")
		$file:=html_preview_renderer($rootFolder; $blob; $info)
End case
```
