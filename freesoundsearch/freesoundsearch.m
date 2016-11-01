/* mootcher_x.c -- output the mootcher_x of a group of numbers ------- */

#import "freesoundsearch.h"
#import "NTFreesoundSearch.h"

void main()
{
	setup((t_messlist **)&freesoundsearch_class, (method)freesoundsearch_new, (method)freesoundsearch_free,
																(short)sizeof(t_freesoundsearch), 0L, A_GIMME, 0);
	addbang((method)freesoundsearch_bang);
	addmess((method)freesoundsearch_assist,"assist",A_CANT,0);
	
	addmess((method)freesoundsearch_search, "search", A_SYM, 0);
	addmess((method)freesoundsearch_login, "login", A_SYM, A_SYM, 0);
	addmess((method)freesoundsearch_logout, "logout", 0);
	
	/* Search Settings */
	addmess((method)freesoundsearch_min_duration, "min_duration", A_FLOAT, 0);
	addmess((method)freesoundsearch_max_duration, "max_duration", A_FLOAT, 0);
	addmess((method)freesoundsearch_min_bitrate, "min_bitrate", A_LONG,0);
	addmess((method)freesoundsearch_min_bitrate, "min_bitdepth", A_LONG,0);
	addmess((method)freesoundsearch_min_samplerate, "min_samplerate", A_LONG,0);
	addmess((method)freesoundsearch_search_descriptions, "search_descriptions", A_LONG, 0);
	addmess((method)freesoundsearch_search_tags, "search_tags", A_LONG, 0);
	addmess((method)freesoundsearch_search_filenames, "search_filenames", A_LONG, 0);
	addmess((method)freesoundsearch_search_usernames, "search_usernames", A_LONG, 0);
	
	/* Other Settings */
	addmess((method)freesoundsearch_verbose, "verbose", A_LONG, 0);
//	addmess((method)freesoundsearch_vol, "vol", A_FLOAT, 0);
//	addmess((method)freesoundsearch_rate, "rate", A_FLOAT, 0);
	addmess((method)freesoundsearch_download_dir, "download_dir", A_SYM, 0);
	
	/* Preview/Download */
	addmess((method)freesoundsearch_preview, "preview", A_LONG, 0);
	addmess((method)freesoundsearch_stop_preview, "stop_preview", 0);
	addmess((method)freesoundsearch_download, "download", A_LONG, 0);
}

void freesoundsearch_bang(t_freesoundsearch *x)
{

}


#pragma mark 
#pragma mark SEARCH

void freesoundsearch_search(t_freesoundsearch *x, t_symbol *s)
{
	// Make a Query string
	char query[255];
	strcpy(query, s->s_name); 
	
	// Start searching...
	[x->t_searchObj startSearchText: query minDuration: x->t_minDuration maxDuration: x->t_maxDuration 
		minBitrate: x->t_minBitrate minBitdepth: x->t_minBitdepth minSamplerate: x->t_minSampleRate 
		searchDescriptions: x->t_searchDescriptions searchTags: x->t_searchTags 
		searchFilenames: x->t_searchFilenames searchUsernames: x->t_searchUsernames];
}

#pragma mark 
#pragma mark LOGIN/LOGOUT 

void freesoundsearch_login(t_freesoundsearch *x, t_symbol *login, t_symbol *passwd)
{
	// maek User ID and Password strings
	char username[255];
	char password[255];
	strcpy(username, login->s_name); 
	strcpy(password, passwd->s_name);
	
	[x->t_searchObj startLogin: username : password];
}

void freesoundsearch_logout(t_freesoundsearch *x)
{
	[x->t_searchObj startLogout];
}

void freesoundsearch_assist(t_freesoundsearch *x, void *b, long m, long a, char *s)
{
	if (m==ASSIST_INLET) {
		sprintf(s,"Query Word / Search Settings "); 
	}
	else {
		switch (a) {
			case 0: sprintf(s,"Search / DL Result etc."); break;
			case 1: sprintf(s,"Number of Samples Found in the Previous Search."); break;
			case 2: sprintf(s,"Login Status"); break;			
			case 3: sprintf(s,"bang When Done"); break;
		}
	}
}

void freesoundsearch_free(t_freesoundsearch *x){
	[x->t_searchObj release];
	[x->t_pool release];
}


void *freesoundsearch_new(t_symbol *s, short ac, t_atom *av)
{
	t_freesoundsearch *x;
	
	// Memory Allocation
	x = (t_freesoundsearch *)newobject(freesoundsearch_class);
	x->bang_out			= bangout(x);
	x->login_out		= intout(x);;
	x->resultnum_out	= intout(x);
	x->anything_out		= outlet_new(x, 0L);					// 0 to indicate the outlet will be used to send various messages
	
	// Initialization
	x->t_pool = [[NSAutoreleasePool alloc] init];
	x->t_searchObj = [[NTFreesoundSearch alloc] init];			// main object for handling everything but Max messages
	if (!x->t_searchObj) {
		error("cannot initialize NTFreesoundSearch object");
		return NULL;
	}
	[x->t_searchObj setDelegate: x];
	
	// Initialize variables
	x->t_minDuration			= 0.0;
	x->t_maxDuration			= 6000.0;
	x->t_minBitrate				= 0;
	x->t_minBitdepth			= 0;
	x->t_minSampleRate			= 0;
	x->t_searchDescriptions		= 1;
	x->t_searchTags				= 1;
	x->t_searchFilenames		= 0;
	x->t_searchUsernames		= 0;
	x->t_verbose				= 1;
	[x->t_searchObj setVerbose: x->t_verbose];
	
	return x;
}

#pragma mark 
#pragma mark SEARCH SETTINGS

void freesoundsearch_min_duration(t_freesoundsearch *x, double f){
	x->t_minDuration	= f;
}

void freesoundsearch_max_duration(t_freesoundsearch *x, double f){
	x->t_maxDuration	= f;
}

void freesoundsearch_min_bitrate(t_freesoundsearch *x, long n){
	x->t_minBitrate	= n;
}

void freesoundsearch_min_bitdepth(t_freesoundsearch *x, long n){
	x->t_minBitdepth	= n;
}

void freesoundsearch_min_samplerate(t_freesoundsearch *x, long n){
	x->t_minSampleRate = n;
}

void freesoundsearch_search_descriptions(t_freesoundsearch *x, long n){
	x->t_searchDescriptions = (n!=0);
}

void freesoundsearch_search_tags(t_freesoundsearch *x, long n){
	x->t_searchTags = (n!=0);
}

void freesoundsearch_search_filenames(t_freesoundsearch *x, long n){
	x->t_searchFilenames = (n!=0);
}

void freesoundsearch_search_usernames(t_freesoundsearch *x, long n){
	x->t_searchUsernames = (n!=0);
}




#pragma mark 
#pragma mark OTHER PREVIEW/DOWNLOAD

void freesoundsearch_preview(t_freesoundsearch *x, long n){
	switch ([x->t_searchObj startPreviewSampleWithID: n]){
		case 0:
			error("no sample with ID %d found in the current search result.", n);
			return;
		case -1:
			error("no preview URL found.");
			return;
	}
}

void freesoundsearch_stop_preview(t_freesoundsearch *x){
	[x->t_searchObj stopPreview];
	freesoundsearch_output_message(x, "preview_done");
}


void freesoundsearch_download(t_freesoundsearch *x, long n){
	switch ([x->t_searchObj startDownloadSampleWithID: n]){
		case 0:
			error("no sample with ID %d found in the current search result.", n);
			return;
		case -2:
			error("too many download in process.");
			return;
	}
}

#pragma mark 
#pragma mark OTHER SETTINGS/FUNCTIONS

void freesoundsearch_verbose(t_freesoundsearch *x, long n){
	x->t_verbose = (n!=0);
	[x->t_searchObj setVerbose: (BOOL)x->t_verbose];
}

void freesoundsearch_vol(t_freesoundsearch *x, float f){
	[x->t_searchObj setPreviewVolume: f];
}

void freesoundsearch_rate(t_freesoundsearch *x, float f){
	[x->t_searchObj setPreviewRate: f];
}

void freesoundsearch_download_dir(t_freesoundsearch *x, t_symbol *t){
	switch ([x->t_searchObj setDownloadPath: t->s_name]){
		case 1:
			if (x->t_verbose) post("download directory was set to :%s", t->s_name);
			break;
		case 0:
			error("cannot set download directory: invalid path");
			break;
		case -1:	
			error("cannot set download directory: not directory");
			break;
	}
}

void freesoundsearch_setlogin(t_freesoundsearch *x, int n){					// internal function
	x->t_loginstatus = n;
	outlet_bang(x->bang_out);	
	outlet_int(x->login_out, x->t_loginstatus);		
}

#pragma mark 
#pragma mark OUTPUT

void freesoundsearch_output_result(t_freesoundsearch *x, NSDictionary *dict){
	t_atom		atoms[6];
	const int	max_length = 120;

	// Sample ID & Filename	& Username
	atom_setlong(atoms,  [[dict valueForKey:@"sample id"] intValue]); // SETLOG -> atom_setlong
	atom_setsym(atoms+1, gensym((char *)[[dict valueForKey:@"original filename"] cStringUsingEncoding:NSISOLatin1StringEncoding])); // SETSYS -> atom_setlong
	atom_setsym(atoms+2, gensym((char *)[[dict valueForKey:@"user name"] cStringUsingEncoding:NSISOLatin1StringEncoding])); 	 // SETSYM -> atom_setsym

	// Description & Tags - trancated to the first 120 characters
	NSString *desc		= [dict valueForKey:@"description"];
	if ([desc length]>max_length){
		NSRange	range		= NSMakeRange(0, max_length);
		desc = [desc substringWithRange: range];
		desc = [desc stringByAppendingString: @"..."];
	}
	NSString *tags		= [dict valueForKey:@"tags"];
	if ([tags length]>max_length){
		NSRange	range		= NSMakeRange(0, max_length);
		tags = [tags substringWithRange: range];
		desc = [desc stringByAppendingString: @"..."];
	}	
	atom_setsym(atoms+3, gensym((char *)[desc cStringUsingEncoding:NSISOLatin1StringEncoding]));
	atom_setsym(atoms+4, gensym((char *)[tags cStringUsingEncoding:NSISOLatin1StringEncoding]));
	
	// preview URL
	atom_setsym(atoms+5, gensym((char *)[[dict valueForKey:@"preview URL"] cStringUsingEncoding:NSISOLatin1StringEncoding]));

	// You cannot use outlet_list(), because the first item in the list must be an integer or float.
	outlet_anything(x->anything_out, gensym("samples"), 6, atoms);
}

void freesoundsearch_output_download(t_freesoundsearch *x, const char *filepath, int done){
	t_atom		atoms[2];
	
	atom_setlong(atoms, done);
	atom_setsym(atoms+1, gensym((char *)filepath));
	
	outlet_anything(x->anything_out, gensym("download"), 2, atoms);
}

void freesoundsearch_output_message(t_freesoundsearch *x, const char *msg){
	outlet_anything(x->anything_out, gensym(msg), 0, NULL);
}

// bang When done
void freesoundsearch_output_bang(t_freesoundsearch *x)
{
	outlet_bang(x->bang_out);
}

// number of samples found
void freesoundsearch_output_samplenum(t_freesoundsearch *x, long n)
{
	outlet_int(x->resultnum_out, n);
}

