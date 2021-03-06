#include <stdlib.h>
#include <objc/runtime.h>
#include "lib/substitute.h"

extern void *SubGetImageByName(const char *filename) __asm__("SubGetImageByName");;
const void *MSGetImageByName(const char *filename) {
    struct substitute_image *im = SubGetImageByName(filename);
    const void *mh = NULL;
    if (im) {
        mh = im->image_header;
        substitute_close_image(im);
    }
    return mh;
}

extern intptr_t _dyld_get_image_slide(const void *);
extern void *SubFindSymbol(void *image, const char *name) __asm__("SubFindSymbol");
const void *MSFindSymbol(void *image, const char *name) {
    if (!image) return SubFindSymbol(NULL, name);
    struct substitute_image im;
    im.image_header = image;
    im.slide = _dyld_get_image_slide(image);
    return SubFindSymbol(&im, name);
}

extern void SubHookFunction(void *symbol, void *replace, void **result) __asm__("SubHookFunction");
void MSHookFunction(void *symbol, void *replace, void **result) {
    SubHookFunction(symbol, replace, result);
}

extern void SubHookMessageEx(Class _class, SEL sel, IMP imp, IMP *result) __asm__("SubHookMessageEx");
void MSHookMessageEx(Class _class, SEL sel, IMP imp, IMP *result) {
    SubHookMessageEx(_class, sel, imp, result);
}

extern void SubHookMemory(void *target, const void *data, size_t size) __asm__("SubHookMemory");
void MSHookMemory(void *target, const void *data, size_t size) {
    SubHookMemory(target, data, size);
}

// i don't think anyone uses this function anymore, but it's here for completeness
void MSHookClassPair(Class _class, Class hook, Class old) {
    unsigned int n_methods = 0;
    Method *hooks = class_copyMethodList(hook, &n_methods);

    for (unsigned int i = 0; i < n_methods; ++i) {
        SEL selector = method_getName(hooks[i]);
        const char *what = method_getTypeEncoding(hooks[i]);

        Method old_mptr = class_getInstanceMethod(old, selector);
        Method cls_mptr = class_getInstanceMethod(_class, selector);

        if (cls_mptr) {
            class_addMethod(old, selector, method_getImplementation(hooks[i]), what);
            method_exchangeImplementations(cls_mptr, old_mptr);
        } else {
            class_addMethod(_class, selector, method_getImplementation(hooks[i]), what);
        }
    }

    free(hooks);
}
