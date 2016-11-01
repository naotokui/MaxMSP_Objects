//
//  NTFreesoundSearch.h
//  NTFreesoundSearch
//
//  Created by Nao Tokui on 11/21/06.
//  Copyright 2006 www.sonasphere.com. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>
#import "NTLoginConnection.h"
#import "NTGeneralURLAccess.h"
#import "freesoundsearch.h"

@interface NTFreesoundSearch : NSObject {
	t_freesoundsearch	*_delegate;			// max object

	/* Objects */
	NTLoginConnection	*loginObj;
	NTGeneralURLAccess	*queryObj;
	
	/* Internal Flags */
	LoginStatus			loginStatus;
	BOOL				isVerbose;
	
	/* Search Resut */
	NSMutableArray		*resultArray;
	NSMutableArray		*sampleInfoAccesses;
	
	/* Preview */
	QTMovie				*previewMovie;
	NSString			*previewURLStr;
	float				previewRate;
	float				previewVolume;
	
	/* Download */
	NSMutableArray		*sampleDLAccesses;
	NSString			*downloadDir;
}

- (void) setDelegate: (t_freesoundsearch *) x;

/* Login Management */
- (void) startLogin: (const char *) login : (const char *) password;
- (void) startLogout;
- (void) setLoginStatus: (LoginStatus) status;

/* Search */
- (void) startSearchText: (const char *) queryword minDuration: (float) minDur maxDuration: (float) maxDur minBitrate: (int) minBR
		minBitdepth: (int)minBD minSamplerate: (int) minSR searchDescriptions: (int) descriptions searchTags: (int) tags 
		searchFilenames: (int) filenames searchUsernames: (int) usernames;
- (void) processSearchResultXMLData: (NSDictionary *) dataDic;
- (void) processSampleXMLData: (NSDictionary *) dataDic;
- (void) URLAccessDone: (NTGeneralURLAccess *) accessObj;
- (NSArray *) searchResults;

/* PREVIEW */
- (int) startPreviewSampleWithID: (int) sampleID;
- (int) startPlaying: (NSString *) url;
- (void) stopPreview;
- (void) previewMovieFinished: (NSNotification *) notification;
- (void) previewMovieStatusStringPosted: (NSNotification *) notification;

/* DOWNLOAD */
- (int) startDownloadSampleWithID: (int) sampleID;
- (void)processDownloadedSampleData: (NSDictionary *) dataDic;
- (int) setDownloadPath: (char *) cpath;

/* Flags */
- (void) setVerbose: (BOOL) flag;
- (BOOL) isVerbose;
- (void) setPreviewRate: (float) rate;
- (void) setPreviewVolume: (float) vol;
@end
