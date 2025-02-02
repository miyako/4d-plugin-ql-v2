/* --------------------------------------------------------------------------------
 #
 #  4DPlugin-QL.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : QL
 #	author : miyako
 #	2021/01/11
 #  
 # --------------------------------------------------------------------------------*/

#include "4DPlugin-QL.h"

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params) {
    
//	try
//	{
        switch(selector)
        {
			// --- QL
            
			case 1 :
				QL_Create_thumbnail(params);
				break;
			case 2 :
				QL_Create_preview(params);
				break;

        }

//	}
//	catch(...)
//	{
//
//	}
}

#pragma mark -

CFURLRef copyPathURL(PA_Unistring *str) {
    
    if(str)
    {
        CFStringRef path = CFStringCreateWithCharacters(kCFAllocatorDefault, (const UniChar *)str->fString, (CFIndex)str->fLength);
        if(path)
        {
            return CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path, kCFURLHFSPathStyle, false);
        }
    }
    
    return nil;
}

#pragma mark -

void QL_Create_thumbnail(PA_PluginParameters params) {

    bool didReturn = false;
    
    CFURLRef url = copyPathURL(PA_GetStringParameter(params, 1));
    
    if(url) {
        CGSize maxSize = CGSizeMake(1024, 1024);
        CFMutableDictionaryRef options = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                                   &kCFTypeDictionaryKeyCallBacks,
                                                                   &kCFTypeDictionaryValueCallBacks);
        if(options) {
            CGImageRef image = QLThumbnailImageCreate(kCFAllocatorDefault, url, maxSize, options);
            if(image) {
                CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault, 0);
                CGImageDestinationRef destination = CGImageDestinationCreateWithData(data, kUTTypeTIFF, 1, NULL);
                CFMutableDictionaryRef properties = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
                CGImageDestinationAddImage(destination, image, properties);
                CGImageDestinationFinalize(destination);
                PA_ReturnPicture(params,
                                 PA_CreatePicture((void *)CFDataGetBytePtr(data),
                                                  (PA_long32)CFDataGetLength(data)));
                didReturn = true;
                CFRelease(properties);
                CFRelease(destination);
                CFRelease(data);
                CGImageRelease(image);
            }else{
                NSString *path = (NSString *)CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
                if(path) {
                    @autoreleasepool {
                        NSImage *image = [[NSWorkspace sharedWorkspace]iconForFile:path];
                        [image setSize:NSMakeSize(maxSize.width, maxSize.height)];
                        NSData *data = [image TIFFRepresentation];
                        PA_ReturnPicture(params, PA_CreatePicture((void *)[data bytes],
                                                                  (PA_long32)[data length]));
                        [path release];
                    }
                    didReturn = true;
                    
                }
            }
            CFRelease(options);
        }
        CFRelease(url);
    }
    if(!didReturn) {
        PA_ReturnPicture(params, PA_CreatePicture((void *)"", 0));
    }
}

typedef enum {
    
    api_version_catalina = 0,
    api_version_big_sur = 1,
    api_version_before_catalina = 2
    
} api_version_t;

static api_version_t get_api_version() {
    
    //https://developer.apple.com/forums/thread/660346
    
    api_version_t v = api_version_catalina;
    
    if (@available(macOS 10.16, *)) {
        v = api_version_big_sur;
    }else
        
        if (@available(macOS 11.0, *)) {
            v = api_version_big_sur;
        }else
            
            if (@available(macOS 10.15, *)) {
                v = api_version_catalina;
            }
    
    return v;
}

static PA_Unistring createUnistring(NSString *str) {
        
    if(str) {
        return PA_CreateUnistring((PA_Unichar *)[str cStringUsingEncoding:NSUTF16LittleEndianStringEncoding]);
    }
    
    return PA_CreateUnistring((PA_Unichar *)"\0\0");
}

static void ob_set_s(PA_ObjectRef obj, NSString *_key, NSString *_value) {
    
    if(obj) {
        
        PA_Unistring key = createUnistring(_key);
        PA_Unistring value = createUnistring(_value);
        PA_Variable v = PA_CreateVariable(eVK_Unistring);
        PA_SetStringVariable(&v, &value);
        PA_SetObjectProperty(obj, &key, v);
        PA_DisposeUnistring(&key);
        PA_ClearVariable(&v);
        
    }
    
}

static void collection_push(PA_CollectionRef c, PA_ObjectRef value) {

    if (value) {
        PA_Variable v = PA_CreateVariable(eVK_Object);
        PA_SetObjectVariable(&v, value);
        PA_SetCollectionElement(c, PA_GetCollectionLength(c), v);
        PA_ClearVariable(&v);
    }
}

static void ob_set_p(PA_ObjectRef obj, NSString *_key, NSString *_value) {
    
    if((obj) && (_value)) {
        
        CFURLRef u = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)_value,
                                                   kCFURLPOSIXPathStyle,
                                                   false);
        if(u) {
            CFStringRef s = CFURLCopyFileSystemPath((CFURLRef)u, kCFURLHFSPathStyle);
            if(s)
            {
                ob_set_s(obj, _key, (NSString *)s);
                CFRelease(s);
            }
            CFRelease(u);
        }

    }
    
}

static void ob_set_n(PA_ObjectRef obj, NSString *_key, NSNumber *_value) {
    
    if((obj) && (_value)) {
        
        PA_Unistring key = createUnistring(_key);
        double value = [_value doubleValue];
        PA_Variable v = PA_CreateVariable(eVK_Real);
        PA_SetRealVariable(&v, value);
        PA_SetObjectProperty(obj, &key, v);
        PA_DisposeUnistring(&key);
        PA_ClearVariable(&v);
        
    }
    
}

static void ob_set_b(PA_ObjectRef obj, NSString *_key, NSNumber *_value) {
    
    if((obj) && (_value)) {
                
        PA_Unistring key = createUnistring(_key);
        bool value = [_value boolValue];
        PA_Variable v = PA_CreateVariable(eVK_Boolean);
        PA_SetBooleanVariable(&v, value);
        PA_SetObjectProperty(obj, &key, v);
        PA_DisposeUnistring(&key);
        PA_ClearVariable(&v);

    }
    
}

void ob_set_c(PA_ObjectRef obj, NSString *_key, PA_CollectionRef value) {
    
    if(obj)
    {
        if(value)
        {
            PA_Unistring key = createUnistring(_key);
            PA_Variable v = PA_CreateVariable(eVK_Collection);
            PA_SetCollectionVariable(&v, value);
            PA_SetObjectProperty(obj, &key, v);
            PA_DisposeUnistring(&key);
            PA_ClearVariable(&v);
        }
    }
}

void ob_set_i(PA_ObjectRef obj, NSString *_key, PA_Picture value) {
    
    if(obj)
    {
        if(value)
        {
            PA_Unistring key = createUnistring(_key);
            PA_Variable v = PA_CreateVariable(eVK_Picture);
            PA_SetPictureVariable(&v, value);
            PA_SetObjectProperty(obj, &key, v);
            PA_DisposeUnistring(&key);
            PA_ClearVariable(&v);
        }
    }
}

static void ql_get_properties(CFDictionaryRef properties, PA_ObjectRef arg2) {
    
    if(properties) {
        
        PA_CollectionRef colAttachments = PA_CreateCollection();
        
        if(CFDictionaryContainsKey(properties, kQLPreviewPropertyAttachmentsKey)) {
            CFDictionaryRef attachments = (CFDictionaryRef)CFDictionaryGetValue(properties, kQLPreviewPropertyAttachmentsKey);
            [(NSDictionary *)attachments enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
                
                NSString *cid = (NSString *)key;
                CFDictionaryRef attachment = (CFDictionaryRef)obj;
                
                PA_ObjectRef objAttachment = PA_CreateObject();
                ob_set_s(objAttachment, @"cid", cid);
                
                NSString *encoding = (NSString *)CFDictionaryGetValue(attachment, kQLPreviewPropertyTextEncodingNameKey);
                ob_set_s(objAttachment, @"encoding", encoding);
                
                NSString *type = (NSString *)CFDictionaryGetValue(attachment, kQLPreviewPropertyMIMETypeKey);
                ob_set_s(objAttachment, @"type", type);
                
                NSData *data = (NSData *)CFDictionaryGetValue(attachment, kQLPreviewPropertyAttachmentDataKey);
                
                if(data) {
                    if(([type hasPrefix:@"text/"])
                       || ([type hasSuffix:@"/javascript"])
                       || ([cid hasSuffix:@".css"])
                       || ([cid hasSuffix:@".js"])
                       || ([cid hasSuffix:@".html"])){
                        
                        NSStringEncoding ie = CFStringConvertEncodingToNSStringEncoding(CFStringConvertIANACharSetNameToEncoding((CFStringRef)encoding));
                        NSString *text = [[NSString alloc]initWithData:data encoding:ie];
                        if(text) {
                            ob_set_s(objAttachment, @"data", text);
                            [text release];
                        }
                    }else{
                        PA_Picture p = PA_CreatePicture((void *)[data bytes], (PA_long32)[data length]);
                        ob_set_i(objAttachment, @"data", p);
                    }
                }
                collection_push(colAttachments, objAttachment);

            }];
            ob_set_c(arg2, @"Attachments", colAttachments);
        }
        
        __block NSArray *stringKeys = @[/*@"PDFStyle",*/
                                        @"TextEncoding",
                                        @"PageElementXPath",
                                        @"MimeType"];
        __block NSArray *pathKeys = @[@"BaseBundlePath"];
        __block NSArray *numberKeys = @[@"Height", @"Width", @"PDFStyle"];
        __block NSArray *boolKeys = @[@"AllowJavascript", @"AllowNetworkAccess"];
        
        [(NSDictionary *)properties enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
            
            NSString *_key = (NSString *)key;
            NSString *_stringValue = (NSString *)obj;
            NSNumber *_numberValue = (NSNumber *)obj;
            
            if([stringKeys indexOfObject:_key] != NSNotFound) {
                ob_set_s(arg2, _key, _stringValue);
                return;
            }
            
            if([pathKeys indexOfObject:_key] != NSNotFound) {
                ob_set_p(arg2, _key, _stringValue);
                return;
            }
            
            if([numberKeys indexOfObject:_key] != NSNotFound) {
                ob_set_n(arg2, _key, _numberValue);
                return;
            }
            
            if([boolKeys indexOfObject:_key] != NSNotFound) {
                ob_set_b(arg2, _key, _numberValue);
                return;
            }
            
        }];
        
    }
    
}

void QL_Create_preview(PA_PluginParameters params) {

    bool didReturn = false;
    
    CFURLRef url = copyPathURL(PA_GetStringParameter(params, 1));
    
    PA_ObjectRef arg2 = PA_GetObjectParameter(params, 2);
    
    if(url) {
        CFMutableDictionaryRef options = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                                   &kCFTypeDictionaryKeyCallBacks,
                                                                   &kCFTypeDictionaryValueCallBacks);
        if(options) {
            
            switch (get_api_version()) {
                case api_version_catalina:
                {
                    /*
                    QLPreviewRef preview = QLPreviewCreate(kCFAllocatorDefault, url, options);
                    if(preview) {
                        QLPreview *qlpreview = [[QLPreview alloc] initWithQLPreviewRef:preview];
                        if(qlpreview) {
                            NSData *data = [qlpreview synchronousGetData];
                            if(data) {
                                
                                if(arg2) {
                     
                     ob_set_s(arg2, @"DisplayBundleID", (NSString *)QLPreviewGetDisplayBundleID(preview));
                     ob_set_s(arg2, @"PreviewContentType", (NSString *)QLPreviewCopyPreviewContentType(preview));

                     CFDictionaryRef properties = QLPreviewCopyProperties(preview);
                     ql_get_properties(properties, arg2);
                     CFRelease(properties);
                                }
                                
                                PA_ReturnBlob(params, (void *)[data bytes], (PA_long32)[data length]);
                                didReturn = true;
                            }
                            [qlpreview release];
                        }
                        QLPreviewClose(preview);
                    }
                     */
                }
                    break;
                    
                case api_version_big_sur:
                {
                    QLPreview *qlpreview = [[QLPreview alloc]initWithURL:(NSURL *)url options:(NSDictionary *)options];
                    if(qlpreview) {
                        NSData *data = [qlpreview synchronousGetData];
                        if(data) {
                                
                            if(arg2) {
                                
                                NSString *displayBundleID = [qlpreview displayBundleID];
                                NSString *previewContentType = [qlpreview previewContentType];
                                
                                ob_set_s(arg2, @"DisplayBundleID", displayBundleID);
                                ob_set_s(arg2, @"PreviewContentType", (NSString *)previewContentType);
                            }

                            NSDictionary *properties = [qlpreview getProperties];
                            ql_get_properties((CFDictionaryRef)properties, arg2);
                            
                            PA_ReturnBlob(params, (void *)[data bytes], (PA_long32)[data length]);
                            didReturn = true;
                        }
                        [qlpreview release];
                    }
                }
                    break;
                    
                default:
                {
                    /*
                  QLPreviewRef preview = QLPreviewCreate(kCFAllocatorDefault, url, options);
                  if(preview) {
                      CFDataRef data = QLPreviewCopyData(preview);
                      if(data) {
                  
                          if(arg2) {

                              ql_get_properties(preview, arg2);
                          }
                     
                          PA_ReturnBlob(params, (void *)CFDataGetBytePtr(data), (PA_long32)CFDataGetLength(data));
                          didReturn = true;
                      }
                  }
                  */
                }
                    break;
            }
            CFRelease(options);
            }
        CFRelease(url);
    }
        
    if(!didReturn) {
        PA_ReturnBlob(params, (void *)"", 0);
    }
}
