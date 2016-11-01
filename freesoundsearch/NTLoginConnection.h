//
//  NTLoginConnection.h
//  freesound_search
//
//  Created by Nao Tokui on 11/21/06.
//  Copyright 2006 www.sonasphere.com. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class NTFreesoundSearch;

typedef enum _LoginProcess { login_idle, loggingin, loggingout } LoginProcess;

@interface NTLoginConnection : NSObject {
	NTFreesoundSearch	*_delegate;				// main  
		
	NSURLConnection		*theConnection;			// NSURLConnection object for downloading html file
	NSMutableData		*receivedData;			// Received html data
	LoginProcess		currentStatus;			// logging in or logging out or idle
}

- (void) setDelegate: (NTFreesoundSearch *) obj;
- (id) delegate;

- (int) startLogin: (const char *) username : (const char *) password;
- (int) startLogout;

- (void) cancel;

@end
