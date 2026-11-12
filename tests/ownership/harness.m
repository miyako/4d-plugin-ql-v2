/*
 * Ownership regressions for the QL plugin (Objective-C, non-ARC, like the
 * Xcode target, which compiles 4DPlugin-QL.cpp as sourcecode.c.objc).
 *
 * The real 4DPlugin-QL.cpp is #included so its static helpers are
 * reachable; it is resolved through -I <source-root>, so the same harness
 * tests the baseline and the fixed tree. The bundled, unmodified
 * 4DPluginAPI.c is linked as is; only Call4D is replaced (engine.cpp).
 *
 * QuickLook's private QLPreview is replaced by FakeQLPreview (by overriding
 * +[QLPreview alloc]), which returns synthetic data and properties. No file
 * is read and no QuickLook generator runs for `QL Create preview`.
 */

#include "4DPlugin-QL.cpp"

#include "engine.h"

#import <objc/runtime.h>
#include <stdio.h>
#include <string.h>

/* ---------- fake QLPreview ---------- */

static NSData *g_ql_data = nil;
static NSDictionary *g_ql_props = nil;
static int g_fake_live = 0;
static int g_fake_inits = 0;

@interface FakeQLPreview : NSObject
@end

@implementation FakeQLPreview
- (id)initWithURL:(NSURL *)url options:(NSDictionary *)options {
    if((self = [super init])) { g_fake_live++; g_fake_inits++; }
    return self;
}
- (void)dealloc { g_fake_live--; [super dealloc]; }
- (id)synchronousGetData { return g_ql_data; }
- (id)getProperties { return g_ql_props; }
- (NSString *)displayBundleID { return @"com.example.fake-generator"; }
- (NSString *)previewContentType { return @"public.html"; }
@end

static id fake_alloc(id self, SEL _cmd) { return [FakeQLPreview alloc]; }
static id fake_alloc_with_zone(id self, SEL _cmd, NSZone *zone) { return [FakeQLPreview alloc]; }

static void install_fake_quicklook(void) {
    Class meta = object_getClass([QLPreview class]);
    class_replaceMethod(meta, @selector(alloc), (IMP)fake_alloc, "@@:");
    class_replaceMethod(meta, @selector(allocWithZone:), (IMP)fake_alloc_with_zone, "@@:^v");
}

/* ---------- helpers ---------- */

static int check(int ok, const char *what) {
    if(!ok) fprintf(stderr, "  check failed: %s\n", what);
    return ok;
}

static NSData *bytes(const char *s) { return [NSData dataWithBytes:s length:strlen(s)]; }

/* Calls the real `QL Create preview` entry point. Returns the blob handle
   (owned by the caller, playing the engine). */
static void *call_preview(const char *hfsPath, void *object) {
    PA_Unistring path = eng_U(hfsPath);
    PA_ObjectRef p2 = (PA_ObjectRef)object;
    void *args[3] = { &path, &p2, NULL };
    PA_Handle result = NULL;
    PluginBlock pb;
    pb.fParameters = args;
    pb.fResult = &result;
    pb.fData = NULL;
    PluginMain(2, &pb);
    eng_dispose_string(&path);
    return result;
}

static int blob_is(void *h, NSData *expected) {
    long n = eng_handle_size(h);
    if(!check(n == (long)[expected length], "blob length")) return 0;
    return check(n == 0 || !memcmp(eng_handle_data(h), [expected bytes], (size_t)n), "blob content");
}

static NSDictionary *attachment(NSString *mime, NSString *encoding, NSData *data) {
    NSMutableDictionary *d = [NSMutableDictionary dictionary];
    if(mime) d[(NSString *)kQLPreviewPropertyMIMETypeKey] = mime;
    if(encoding) d[(NSString *)kQLPreviewPropertyTextEncodingNameKey] = encoding;
    if(data) d[(NSString *)kQLPreviewPropertyAttachmentDataKey] = data;
    return d;
}

static NSDictionary *attachments_props(void) {
    return @{
        (NSString *)kQLPreviewPropertyAttachmentsKey: @{
            @"style.css": attachment(@"text/css", @"UTF-8", bytes("body{}")),
            @"image.png": attachment(@"image/png", nil, bytes("\x89PNG-synthetic")),
        },
        @"MimeType": @"text/html",
    };
}

/* ---------- tests ---------- */

/* LIVE: every text property written by `QL Create preview` went through
   ob_set_s, which disposed its value Unistring and then cleared the
   variable holding the same (shallow-set) buffer: a double free. */
static int defect_cmd_preview_metadata_double_dispose(void) {
    g_ql_data = bytes("<html>synthetic</html>");
    g_ql_props = @{ @"MimeType": @"text/html", @"TextEncoding": @"UTF-8",
                    @"Width": @640, @"AllowJavascript": @NO };
    void *o = eng_new_object(0);
    void *blob = call_preview("Macintosh HD:tmp:ql-ownership.html", o);
    int ok = blob_is(blob, g_ql_data);
    ok &= check(eng_prop_text_is(o, "DisplayBundleID", "com.example.fake-generator"), "DisplayBundleID");
    ok &= check(eng_prop_text_is(o, "PreviewContentType", "public.html"), "PreviewContentType");
    ok &= check(eng_prop_text_is(o, "MimeType", "text/html"), "MimeType");
    ok &= check(eng_prop_text_is(o, "TextEncoding", "UTF-8"), "TextEncoding");
    ok &= check(eng_prop_real(o, "Width") == 640.0, "Width");
    ok &= check(eng_prop_bool(o, "AllowJavascript") == 0, "AllowJavascript");
    ok &= check(eng_violations("double_dispose_string") == 0, "each value string disposed exactly once");
    ok &= check(g_fake_inits == 1 && g_fake_live == 0, "QLPreview created once and released");
    eng_release(o);
    eng_free_handle(blob);
    return eng_no_leaks() && ok;
}

static int defect_ob_set_s_double_dispose(void) {
    void *o = eng_new_object(0);
    ob_set_s((PA_ObjectRef)o, @"key", @"value");
    int ok = check(eng_prop_text_is(o, "key", "value"), "property stored");
    ok &= check(eng_violations("double_dispose_string") == 0, "value disposed exactly once");
    eng_release(o);
    return ok;
}

static int defect_ob_set_s_empty_owner_leak(void) {
    void *o = eng_new_object(0);
    ob_set_s((PA_ObjectRef)o, @"key", @"value");
    eng_release(o);
    return check(!eng_string_left(""), "no empty string left over from PA_CreateVariable(eVK_Unistring)");
}

/* LIVE (non-text attachment data): PA_CreateVariable(eVK_Picture) allocated
   an empty picture that the shallow PA_SetPictureVariable overwrote. */
static int defect_ob_set_i_empty_picture_leak(void) {
    void *o = eng_new_object(0);
    PA_Picture p = PA_CreatePicture((void *)"x", 1);
    ob_set_i((PA_ObjectRef)o, @"data", p); /* consumes p */
    int ok = check(eng_prop_live_picture(o, "data"), "picture stored");
    ok &= check(!eng_live_picture(p), "the caller's picture was consumed");
    ok &= check(eng_picture_count() == 1, "only the stored copy is alive");
    eng_release(o);
    return eng_no_leaks() && ok;
}

/* LIVE: attachments (text and binary) are written into a collection. */
static int defect_cmd_preview_attachments(void) {
    g_ql_data = bytes("<html><img src=\"cid:image.png\"></html>");
    g_ql_props = attachments_props();
    void *o = eng_new_object(0);
    void *blob = call_preview("Macintosh HD:tmp:ql-ownership.html", o);
    int ok = blob_is(blob, g_ql_data);
    void *c = eng_prop_ref(o, "Attachments");
    ok &= check(c && eng_item_count(c) == 2, "two attachments");
    int css = 0, png = 0;
    for(size_t i = 0; c && i < eng_item_count(c); i++) {
        void *a = eng_item_ref(c, i);
        if(!a) continue;
        if(eng_prop_text_is(a, "cid", "style.css") && eng_prop_text_is(a, "type", "text/css")
           && eng_prop_text_is(a, "encoding", "UTF-8") && eng_prop_text_is(a, "data", "body{}")) css++;
        if(eng_prop_text_is(a, "cid", "image.png") && eng_prop_text_is(a, "type", "image/png")
           && eng_prop_live_picture(a, "data")) png++;
    }
    ok &= check(css == 1 && png == 1, "css text and png picture attachments");
    ok &= check(eng_prop_text_is(o, "MimeType", "text/html"), "MimeType");
    ok &= check(eng_violations("double_dispose_string") == 0, "no string disposed twice");
    eng_release(o);
    eng_free_handle(blob);
    return eng_no_leaks() && ok;
}

/* LIVE: the attachments collection was created even when the preview has
   no attachments, and then never released. */
static int defect_cmd_preview_no_attachments_collection_leak(void) {
    g_ql_data = bytes("%PDF-synthetic");
    g_ql_props = @{ @"Height": @480, @"PDFStyle": @3 };
    void *o = eng_new_object(0);
    void *blob = call_preview("Macintosh HD:tmp:ql-ownership.pdf", o);
    int ok = blob_is(blob, g_ql_data);
    ok &= check(eng_prop_real(o, "Height") == 480.0 && eng_prop_real(o, "PDFStyle") == 3.0, "numbers stored");
    ok &= check(eng_prop_type(o, "Attachments") == -1, "no Attachments property");
    ok &= check(eng_object_count() == 1, "only the caller's object is alive");
    eng_release(o);
    eng_free_handle(blob);
    return eng_no_leaks() && ok;
}

/* LIVE: with no object argument the attachments collection (and the
   attachment objects in it) were built but never stored nor released. */
static int defect_cmd_preview_null_object_leak(void) {
    g_ql_data = bytes("<html/>");
    g_ql_props = attachments_props();
    void *blob = call_preview("Macintosh HD:tmp:ql-ownership.html", NULL);
    int ok = blob_is(blob, g_ql_data);
    eng_free_handle(blob);
    return eng_no_leaks() && ok;
}

/* ---------- controls (pass on both trees) ---------- */

static int control_cmd_preview_no_data(void) {
    g_ql_data = nil;
    g_ql_props = attachments_props();
    void *o = eng_new_object(0);
    void *blob = call_preview("Macintosh HD:tmp:ql-ownership.html", o);
    int ok = blob_is(blob, [NSData data]);
    ok &= check(eng_prop_count(o) == 0, "object untouched");
    ok &= check(g_fake_live == 0, "QLPreview released");
    eng_release(o);
    eng_free_handle(blob);
    return eng_no_leaks() && ok;
}

static int control_cmd_preview_no_properties(void) {
    g_ql_data = bytes("<html/>");
    g_ql_props = nil;
    void *blob = call_preview("Macintosh HD:tmp:ql-ownership.html", NULL);
    int ok = blob_is(blob, g_ql_data);
    eng_free_handle(blob);
    return eng_no_leaks() && ok;
}

static int control_ob_set_n_b_c_round_trip(void) {
    void *o = eng_new_object(0);
    ob_set_n((PA_ObjectRef)o, @"n", @2.5);
    ob_set_b((PA_ObjectRef)o, @"b", @YES);
    PA_CollectionRef c = PA_CreateCollection();
    PA_ObjectRef item = PA_CreateObject();
    collection_push(c, item); /* consumes item */
    ob_set_c((PA_ObjectRef)o, @"c", c); /* consumes c */
    void *stored = eng_prop_ref(o, "c");
    int ok = check(eng_prop_count(o) == 3, "three properties set");
    ok &= check(eng_prop_real(o, "n") == 2.5 && eng_prop_bool(o, "b") == 1, "values stored");
    ok &= check(stored == (void *)c && eng_item_count(stored) == 1, "collection stored with its item");
    eng_release(o);
    return eng_no_leaks() && ok;
}

typedef struct { const char *name; int (*fn)(void); } Test;
static const Test kTests[] = {
    { "defect_cmd_preview_metadata_double_dispose", defect_cmd_preview_metadata_double_dispose },
    { "defect_ob_set_s_double_dispose", defect_ob_set_s_double_dispose },
    { "defect_ob_set_s_empty_owner_leak", defect_ob_set_s_empty_owner_leak },
    { "defect_ob_set_i_empty_picture_leak", defect_ob_set_i_empty_picture_leak },
    { "defect_cmd_preview_attachments", defect_cmd_preview_attachments },
    { "defect_cmd_preview_no_attachments_collection_leak", defect_cmd_preview_no_attachments_collection_leak },
    { "defect_cmd_preview_null_object_leak", defect_cmd_preview_null_object_leak },
    { "control_cmd_preview_no_data", control_cmd_preview_no_data },
    { "control_cmd_preview_no_properties", control_cmd_preview_no_properties },
    { "control_ob_set_n_b_c_round_trip", control_ob_set_n_b_c_round_trip },
};

int main(int argc, char **argv) {
    eng_install();
    install_fake_quicklook();
    size_t n = sizeof(kTests) / sizeof(kTests[0]);
    if(argc == 2 && !strcmp(argv[1], "--list")) {
        for(size_t i = 0; i < n; i++) printf("%s\n", kTests[i].name);
        return 0;
    }
    if(argc != 2) { fprintf(stderr, "usage: %s --list | <test>\n", argv[0]); return 2; }
    for(size_t i = 0; i < n; i++) {
        if(!strcmp(argv[1], kTests[i].name)) {
            int ok;
            @autoreleasepool { ok = kTests[i].fn(); }
            printf("%s: %s\n", kTests[i].name, ok ? "PASS" : "FAIL");
            return ok ? 0 : 1;
        }
    }
    fprintf(stderr, "unknown test %s\n", argv[1]);
    return 2;
}
