//
//  NTLoginConnection.m
//  NTFreesoundSearch
//
//  Created by Nao Tokui on 11/21/06.
//  Copyright 2006 www.sonasphere.com. All rights reserved.
//

#import "NTLoginConnection.h"
#import "NTFreesoundSearch.h"
#import "AGRegex.h"
//#import <AGRegex/AGRegex.h>

#include "ext.h"

// Login/Logout URL 
#define	LOGIN_TARGET_URL	@"http://www.freesound.org/forum/login.php"
#define	LOGOUT_TARGET_URL	@"http://www.freesound.org/index.php?logout=true"

// Time Out
#define TIMEOUT_DURATION	30

@implementation NTLoginConnection

#pragma mark -

/*** Delegate Functions ***/
- (void) setDelegate: (NTFreesoundSearch *) obj
{
	_delegate = obj;
}
- (id) delegate{ return _delegate; }

- (void) dealloc {	
	[super dealloc];
}


#pragma mark -

/*** Login/Logout Procedure ***/
- (int) startLogin: (const char *) login : (const char *) password	
{
	// Check if there is login request still working
	if (currentStatus == loggingin){
		error("login request still in process");
		return 0;
	}
	
	// Make login & password strings
	NSString *loginStr	= [NSString stringWithCString: login];
	NSString *pwStr		= [NSString stringWithCString: password];
	// Warn and return 0 if these strings are blank
	if ([[loginStr stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]] isEqualToString:@""] 
			|| [[pwStr stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]] isEqualToString:@""]){
			error("Username/Password strings are blank. Please specify your Username and Password.");
			return 0;			
	}		
		
	// Create a request for the login procedure for registered users
	NSMutableURLRequest *loginRequest=[[NSMutableURLRequest alloc] initWithURL:[NSURL URLWithString: LOGIN_TARGET_URL] 
					cachePolicy: NSURLRequestReloadIgnoringCacheData  // No Cache. Always tries to load the original source.
					timeoutInterval: TIMEOUT_DURATION];
	NSAssert(loginRequest, @"Cannot make a URL request");				
								
	// Configure the request 
	[loginRequest setHTTPShouldHandleCookies: YES];
	[loginRequest setHTTPMethod:@"POST"];
	NSString *postStr = [NSString stringWithFormat:
		@"username=%@&password=%@&login=login&redirect=../index.php", loginStr, pwStr];					// POST Data
	[loginRequest setHTTPBody:[postStr dataUsingEncoding: NSISOLatin1StringEncoding]];
		
	// Create the connection with the request and start loading the data
	theConnection=[[NSURLConnection alloc] initWithRequest:loginRequest delegate:self];
	[loginRequest release];
	if (theConnection) {
		// Create the NSMutableData that will hold the received data
		receivedData=[[NSMutableData data] retain];
		currentStatus = loggingin;
		return 1;																						// Starting...
	} else {
		// inform the user that the download could not be made
		NSLog(@"Cannot Make A Connection to: %@", LOGIN_TARGET_URL);
		return 0;
	}
	return 0;
}

- (int) startLogout
{
	// Check if there is login request still working
	if (currentStatus == loggingout){
		error("logout request still in process");
		return 0;
	}
			
	// Create a request for the login procedure for registered users
	NSMutableURLRequest *logoutRequest=[[NSMutableURLRequest alloc] initWithURL:[NSURL URLWithString: LOGOUT_TARGET_URL] 
					cachePolicy: NSURLRequestReloadIgnoringCacheData									// DO CACHE!
					timeoutInterval: TIMEOUT_DURATION];
	NSAssert(logoutRequest, @"Cannot make a URL request");				
		
	// Create the connection with the request and start loading the data
	theConnection=[[NSURLConnection alloc] initWithRequest:logoutRequest delegate:self];
	[logoutRequest release];
	if (theConnection) {
		// Create the NSMutableData that will hold the received data
		receivedData=[[NSMutableData data] retain];
		currentStatus = loggingout;
		return 1;
	} else {
		// inform the user that the download could not be made
		NSLog(@"Cannot Make A Connection to: %@", LOGIN_TARGET_URL);
		return 0;
	}
	return 0;
}

#pragma mark DELEGATE FUNCTIONS

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    // this method is called when the server has determined that it
    // has enough information to create the NSURLResponse
    // it can be called multiple times, for example in the case of a 
    // redirect, so each time we reset the data.
    [receivedData setLength: 0];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    // append the new data to the receivedData
    [receivedData appendData:data];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    // release the connection, and the data object
    [connection release]; connection = nil; theConnection = nil;
    [receivedData release]; 
		
    // Inform the user
    NSLog(@"Connection failed! Error - %@ %@",
          [error localizedDescription],
          [[error userInfo] objectForKey:NSErrorFailingURLStringKey]);
	
	// Inform the client word: nil == translation failed, and finishing the process
	currentStatus = login_idle;
}

-(NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse
{
	return request;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    // Analyze downloaded html file
 	NSString *html = [[NSString alloc] initWithData: receivedData encoding: NSISOLatin1StringEncoding ];	// Downloaded HTML (iso-8859-1 encoding)

	if (currentStatus == loggingin){		
		AGRegex *regex = [[AGRegex alloc] initWithPattern: @"You are logged in as" options: AGRegexCaseInsensitive];		// Regular Expression
		AGRegexMatch *match = [regex findInString:html];				

		// Login Succeeded
		if ([match count]>=1) {		
			[_delegate setLoginStatus: loggedin];
		} else {
			[_delegate setLoginStatus: loginfailed];	// Login Failed.
		}
		[regex release];
	} else if (currentStatus == loggingout){
		AGRegex *regex = [[AGRegex alloc] initWithPattern: @"autologin" options: AGRegexCaseInsensitive];					// Regular Expression
		AGRegexMatch *match = [regex findInString:html];				

		// Login Succeeded
		if ([match count]>=1) {		
			[_delegate setLoginStatus: notloggedinyet];
		} else {	
			[_delegate setLoginStatus: logoutfailed];	// Logout Failed.
		}	
		[regex release];
	}
	
    // Release the connection and data object
    [connection release]; connection = nil; theConnection = nil;
    [receivedData release]; [html release];
	currentStatus = login_idle;
}

- (void) cancel
{
	if (currentStatus == loggingin || currentStatus == loggingout){
		[theConnection cancel];
		[theConnection release]; theConnection = nil;
		[receivedData release];
		currentStatus = login_idle;
	}
}

@end
