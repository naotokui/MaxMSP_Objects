//
//  freesound_search.m
//  freesound_search
//
//  Created by Nao Tokui on 11/21/06.
//  Copyright 2006 www.sonasphere.com. All rights reserved.
//

#import "NTFreesoundSearch.h"
#include "ext.h"

#define	SEARCH_TARGET_URL	@"http://www.freesound.org/searchTextXML.php"
#define	SAMPLE_TARGET_URL	@"http://www.freesound.org/samplesViewSingleXML.php"
#define DOWNLOAD_TARGET_URL @"http://www.freesound.org/download"

#define START_SAMPLE		0					// For "start" parameter 
#define MAX_SAMPLES			100					// For "limit" parameter
#define MAX_DOWNLOAD		5

@implementation NTFreesoundSearch

- (id) init {
	self = [super init];
	if (self != nil) {
		// Initialization
		loginStatus = notloggedinyet;
		
		// Object for Login / Logout
		loginObj	=	[[NTLoginConnection alloc] init];
		[loginObj setDelegate: self];
		
		// Object for search
		queryObj	= [[NTGeneralURLAccess alloc] init];
		[queryObj setDelegate: self];
		[queryObj setDelegateFunction: @selector(processSearchResultXMLData:)];
		resultArray			= [[NSMutableArray alloc] init];
		sampleInfoAccesses	= [[NSMutableArray alloc] init];
		sampleDLAccesses	= [[NSMutableArray alloc] init];
		
		// Preview Setting
		previewRate		= 1.0;
		previewVolume	= 1.0;
		previewMovie	= nil;
		
		// Download Setting 
		downloadDir		= @"~/Desktop";
	}
	return self;
}

- (void) dealloc {
	[loginObj cancel];		[loginObj release]; 
	[queryObj cancel];		[queryObj release];
	
	[resultArray release]; 
	
	NSEnumerator *enm			= [sampleInfoAccesses objectEnumerator];
	NTGeneralURLAccess *access;
	while (access = [enm nextObject]) [access cancel];
	[sampleInfoAccesses release];
	
	enm							= [sampleDLAccesses objectEnumerator];
	while (access = [enm nextObject]) [access cancel];
	[sampleDLAccesses release];
	
	if (previewMovie){
		[[NSNotificationCenter defaultCenter] removeObserver: self];
		[previewMovie stop];	
		[previewMovie release];
	}
	
	[previewURLStr release];
	[downloadDir release];
	
	[super dealloc];
}


- (void) setDelegate: (t_freesoundsearch *) x
{
	_delegate = x;
}

#pragma mark
#pragma mark LOGIN MANAGEMENT

- (void) startLogin: (const char *) login : (const char *) password
{	
	// Check if already logged in or not
	if ( loginStatus == loggedin ) {
		error("You have already logged in.");
		return;
	 } 
	
	// Start actual login procedure
	if ([loginObj startLogin: login : password]){
		if (isVerbose){
			post("freesoundsearch: trying to login as \"%s\"...", login);
		}
	} else {
		error("cannot start the login procedure.");
	}
}

- (void) startLogout
{
	// Check if logged in already
	if (loginStatus != loggedin ){
		error("You are not logged in yet.");
		return; 
	}
	
	// Start actual logout procedure
	if ([loginObj startLogout]){
		if (isVerbose){
			post("freesoundsearch: logging out...");
		}
	} else {
		error("cannot start the logout procedure.");
	}
}

- (void) setLoginStatus: (LoginStatus) status
{
	switch (status){
		case loggedin:
			loginStatus		=	loggedin;	
			if (isVerbose) post("freesoundsearch: login succeeded.");
			freesoundsearch_setlogin(_delegate, 1);							
			break;
		case notloggedinyet:
			loginStatus		=	notloggedinyet;	
			if (isVerbose) post("freesoundsearch: logged out.");
			freesoundsearch_setlogin(_delegate, 0);							
			break;
		case loginfailed:
			post("freesoundsearch: login failed. check your id & password");
			break;
		case logoutfailed:
			post("freesoundsearch: logout failed. any network problems?");
			break;
	}
	
	// set login status in the max object and let it output the current status from 2nd outlet
	
}

- (LoginStatus) loginStatus{	return loginStatus; }

#pragma mark
#pragma mark SEARCH

- (void) startSearchText: (const char *) queryword minDuration: (float) minDur maxDuration: (float) maxDur minBitrate: (int) minBR
		minBitdepth: (int)minBD minSamplerate: (int) minSR searchDescriptions: (int) descriptions searchTags: (int) tags 
		searchFilenames: (int) filenames searchUsernames: (int) usernames
{
	// Check if already login
	if (loginStatus != loggedin){
		error("you have not logged in yet.");
		return;
	}
	
	// Clear previous result
	[resultArray removeAllObjects];
	
	// Search Setting
	NSURL *url			= [NSURL URLWithString: SEARCH_TARGET_URL];
	NSString *queryStr	= [NSString stringWithCString: queryword];
	NSString *postStr	= [NSString stringWithFormat: 
		@"search=%@&durationMin=%f&durationMax=%f&bitrate=%d&bitdepth=%d&samplerate=%d&searchDescriptions=%d&searchTags=%d&searchFilenames=%d&searchUsernames=%d&start=%d&limit=%d&order=0", 
			queryStr, minDur, maxDur, minBR, minBD, minSR, descriptions, tags, filenames, usernames, START_SAMPLE, MAX_SAMPLES];
	
	
	// Start searching 
	if ([queryObj startURLAccess: url : postStr]){
		if (isVerbose){
			post("freesoundsearch: searching for %s", queryword);
		}	
	} else {
		error("previous search request still in process");
	}
}

- (void) processSearchResultXMLData: (NSDictionary *) dataDic
{
	NSData *xmlData		= [dataDic valueForKey:@"receivedData"];
	if (xmlData){
		NSString *xml = [[NSString alloc] initWithData: xmlData encoding: NSISOLatin1StringEncoding ];	// XML

		// Make XML Document from XML String
		NSError *err = nil;
		NSXMLDocument *resultXML		= [[NSXMLDocument alloc] initWithXMLString: xml options: NSXMLDocumentTidyXML error: &err];
		if (err!=nil){
			error("cannot parse XML returned from the server");
			NSLog(@"XML Parse Error: %@", [err description]);
			return;
		}

		// Parse search result
		NSArray *resultIDs				= [[resultXML rootElement] elementsForName: @"sample"];
		int resultNum					= [resultIDs count];
		freesoundsearch_output_samplenum(_delegate, resultNum);
		
		if (isVerbose) post("freesoundsearch: %d sample%s found.", resultNum, (resultNum>1)? "s":"");
		if (resultNum>MAX_SAMPLES){
			post("freesoundsearch: too many samples found.");
			post("freesoundsearch: the first %d samples will be output", MAX_SAMPLES);
		}
		
		NSEnumerator		*enm		= [resultIDs objectEnumerator];
		NSXMLElement		*element;
		NTGeneralURLAccess	*accessObj;
		NSURL				*url;
		NSString			*urlStr;
		int					count		= 0;
		while (element = [enm nextObject]){
			accessObj	= [[NTGeneralURLAccess alloc] init];
			[sampleInfoAccesses addObject: accessObj];
			[accessObj setDelegate: self];
			[accessObj setDelegateFunction: @selector(processSampleXMLData:)];
			
			urlStr		= [NSString stringWithFormat: @"%@?id=%@", SAMPLE_TARGET_URL, [[element attributeForName:@"id"] stringValue]];
			url			= [NSURL URLWithString: urlStr];
			[accessObj startURLAccess: url : nil];
			
			[accessObj release];
			
			if (count++>MAX_SAMPLES) break;
		} 
		
		[resultXML release]; [xml release];
	}
}

- (void) processSampleXMLData: (NSDictionary *) dataDic;
{
	NSData *xmlData		= [dataDic valueForKey:@"receivedData"];
	if (xmlData){
		NSString *xml = [[NSString alloc] initWithData: xmlData encoding: NSISOLatin1StringEncoding ];	// XML

		// Make XML Document from XML String
		NSError *err = nil;
		NSXMLDocument *resultXML		= [[NSXMLDocument alloc] initWithXMLString: xml options: NSXMLDocumentTidyXML error: &err];
		if (err!=nil){
			error("cannot parse XML returned from the server");
			NSLog(@"XML Parse Error: %@", [err description]);
			return;
		}

		// Parsing XML in a very rough way (Need to be refined!!)
		// I don't know the best way to convert XML into NSDictionary
		NSMutableDictionary *info	= [NSMutableDictionary dictionary];
		NSXMLElement		*sample	= [[[resultXML rootElement] elementsForName: @"sample"] objectAtIndex: 0];
		int sampleID				= [[[sample attributeForName: @"id"] stringValue] intValue];
		if (sampleID>0) [info setObject: [NSNumber numberWithInt:sampleID] forKey: @"sample id"];
		else return;
		
		NSXMLElement		*user	= [[sample elementsForName: @"user"] lastObject];
		NSXMLElement		*userName	= [[user elementsForName:@"name"] lastObject];
		if (userName) [info setObject: [userName stringValue] forKey: @"user name"];
		else [info setObject: @"-" forKey: @"user name"];
		
		NSXMLElement		*filename =[[sample elementsForName:@"originalFilename"] lastObject];
		if (filename) [info setObject: [filename stringValue] forKey:@"original filename"];
		else [info setObject: @"-" forKey: @"original filename"];

		NSXMLElement	*description =[[[[[[sample elementsForName:@"descriptions"] lastObject] 
						elementsForName:@"description"] lastObject] elementsForName:@"text"] lastObject];
		if (description) [info setObject: [description stringValue] forKey:@"description"];
		else [info setObject: @"-" forKey: @"description"];	
		
		NSXMLElement		*preview	= [[sample elementsForName: @"preview"] lastObject];	
		if (preview) [info setObject: [preview stringValue] forKey: @"preview URL"];
		else [info setObject: @"-" forKey: @"preview URL"];	
		
		NSArray			*tags = [[[sample elementsForName:@"tags"] lastObject] children];
		NSEnumerator	*tagEnm = [tags objectEnumerator];
		NSXMLElement	*tag;
		NSMutableString *tagStr = nil;
		while (tag = [tagEnm nextObject]){
			if (tagStr==nil) tagStr = [NSMutableString stringWithString: [tag stringValue]];
			else {
				[tagStr appendFormat: @", %@",[tag stringValue]];
			}
		}
		if (tagStr) [info setObject: tagStr  forKey:@"tags"];
		else [info setObject: @"-"  forKey:@"tags"];
		
		// Store Info
		[resultArray addObject: info];

		freesoundsearch_output_result(_delegate, info);
		[resultXML release]; [xml release];
	}
}


- (void) URLAccessDone: (NTGeneralURLAccess *) accessObj
{		
	// DO NOT release the URLAccess for user query
	if (accessObj == queryObj) return;
	
	if ([sampleInfoAccesses containsObject: accessObj]){
		[sampleInfoAccesses removeObject: accessObj];
		if ([sampleInfoAccesses count]==0) {
			freesoundsearch_output_bang(_delegate);
			freesoundsearch_output_message(_delegate, "search_done");
		}
	//	NSLog(@"%d objects", [sampleInfoAccesses count]);
	}
	
	if ([sampleDLAccesses containsObject: accessObj]){
		[sampleDLAccesses removeObject: accessObj];
		if ([sampleDLAccesses count]==0) {
			freesoundsearch_output_bang(_delegate);
			freesoundsearch_output_message(_delegate, "all_dl_done");
		}
	//			NSLog(@"%d dl objects", [sampleDLAccesses count]);
	}
}

- (NSArray *) searchResults{ return resultArray; }

#pragma mark 
#pragma mark PREVIEW

- (int) startPreviewSampleWithID: (int) sampleID
{
	// Stop previous preview if any 
	if (previewMovie){
		if (isVerbose) post("freesoundsearch: preview cancelled.");
		[[NSNotificationCenter defaultCenter] removeObserver: self name: QTMovieDidEndNotification object: previewMovie];
		[[NSNotificationCenter defaultCenter] removeObserver: self name: QTMovieStatusStringPostedNotification object: previewMovie];
		[previewMovie stop];
		[previewMovie release];
		previewMovie = nil;
	}
	if (previewURLStr){
		[previewURLStr release]; previewURLStr = nil;
	}

	// Check if information for the sample is already downloaded 
	NSDictionary	*sampleInfo		= nil;
	NSEnumerator	*enm			= [resultArray objectEnumerator];
	NSDictionary	*dict;
	while (dict = [enm nextObject]){
		if ([[dict valueForKey:@"sample id"] intValue]==sampleID){
			sampleInfo	= dict;
			break;
		}
	}
	if (sampleInfo == nil) return 0;						// no sample found
	
	// Start playing the preview URL
	previewURLStr	= [[sampleInfo valueForKey:@"preview URL"] retain];
	if (previewURLStr == nil){
		return -1;											// no preview URL found
	}
	
	if  ([self startPlaying: previewURLStr]==1){
		if (isVerbose){
			char *name		= (char *)[[sampleInfo valueForKey:@"original filename"] cString];
			if (name) post("freesoundsearch: start playing sample: %d %s", sampleID, name);
			else post("freesoundsearch: start playing sample: %d", sampleID);
		}
		return 1;
	}
	return -1;
}

- (int) startPlaying: (NSString *) url
{
	// Prepare
	NSURL *previewURL		= [NSURL URLWithString: url];
	NSError *err			= nil;
	previewMovie			= [[QTMovie alloc] initWithURL: previewURL error: &err];
	NSAssert1(previewMovie!=nil, @"Cannot initialize preview movie: %@", [err description]);
	if (err!=nil) return 0;
	
	// Settings
	[previewMovie setRate: previewRate];
	[previewMovie setVolume: previewVolume];
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(previewMovieFinished:) name: QTMovieDidEndNotification object: previewMovie];
	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(previewMovieStatusStringPosted:)
																				name: QTMovieStatusStringPostedNotification object: previewMovie];
	// Start playing
	[previewMovie play];
	freesoundsearch_output_message(_delegate, "preview_start");
	return 1;
}

- (void) stopPreview
{
	if (previewMovie){
		[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(previewMovieFinished:) name: QTMovieDidEndNotification object: previewMovie];
		[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(previewMovieStatusStringPosted:)
																				name: QTMovieStatusStringPostedNotification object: previewMovie];
		[previewMovie stop];
		[previewMovie release];
		previewMovie = nil;
		freesoundsearch_output_bang(_delegate);
	}
}

- (void) previewMovieFinished: (NSNotification *) notification
{
	if (isVerbose) post("freesoundsearch: preview finished");
	
	[[NSNotificationCenter defaultCenter] removeObserver: self name: QTMovieDidEndNotification object: previewMovie];
	[[NSNotificationCenter defaultCenter] removeObserver: self name: QTMovieStatusStringPostedNotification object: previewMovie];
	
	[previewMovie release];
	previewMovie = nil;
	
	// Output bang and message
	freesoundsearch_output_bang(_delegate);
	freesoundsearch_output_message(_delegate, "preview_done");
}

- (void) previewMovieStatusStringPosted: (NSNotification *) notification
{
	/* Restarting Playback ... */
	if ([[[notification userInfo] valueForKey:@"QTMovieStatusFlagsNotificationParameter"] intValue]==4){
		if (previewMovie){
			[[NSNotificationCenter defaultCenter] removeObserver: self name: QTMovieDidEndNotification object: previewMovie];
			[[NSNotificationCenter defaultCenter] removeObserver: self name: QTMovieStatusStringPostedNotification object: previewMovie];
			[previewMovie stop];
			[previewMovie release];
			previewMovie = nil;
		}	
		// Restart ...
		[self startPlaying: previewURLStr];
	}
}

#pragma mark 
#pragma mark DOWNLOAD

- (int) startDownloadSampleWithID: (int) sampleID
{
	if ([sampleDLAccesses count]>=MAX_DOWNLOAD){
		return -2;
	}
	
	// Check if information for the sample is already downloaded 
	NSDictionary	*sampleInfo		= nil;
	NSEnumerator	*enm			= [resultArray objectEnumerator];
	NSDictionary	*dict;
	while (dict = [enm nextObject]){
		if ([[dict valueForKey:@"sample id"] intValue]==sampleID){
			sampleInfo	= dict;
			break;
		}
	}
	if (sampleInfo == nil) return 0;						// no sample found
	
	// Create object for downloading
	NTGeneralURLAccess *downloadObj	= [[NTGeneralURLAccess alloc] init];
	[sampleDLAccesses addObject: downloadObj];
	[downloadObj setDelegate: self];
	[downloadObj setDelegateFunction: @selector(processDownloadedSampleData:)];
	
	// Make a URL to be downloaded and a local file path
	NSString *org_filename	= [sampleInfo valueForKey:@"original filename"];
	NSString *username		= [sampleInfo valueForKey: @"user name"];
	NSString *filename		= [NSString stringWithFormat: @"%@_%@", username, org_filename];	// local filename = (user name)_(original filename)
	NSString *filepath	= [[downloadDir stringByAppendingPathComponent: filename] stringByStandardizingPath];
	[downloadObj setFilepath: filepath];
	NSString	*urlStr	= [[NSString stringWithFormat: @"%@/%d/%d_%@_%@",
								DOWNLOAD_TARGET_URL, sampleID, sampleID, username, filename]
								stringByAddingPercentEscapesUsingEncoding: NSISOLatin1StringEncoding];
								
	// Start downloading ...
	NSURL		*url	= [NSURL URLWithString: urlStr];
	[downloadObj startURLAccess: url : nil];
	
	[downloadObj release];
	
	if (isVerbose){
		char *name		= (char *)[filename cString];
		if (name) post("freesoundsearch: start downloading sample: %d %s", sampleID, name);
		else post("freesoundsearch: start playing sample: %d", sampleID);
	}
	return 1;
}

- (void)processDownloadedSampleData: (NSDictionary *) dataDic
{
	NSData *data		= [dataDic valueForKey:@"receivedData"];
	NSString *filepath	= [(NTGeneralURLAccess *)[dataDic valueForKey:@"accessObj"] filepath];
	if (data && filepath){
		const char	*filepathChar	= [filepath cStringUsingEncoding: NSISOLatin1StringEncoding];
		
		if ([data writeToFile: filepath atomically: YES]){
			if (isVerbose){
				post("freesoundsearch: %s downloaded.", [[filepath lastPathComponent] cStringUsingEncoding: NSISOLatin1StringEncoding]);
			}
			freesoundsearch_output_bang(_delegate);
			freesoundsearch_output_download(_delegate, filepathChar, 1);
		} else {
			freesoundsearch_output_bang(_delegate);
			error("cannot save download file to: %s", [filepath cString]);
			freesoundsearch_output_download(_delegate, filepathChar, 0);
		}
	}
}

- (int) setDownloadPath: (char *) cpath
{
	NSString *path = [[NSString stringWithCString: cpath] stringByStandardizingPath];
	NSLog(path);
	
	BOOL isDir = NO;
	if ([[NSFileManager defaultManager] fileExistsAtPath: path isDirectory: &isDir]){
		if (isDir){
			[downloadDir release];
			downloadDir = [path retain];
			return 1;
		} else return -1;
	} else return 0;
}

#pragma mark 
#pragma mark FLAGS

- (void) setVerbose: (BOOL) flag
{
	isVerbose = flag;
}
- (BOOL) isVerbose{ return isVerbose; }

- (void) setPreviewRate: (float) rate
{
	previewRate = rate;
	if (previewMovie) [previewMovie setRate: previewRate];
}

- (void) setPreviewVolume: (float) vol
{
	previewVolume = vol;
	if (previewMovie) [previewMovie setVolume: previewVolume];
}

@end
