//
//  NTGeneralURLAccess.m
//  freesound_search
//
//  Created by Nao Tokui on 11/21/06.
//  Copyright 2006 www.sonasphere.com. All rights reserved.
//

#import "NTGeneralURLAccess.h"
#import "NTFreesoundSearch.h"
#import "AGRegex.h"

#define		TIMEOUT_INTERVAL	30.0

@implementation NTGeneralURLAccess

- (void) setDelegate: (NTFreesoundSearch *) obj
{
	_delegate = obj;
}
- (void) setDelegateFunction: (SEL) selector
{
	_function = selector;
}

- (id) delegate{ return _delegate; }

 - (void) dealloc {
	[filepath release];
	[super dealloc];
}

- (void) setFilepath: (NSString *) path
{
	[filepath release];
	filepath = [path retain];
}
- (NSString *) filepath{ return filepath; }

- (int) startURLAccess: (const NSURL *) urlObj: (const NSString *) postStr	
{	
	// Check if there is any on-going request  
	if (currentStatus == accessing) return 0;
		
	// Create a URL request
	NSString *urlStr  = [urlObj absoluteString];
	NSMutableURLRequest *urlRequest=[[NSMutableURLRequest alloc] initWithURL: (NSURL *)urlObj
								cachePolicy: NSURLRequestUseProtocolCachePolicy  // DO CACHE!
								timeoutInterval: (NSTimeInterval)TIMEOUT_INTERVAL];
	NSAssert(urlRequest, @"Cannot make a URL request");				
								
	// Configure the request 
	[urlRequest setHTTPMethod:@"POST"];
	[urlRequest setHTTPBody:[postStr dataUsingEncoding: NSISOLatin1StringEncoding]];
		
	// Create the connection with the request and start loading the data
	theConnection=[[NSURLConnection alloc] initWithRequest: urlRequest delegate:self];
	[urlRequest release];
	
	if (theConnection) {
		// Create the NSMutableData that will hold the received data
		receivedData	=[[NSMutableData data] retain];
		currentStatus	= accessing;
		return 1;
	} else {
		// Inform the user that the download could not be made
		NSLog(@"Cannot Make A Connection to: %@", urlStr);
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

-(NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse
{
		return request;
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    // release the connection, and the data object
    [connection release]; 
    [receivedData release]; 
	
    // inform the user
    NSLog(@"Connection failed! Error - %@ %@",
          [error localizedDescription],
          [[error userInfo] objectForKey:NSErrorFailingURLStringKey]);
	
	// Notify the access failed
	[_delegate performSelector: _function withObject: nil];	  
	[_delegate URLAccessDone: self];	  
	
	currentStatus = access_idle;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    // Send back the received data.
	NSDictionary *dataDic	= [NSDictionary dictionaryWithObjectsAndKeys: 
		receivedData, @"receivedData", self, @"accessObj", nil];
	[_delegate performSelector: _function withObject: dataDic];
	
    // release the connection, and the data object
    [connection release];
    [receivedData release];
	
	// Notify that the access was done.
	[_delegate URLAccessDone: self];
	
	currentStatus = access_idle;
}

- (void) cancel
{
	if (currentStatus == accessing){
		[theConnection cancel];
		[theConnection release];
		[receivedData release];
		currentStatus = access_idle;
	}
}



@end


