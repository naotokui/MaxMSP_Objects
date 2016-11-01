/*
 *  freesoundsearch.h
 *  freesoundsearch
 *
 *  Created by Nao Tokui on 1/14/07.
 *  Copyright 2007 www.sonasphere.com. All rights reserved.
 *
 */


#include "ext.h"
#import <Cocoa/Cocoa.h>

@class NTFreesoundSearch;

// Login Status
typedef enum _LoginStatus { notloggedinyet, loggedin, loginfailed, logoutfailed } LoginStatus;

typedef struct _mootcher_x
{
	// Object Settings
	struct object	m_ob;
	void			*bang_out;
	void			*login_out;					// outlet for login status
	void			*resultnum_out;				// outlet for number of samples found in the previous search
	void			*anything_out;				// outlet for search/DL result 
	
	// Main Object
	NTFreesoundSearch	*t_searchObj;
	NSAutoreleasePool	*t_pool;

	/* Search Settings */						// Default Value
	float			t_minDuration;				// 0 sec
	float			t_maxDuration;				// 6000	sec
	int				t_minBitrate;				// 0	
	int				t_minBitdepth;				// 0
	int				t_minSampleRate;			// 0
	int				t_searchDescriptions;		// YES
	int				t_searchTags;				// YES
	int				t_searchFilenames;			// NO
	int				t_searchUsernames;			// NO
	
	/* Flags */
	int				t_verbose;					// YES
	int				t_loginstatus;				// 0: (not logged in)
} t_freesoundsearch;

void *freesoundsearch_class;

void freesoundsearch_bang(t_freesoundsearch *x);
void freesoundsearch_assist(t_freesoundsearch *x, void *b, long m, long a, char *s);

void freesoundsearch_login(t_freesoundsearch *x, t_symbol *login, t_symbol *password);
void freesoundsearch_logout(t_freesoundsearch *x);
void freesoundsearch_search(t_freesoundsearch *x, t_symbol *s);

void freesoundsearch_free(t_freesoundsearch *x);
void *freesoundsearch_new(t_symbol *s, short ac, t_atom *av);

/* Search Settings */
void freesoundsearch_min_duration(t_freesoundsearch *x, double f);
void freesoundsearch_max_duration(t_freesoundsearch *x, double f);
void freesoundsearch_min_bitrate(t_freesoundsearch *x, long n);
void freesoundsearch_min_bitdepth(t_freesoundsearch *x, long n);
void freesoundsearch_min_samplerate(t_freesoundsearch *x, long n);
void freesoundsearch_search_descriptions(t_freesoundsearch *x, long n);
void freesoundsearch_search_tags(t_freesoundsearch *x, long n);
void freesoundsearch_search_filenames(t_freesoundsearch *x, long n);
void freesoundsearch_search_usernames(t_freesoundsearch *x, long n);

/* Preview/Download */
void freesoundsearch_preview(t_freesoundsearch *x, long n);
void freesoundsearch_stop_preview(t_freesoundsearch *x);
void freesoundsearch_download(t_freesoundsearch *x, long n);

/* Other Functions */
void freesoundsearch_verbose(t_freesoundsearch *x, long n);
void freesoundsearch_setlogin(t_freesoundsearch *x, int n);

void freesoundsearch_rate(t_freesoundsearch *x, float f);
void freesoundsearch_vol(t_freesoundsearch *x, float f);
void freesoundsearch_download_dir(t_freesoundsearch *x, t_symbol *t);

/* Output */
void freesoundsearch_output_result(t_freesoundsearch *x, NSDictionary *dict);
void freesoundsearch_output_download(t_freesoundsearch *x, const char *filepath, int done);
void freesoundsearch_output_message(t_freesoundsearch *x, const char *msg);
void freesoundsearch_output_bang(t_freesoundsearch *x);
void freesoundsearch_output_samplenum(t_freesoundsearch *x, long n);