/*
 * Synthetic 4D engine boundary for ownership regressions.
 *
 * Replaces only gCall4D; the bundled, unmodified 4DPluginAPI.c is linked as
 * is. Every engine resource (Unistrings, pictures, objects/collections,
 * handles) is a real heap block tracked in a live set, so leaks are detected
 * exactly and a dispose of something the engine no longer owns (double free)
 * is reported by name instead of being freed a second time.
 *
 * Engine semantics modelled (per the SDK contract):
 *  - PA_CreateVariable(eVK_Unistring/eVK_Picture) allocates an empty value.
 *  - PA_SetStringVariable/PA_SetPictureVariable are shallow (SDK code).
 *  - PA_SetObjectProperty/PA_SetCollectionElement deep-copy strings and
 *    pictures and retain objects/collections; the caller still owns its
 *    variable.
 *  - PA_ClearVariable disposes the value the variable holds.
 *  - PA_ExecuteCommandByID returns an owned result (New Object/Collection).
 *  - PA_ReturnBlob/PA_ReturnPicture hand the result to the engine (the test
 *    plays the engine and frees it).
 */

#include "4DPluginAPI.h"
#include "PrivateTypes.h"
#include "EntryPoints.h"
#include "engine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

extern "C" Call4DProcPtr gCall4D;

namespace {

typedef std::basic_string<PA_Unichar> u16;

struct Obj {
    int rc;
    bool coll;
    std::map<u16, PA_Variable> props;
    std::vector<PA_Variable> items;
};

struct Handle { char *data; size_t size; };

std::set<void *> g_strings;
std::set<void *> g_pictures;
std::set<Obj *> g_objects;
std::set<void *> g_handles;
std::vector<std::string> g_violations;

void violation(const std::string &what) {
    fprintf(stderr, "  ENGINE CONTRACT: %s\n", what.c_str());
    g_violations.push_back(what);
}

[[noreturn]] void die(const char *msg) {
    fprintf(stderr, "HARNESS ERROR: %s\n", msg);
    fflush(stderr);
    abort();
}

std::string narrow(const u16 &s) { std::string r; for(PA_Unichar c : s) r.push_back((char)c); return r; }
u16 widen(const std::string &s) { u16 r; for(char c : s) r.push_back((PA_Unichar)(unsigned char)c); return r; }

PA_Unistring new_string(const PA_Unichar *src, size_t len) {
    PA_Unichar *buf = (PA_Unichar *)malloc((len + 1) * sizeof(PA_Unichar));
    if(len) memcpy(buf, src, len * sizeof(PA_Unichar));
    buf[len] = 0;
    g_strings.insert(buf);
    PA_Unistring s;
    s.fLength = (PA_long32)len;
    s.fString = buf;
    s.fReserved1 = 0;
    s.fReserved2 = 0;
    return s;
}

void free_string(PA_Unistring *s) {
    if(!s->fString) return;
    if(!g_strings.erase(s->fString)) violation("double_dispose_string");
    else free(s->fString);
    s->fString = 0;
    s->fLength = 0;
}

void *new_picture() {
    void *p = malloc(16);
    g_pictures.insert(p);
    return p;
}

void free_picture(void *p) {
    if(!p) return;
    if(!g_pictures.erase(p)) { violation("double_dispose_picture"); return; }
    free(p);
}

Handle *new_handle(size_t size) {
    Handle *h = (Handle *)malloc(sizeof(Handle));
    h->size = size;
    h->data = (char *)malloc(size ? size : 1);
    g_handles.insert(h);
    return h;
}

void free_handle(Handle *h) {
    if(!h) return;
    if(!g_handles.erase(h)) { violation("double_dispose_handle"); return; }
    free(h->data);
    free(h);
}

Handle *live_handle(void *p) {
    if(!g_handles.count(p)) die("used an unknown handle");
    return (Handle *)p;
}

void release_object(Obj *o);

void clear_var(PA_Variable *v) {
    switch(v->fType) {
        case eVK_Unistring: free_string(&v->uValue.fString); break;
        case eVK_Picture: free_picture(v->uValue.fPicture); break;
        case eVK_Blob: free_handle((Handle *)v->uValue.fBlob.fHandle); break;
        case eVK_Object: if(v->uValue.fObject) release_object((Obj *)v->uValue.fObject); break;
        case eVK_Collection: if(v->uValue.fCollection) release_object((Obj *)v->uValue.fCollection); break;
        default: break;
    }
    v->fType = eVK_Undefined;
}

Obj *live(void *p) {
    if(!g_objects.count((Obj *)p)) die("used a dead object/collection");
    return (Obj *)p;
}

PA_Variable undefined_var() { PA_Variable v; memset(&v, 0, sizeof(v)); v.fType = eVK_Undefined; return v; }

PA_Variable copy_var(const PA_Variable &src) {
    PA_Variable dst = src;
    switch(src.fType) {
        case eVK_Unistring:
        {
            /* PA_Variable is packed: never bind references to its members. */
            void *str = src.uValue.fString.fString;
            if(!g_strings.count(str)) die("stored a dead string");
            dst.uValue.fString = new_string(src.uValue.fString.fString, src.uValue.fString.fLength);
        }
            break;
        case eVK_Picture:
        {
            void *pic = src.uValue.fPicture;
            if(!g_pictures.count(pic)) die("stored a dead picture");
        }
            dst.uValue.fPicture = new_picture();
            break;
        case eVK_Object:
            if(src.uValue.fObject) live(src.uValue.fObject)->rc++;
            break;
        case eVK_Collection:
            if(src.uValue.fCollection) live(src.uValue.fCollection)->rc++;
            break;
        default: break;
    }
    return dst;
}

Obj *new_object(bool coll) {
    Obj *o = new Obj();
    o->rc = 1;
    o->coll = coll;
    g_objects.insert(o);
    return o;
}

void release_object(Obj *o) {
    if(!g_objects.count(o)) { violation("double_release_object"); return; }
    if(--o->rc == 0) {
        for(auto &kv : o->props) clear_var(&kv.second);
        for(auto &v : o->items) clear_var(&v);
        g_objects.erase(o);
        delete o;
    }
}

u16 key_of(const PA_Unistring &k) {
    void *str = k.fString;
    if(!g_strings.count(str)) die("used a dead key string");
    return u16(k.fString, k.fLength);
}

PA_Variable object_result(Obj *o) {
    PA_Variable v = undefined_var();
    v.fType = o->coll ? eVK_Collection : eVK_Object;
    if(o->coll) v.uValue.fCollection = (PA_CollectionRef)o; else v.uValue.fObject = (PA_ObjectRef)o;
    return v;
}

void execute_command(EngineBlock *eb) {
    PA_Variable *ret = (PA_Variable *)eb->fPtr2;
    switch(eb->fParam1) {
        case 1471: *ret = object_result(new_object(false)); break; /* New Object */
        case 1472: *ret = object_result(new_object(true)); break;  /* New Collection */
        default: {
            char msg[64];
            snprintf(msg, sizeof(msg), "unexpected PA_ExecuteCommandByID %d", (int)eb->fParam1);
            die(msg);
        }
    }
}

void engine(short selector, EngineBlock *eb) {
    switch(selector) {
        case EX_GET_INFORMATION:
            eb->fParam2 = 0x00002000; /* v20 */
            break;
        case EX_CREATE_UNISTRING: {
            const PA_Unichar *src = (const PA_Unichar *)eb->fHandle;
            size_t len = 0;
            while(src[len]) len++;
            eb->fUniString1 = new_string(src, len);
            break;
        }
        case EX_DISPOSE_UNISTRING:
            free_string(&eb->fUniString1);
            break;
        case EX_CREATE_PICTURE:
            eb->fPicture = new_picture();
            break;
        case EX_DISPOSE_PICTURE:
            free_picture(eb->fPicture);
            break;
        case EX_CLEAR_VARIABLE:
            clear_var((PA_Variable *)eb->fPtr1);
            break;
        case EX_CONVERT_STRING: { /* FromUserData (PA_ReturnBlob) */
            if(eb->fParam1 != 5) die("unexpected EX_CONVERT_STRING mode");
            Handle *h = new_handle((size_t)eb->fParam4);
            if(eb->fParam4) memcpy(h->data, (const void *)eb->fParam3, (size_t)eb->fParam4);
            eb->fHandle = (PA_Handle)h;
            break;
        }
        case EX_SET_OBJ_VALUE: {
            Obj *o = live(eb->fPtr1);
            PA_Variable value = copy_var(*(PA_Variable *)eb->fPtr2);
            if(eb->fShort == eVK_Collection) {
                if(!o->coll || eb->fLongint < 0) die("bad collection set");
                size_t i = (size_t)eb->fLongint;
                if(i >= o->items.size()) o->items.resize(i + 1, undefined_var());
                clear_var(&o->items[i]);
                o->items[i] = value;
            } else {
                if(o->coll) die("property set on a collection");
                u16 k = key_of(eb->fUniString1);
                auto it = o->props.find(k);
                if(it != o->props.end()) clear_var(&it->second);
                o->props[k] = value;
            }
            break;
        }
        case EX_GET_OBJ_VALUE: {
            Obj *o = live(eb->fPtr1);
            PA_Variable *out = (PA_Variable *)eb->fPtr2;
            *out = undefined_var();
            if(eb->fShort == eVK_Collection && eb->fLongint == -1) {
                if(!o->coll) die("length of an object");
                out->fType = eVK_Longint;
                out->uValue.fLongint = (PA_long32)o->items.size();
                break;
            }
            die("unmodelled EX_GET_OBJ_VALUE");
        }
        case EX_EXECUTE_COMMAND_BY_ID:
            execute_command(eb);
            break;
        case EX_HANDLE_MANAGER: {
            switch(eb->fParam1) {
                case 1: eb->fHandle = (PA_Handle)new_handle((size_t)eb->fParam2); break;
                case 2: free_handle((Handle *)eb->fHandle); break;
                case 5: eb->fParam3 = (sLONG_PTR)live_handle(eb->fHandle)->data; break;
                case 6: break;
                default: die("unexpected handle manager op");
            }
            break;
        }
        default: {
            char msg[64];
            snprintf(msg, sizeof(msg), "unmodelled engine selector %d", selector);
            die(msg);
        }
    }
}

const PA_Variable *find_prop(void *o, const char *key) {
    Obj *ob = live(o);
    auto it = ob->props.find(widen(key));
    return it == ob->props.end() ? nullptr : &it->second;
}

} // namespace

extern "C" {

void eng_install(void) { gCall4D = engine; }

void *eng_new_object(int coll) { return new_object(coll != 0); }

void eng_release(void *o) { release_object((Obj *)o); }

PA_Unistring eng_U(const char *ascii) {
    u16 s = widen(ascii);
    return new_string(s.c_str(), s.size());
}

void eng_dispose_string(PA_Unistring *s) { free_string(s); }

int eng_no_leaks(void) {
    int ok = 1;
    for(void *p : g_strings) {
        fprintf(stderr, "  LEAK: engine string \"%s\"\n", narrow(u16((PA_Unichar *)p)).c_str());
        ok = 0;
    }
    if(!g_pictures.empty()) { fprintf(stderr, "  LEAK: %zu engine picture(s)\n", g_pictures.size()); ok = 0; }
    if(!g_objects.empty()) { fprintf(stderr, "  LEAK: %zu engine object(s)/collection(s)\n", g_objects.size()); ok = 0; }
    if(!g_handles.empty()) { fprintf(stderr, "  LEAK: %zu engine handle(s)\n", g_handles.size()); ok = 0; }
    if(!g_violations.empty()) ok = 0;
    return ok;
}

int eng_violations(const char *what) {
    int n = 0;
    for(auto &v : g_violations) if(v == what) n++;
    return n;
}

int eng_string_left(const char *ascii) {
    for(void *p : g_strings) if(narrow(u16((PA_Unichar *)p)) == ascii) return 1;
    return 0;
}

size_t eng_picture_count(void) { return g_pictures.size(); }
size_t eng_object_count(void) { return g_objects.size(); }

size_t eng_prop_count(void *o) { return live(o)->props.size(); }

int eng_prop_type(void *o, const char *key) {
    const PA_Variable *v = find_prop(o, key);
    return v ? v->fType : -1;
}

int eng_prop_text_is(void *o, const char *key, const char *ascii) {
    const PA_Variable *v = find_prop(o, key);
    if(!v || v->fType != eVK_Unistring) return 0;
    void *str = v->uValue.fString.fString;
    if(!g_strings.count(str)) return 0;
    return narrow(u16(v->uValue.fString.fString, v->uValue.fString.fLength)) == ascii;
}

double eng_prop_real(void *o, const char *key) {
    const PA_Variable *v = find_prop(o, key);
    return v && v->fType == eVK_Real ? v->uValue.fReal : -1.0;
}

int eng_prop_bool(void *o, const char *key) {
    const PA_Variable *v = find_prop(o, key);
    return v && v->fType == eVK_Boolean ? v->uValue.fBoolean : -1;
}

int eng_prop_live_picture(void *o, const char *key) {
    const PA_Variable *v = find_prop(o, key);
    if(!v || v->fType != eVK_Picture) return 0;
    void *pic = v->uValue.fPicture;
    return g_pictures.count(pic) ? 1 : 0;
}

void *eng_prop_ref(void *o, const char *key) {
    const PA_Variable *v = find_prop(o, key);
    if(!v) return nullptr;
    if(v->fType == eVK_Object) return v->uValue.fObject;
    if(v->fType == eVK_Collection) return v->uValue.fCollection;
    return nullptr;
}

size_t eng_item_count(void *c) { return live(c)->items.size(); }

void *eng_item_ref(void *c, size_t i) {
    Obj *o = live(c);
    if(i >= o->items.size() || o->items[i].fType != eVK_Object) return nullptr;
    return o->items[i].uValue.fObject;
}

int eng_live_picture(void *p) { return g_pictures.count(p) ? 1 : 0; }
void eng_free_picture(void *p) { free_picture(p); }

long eng_handle_size(void *h) { return g_handles.count(h) ? (long)((Handle *)h)->size : -1; }
const char *eng_handle_data(void *h) { return g_handles.count(h) ? ((Handle *)h)->data : nullptr; }
void eng_free_handle(void *h) { free_handle((Handle *)h); }

}
