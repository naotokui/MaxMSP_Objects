//
//  NTGeneralURLAccess.h
//  freesound_search
//
//  Created by Nao Tokui on 11/21/06.
//  Copyright 2006 www.sonasphere.com. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class NTFreesoundSearch;

typedef enum _accessProcess { access_idle, accessing } accessProcess;

@interface NTGeneralURLAccess : NSObject {
	NTFreesoundSearch*		_delegate;
	SEL						_function;				// delegate's function, which will be called when done 
	
	accessProcess			currentStatus;
	NSURLConnection			*theConnection;
	NSMutableData			*receivedData;			// Received html data
	
	NSString				*filepath;
}

- (void) setDelegate: (NTFreesoundSearch *) obj;
- (void) setDelegateFunction: (SEL) selector;
- (id) delegate;
- (void) setFilepath: (NSString *) path;
- (NSString *) filepath;

- (int) startURLAccess: (const NSURL *) urlObj: (const NSString *) postStr;
- (void) cancel;

@end
