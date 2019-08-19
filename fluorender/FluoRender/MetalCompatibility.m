//
//  MetalCompatibility.m
//  wxVulkanTex
//
//  Created by takashi kawase on 5/9/19.
//
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

void makeViewMetalCompatible(void* handle)
{
    NSView* view = (NSView*)handle;
    assert([view isKindOfClass:[NSView class]]);
    
    if (![view.layer isKindOfClass:[CAMetalLayer class]])
    {
        [view setLayer:[CAMetalLayer layer]];
        [view setWantsLayer:YES];
    }
}
