/* Mootcher.cpp **********************************************************************
	
	Mootcher 23-8-2005

	Mootcher Online Access to thefreesoundproject website
	http://freesound.iua.upf.edu/

	GPL 2005 Jorn Lemon
	mail for questions/remarks: mootcher@twistedlemon.nl
	or go to the freesound website forum

	-----------------------------------------------------------------

	Includes:
		curl.h    (version 7.14.0)
		tinyxml.h (version 2.3.4)
		sndfile.h (version 1.0.X) 
				// experimental sndfile version with 
				// mp3 & ogg support @ http://freesound.iua.upf.edu/
	Librarys:
		libcurl.lib tinyxmld.lib libsndfile.lib

	-----------------------------------------------------------------
	Licence GPL:

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


*************************************************************************************/
#include "Mootcher.h"

// compile the tinyXml lib with pre-definition: TIXML_USE_STL
// to overcome new/delete errors in flext treading
#include "tinyxml.h"
#include "sndfile.h"

//------------------------------------------------------------------------
Mootcher::	Mootcher(const char *saveLocation)
	: curl( NULL )
	, connection( NULL )
	, DBsize( 0 )
	, opensounds( 0 )
{
	changeWorkingDir(saveLocation);
};
//------------------------------------------------------------------------
Mootcher::	~Mootcher()
{
	for( std::vector<float*>::iterator i = allSoundsP.begin(); i != allSoundsP.end(); ++i )
		delete *i;
	allSoundsP.clear();
	allSoundsID.clear();
	allSoundsS.clear();

	logfile.close();
	remove( "cookiejar.txt" );
}
//------------------------------------------------------------------------
const char* Mootcher::changeWorkingDir(const char *saveLocation)
{
	basePath = saveLocation;
#ifdef __WIN32__
	std::string replace = "/";
	int pos = (int)basePath.find("\\");
	while( pos != std::string::npos ){
		basePath.replace(pos, 1, replace);
		pos = (int)basePath.find("\\");
	}
#endif
	// 
	int pos2 = basePath.find_last_of("/");
	if(basePath.length() != (pos2+1)) basePath += "/";
	// add a check if the given directory exists
	createResourceLocation();
	return basePath.c_str();
}
//------------------------------------------------------------------------
void		Mootcher::createResourceLocation()
{
	// create a freesound.xml if it does not exist
	xmlLocation = basePath + "freesound.xml";
	std::ifstream freesoundXMLin;
	freesoundXMLin.open(xmlLocation.c_str(), std::ifstream::in);
	freesoundXMLin.close();

	if (freesoundXMLin.fail() ){
		std::ofstream freesoundXMLout;
		freesoundXMLout.open(xmlLocation.c_str(), std::ofstream::out);
		freesoundXMLout << "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\n";
		freesoundXMLout << "<freesound> </freesound>\n";
		freesoundXMLout.close();
	
		// create a snd directory
		std::string sndLocation = basePath + "snd\\";
		if (!CreateDirectory((char *)sndLocation.c_str(), NULL))
			toLog("Directory 'snd' already exists.\n"); 
		
		// create logfile
		std::string logfileLocation = basePath + "mootcher_logfile.txt";
		logfile.open(logfileLocation.c_str());
#ifdef PD
		time_t rawtime;
		time ( &rawtime );
		logfile << "Mootcher session started: " << ctime(&rawtime);
#endif
		logfile << "Working in directory: " << basePath.c_str() << "\n";
		logfile << "-----------------------------------------------------\n" << std::flush;
	}
}
//------------------------------------------------------------------------
size_t		Mootcher::WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register int realsize = (int)(size * nmemb);
	struct MemoryStruct *mem = (struct MemoryStruct *)data;

	// There might be a realloc() out there that doesn't like 
	// reallocing NULL pointers, so we take care of it here
	if(mem->memory)	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	else			mem->memory = (char *)malloc(mem->size + realsize + 1);

	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}
//------------------------------------------------------------------------
int			Mootcher::audioFileWrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct OnlineFile *out=(struct OnlineFile *)stream;
	if(out && !out->stream) {
		// open file for writing
		out->stream=fopen(out->filename, "wb");
		if(!out->stream)
		return -1;	// failure, can't open file to write
	}
	return (int)fwrite(buffer, size, nmemb, out->stream);
};
//------------------------------------------------------------------------
void		Mootcher::toLog(std::string input)
{
	// Add a message to the logbook
	if (logfile.is_open()) logfile << input << std::flush;
}
//------------------------------------------------------------------------
int			Mootcher::localFilesTotal()
{
	// Update the DBsize variable and return it
	myLocalFile(0);
	return DBsize;
}
//------------------------------------------------------------------------
std::string	Mootcher::myLocalFile(int nr)
{
	int i = 0;
	std::string LocalFileID;
	// First check in the freesound.xml if the file is already downloaded	
	TiXmlDocument docLocalfile(xmlLocation.c_str());
	docLocalfile.LoadFile(xmlLocation.c_str());
	docLocalfile.Parse(xmlLocation.c_str());

	// parse the created DOM
	TiXmlElement *root1 = docLocalfile.RootElement();
	TiXmlElement *sample1 = 0;
	TiXmlNode *node1 = 0;

	// itterate all the ID's that already exist in freesound.xml
	while( node1 = root1->IterateChildren(node1) ){
		if( strcmp( node1->Value(), "sample") == 0 ){
			i++; // counter for nr. of samples
			if(i == nr)	{
				sample1 = (TiXmlElement *) node1;
				LocalFileID = sample1->Attribute("id");
			}	
		}
	}
	DBsize = i;

	// if request is out of range return a char "0"
	if(nr > DBsize || nr < 1) LocalFileID = "0";

	return LocalFileID;
}
//------------------------------------------------------------------------
void		Mootcher::setcUrlOptions()
{
	curl = curl_easy_init();
	// basic init for curl
	curl_global_init(CURL_GLOBAL_ALL);
	// send all data to 'WriteMemoryCallback' function
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); 
	// some servers don't like requests that are made without a user-agent field, so we provide one
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	// setup curl error buffer
	CURLcode res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
	// always use the cookie with session id which is received at the login
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookiejar.txt");
}
//------------------------------------------------------------------------
int			Mootcher::doLogin(std::string login, std::string password)
{
	struct MemoryStruct xml_page;
	xml_page.memory = NULL;
	xml_page.size = NULL;

	// create the post message from the login and password
	std::string postMessage;
	postMessage += "username=";
	postMessage += curl_escape(login.c_str(), 0);
	postMessage += "&password=";
	postMessage += curl_escape(password.c_str(), 0);
	postMessage += "&login=";
	postMessage += curl_escape("1", 0);
	postMessage += "&redirect=";
	postMessage += curl_escape("../tests/login.php", 0);

	// Do the setup for libcurl
	setcUrlOptions();
	// we will pass our 'xml_page' struct to the callback function 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml_page);
	// save the sessoin id that is given back by the server in a cookie
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookiejar.txt");
	// Allow redirection
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	// use POST for login variables
	curl_easy_setopt(curl, CURLOPT_POST, TRUE);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postMessage.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1);

	if(curl)
	{
		// the url to get
		std::string login_url = "http://freesound.iua.upf.edu/forum/login.php";
		curl_easy_setopt(curl, CURLOPT_URL, login_url.c_str() );

		// preform online request
		CURLcode res = curl_easy_perform(curl);
		// if something go's wrong 'connection' is set to '0'
		if(strcmp(curl_easy_strerror(res), "no error") == 0) connection = 1;
		else connection = 0;

		if (connection == 1){
			std::string check_page = xml_page.memory;
			int test = (int)check_page.find("logged"); 
			if(	test != -1){
				sprintf(message, "Login: %s\n", xml_page.memory);
				toLog(message);
			}
			else {
				sprintf(message, "Login: Check username and password\n");
				toLog(message);
				connection = 0;
			}
		}

		// free the memory
		if(xml_page.memory){		
			free( xml_page.memory );
			xml_page.memory = NULL;
			xml_page.size = NULL;
		}

		// fills the cookie jar with the session id, which is related to the login.
		curl_easy_cleanup(curl);

		return connection;
	}
	else return 3; // will be returned if a curl related problem ocurrs
}
//------------------------------------------------------------------------
std::string	Mootcher::searchText(std::string word, const float durMin, float durMax, int bitRate, int bitDepth, int sampleRate, int nr_request, int &nr_result)
{
	struct MemoryStruct xml_page;
	xml_page.memory = NULL;
	xml_page.size = NULL;
	
	std::string result;
	std::string resultList;
	int i = 0;
	int nr = nr_request;

	if(connection != 0)
	{
		// create a url encoded post message
		std::string postMessage;
		char tempString[ 128 ];
		char *tempPointer = &tempString[0];

		postMessage = "search=";
		postMessage += curl_escape(word.c_str(), 0);
		sprintf( tempPointer, "&durationMin=%.2f", durMin );
		postMessage += tempPointer;
		if(durMax > 0.){ // if max. duration is 0. no results will be shown
			sprintf( tempPointer, "&durationMax=%.2f", durMax );
			postMessage += tempPointer;
		}
		sprintf( tempPointer, "&bitrate=%d", bitRate);
		postMessage += tempPointer;
		sprintf( tempPointer, "&bitdepth=%d", bitDepth);
		postMessage += tempPointer;
		sprintf( tempPointer, "&samplerate=%d", sampleRate);
		postMessage += tempPointer;

		// basic init for curl 
		curl = curl_easy_init();
		curl_global_init(CURL_GLOBAL_ALL);	
		// always use the cookie with session id which is received at the login 
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookiejar.txt");
		// send all data to 'WriteMemoryCallback' function  
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); 
		// some servers don't like requests that are made without a user-agent field, so we provide one 
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		// setup curl error buffer 
		CURLcode res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
		
		if(curl)
		{
			// pass the 'xml_page' struct to the callback function
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml_page);
			// setup the post message
			curl_easy_setopt(curl, CURLOPT_POST, TRUE);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postMessage.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1);
			
			// the url to get
			std::string search_url = "http://freesound.iua.upf.edu/searchTextXML.php";
			curl_easy_setopt(curl, CURLOPT_URL, search_url.c_str());

			// preform the online search 
 			CURLcode res = curl_easy_perform(curl);
			if(strcmp(curl_easy_strerror(res), "no error") == 0) connection = 1;
			else connection = 0;
			
			// create a TinyXML document and associate it to the downloaded page
			TiXmlDocument doc;
			doc.Parse( xml_page.memory );

			// parse the created document
			TiXmlElement *root = doc.RootElement();
			std::string error = root->Value();
			
			// if the page is not a valid xml document with a 'freesound' root
			if(error != "freesound"){
				sprintf(message, "searchtext: There is no valid root in the xml file");
				toLog(message);
			}
			else {
				TiXmlElement *sample = 0;
				TiXmlNode *node = 0;
				i = 0;
				
				// if the requested value is to low, set to default
				if(nr < 1) nr = 1;

				// get the "id" 
				while( node = root->IterateChildren(node) ){
					if( strcmp( node->Value(), "sample") == 0 ){
						++i;
						sample = (TiXmlElement *) node;
						resultList += sample->Attribute("id");
						resultList += " ";
						if(i == nr){
							result = sample->Attribute("id");
						}
					}
				}
			}

			// free the memory
			if(xml_page.memory){
				free( xml_page.memory );
				xml_page.memory = NULL;
				xml_page.size = NULL;
			}
			root->Clear();

			// curl_easy_cleanup, fills cookiejar with session id
			curl_easy_cleanup(curl);

		}
	}
	if(result == "") result = "0";
	if(resultList == "") resultList = "0";

	if(nr_request == 0){
		nr_result = i;
		return resultList;
	}
	else {
		nr_result = 1;
		return result;
	}
}
//------------------------------------------------------------------------
void		Mootcher::GetSimilarXml(std::string ID, int dis_sim, struct MemoryStruct &xml_page)
{
	// basic curl setup
	setcUrlOptions();
	// we pass our 'xml_page' struct to the callback function
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml_page);

	if(curl)
	{
		// create URL to get
		std::string getsim_url = "http://freesound.iua.upf.edu/searchSimilarXML.php?id=";
		std::string disSimillar;	
		if(dis_sim == 0) disSimillar = "&inverse=false";
		else disSimillar = "&inverse=true";
		getsim_url += ID.c_str();
		getsim_url += disSimillar;
		
		curl_easy_setopt(curl, CURLOPT_URL, getsim_url.c_str() );
		
		// get the online xml page
		CURLcode res = curl_easy_perform(curl);
		if(strcmp(curl_easy_strerror(res), "no error") == 0) connection = 1;
		else connection = 0;
	}
}
//------------------------------------------------------------------------
std::string	Mootcher::getSimilar(int dis_sim, std::string ID, int nr, int &nr_result)
{
	struct MemoryStruct xml_page;
	xml_page.memory = NULL;
	xml_page.size = NULL;
	int nrOriginal = nr;
	int i = 0;
	std::string similar;
	std::string similarList;
	TiXmlDocument *doc = NULL;

	if(connection != 0)
	{
		// get the online XML page
		GetSimilarXml(ID.c_str(), dis_sim, xml_page);

		// Create a DOM from the struct 'xml_page'
		doc = new TiXmlDocument(xmlLocation.c_str());
		doc->Parse( xml_page.memory );
		
		// must be a check if 'xml_page' contains valid data
		if(doc->ErrorId() == 0)
		{
			// results are limited to 19
			if(nr < 1) nr = 1;

			// parse the created DOM
			TiXmlElement *root = doc->RootElement();
			std::string error = root->Value();
			
			// if the page is not a valid xml document with a 'freesound' root
			if(error != "freesound"){
				sprintf(message, "getSimilar: There is no valid 'freesound' root in the xml file\n");
				toLog(message);
				similar = "0";
				return similar;
			}
			else {
				TiXmlElement *sample = 0;
				TiXmlNode *node = 0;
				// get the "id" 
				while( node = root->IterateChildren(node) ){
					i++;
					if( strcmp( node->Value(), "sample") == 0 ){
						sample = (TiXmlElement *) node;
						similarList += sample->Attribute("id");
						similarList += " ";
						if(i == nr){
							similar = sample->Attribute("id");
						}
					}
				}
			}
		}
		else
		{
			sprintf(message, "getSimilar: Error in xml file: %s\n", doc->ErrorDesc());
			toLog(message);			
		}
	}

	if(similar == "") similar = "0";
	if(nrOriginal < 1) {
		nr_result = i;
		return similarList;
	}
	else {
		nr_result = 1;
		return similar;
	}
}
//------------------------------------------------------------------------
std::string Mootcher::changeExtension(std::string filename)
{
	std::string aiff = ".aiff";
	std::string aif = ".aif";
	std::string wav = ".wav";
	std::string mp3 = ".mp3";
	std::string ogg = ".ogg";

	std::string replace = ".xml";
	int pos = 0;

 	pos = (int)filename.find(aiff);
	if(pos != std::string::npos) filename.replace(pos, aiff.size(), replace); 
 	pos = (int)filename.find(aif);
	if(pos != std::string::npos) filename.replace(pos, aif.size(), replace); 
 	pos = (int)filename.find(wav);
	if(pos != std::string::npos) filename.replace(pos, wav.size(), replace); 
 	pos = (int)filename.find(mp3);
	if(pos != std::string::npos) filename.replace(pos, mp3.size(), replace); 
 	pos = (int)filename.find(ogg);
	if(pos != std::string::npos) filename.replace(pos, ogg.size(), replace); 

	return filename;
}
//------------------------------------------------------------------------
void		Mootcher::GetXml(std::string ID, struct MemoryStruct &xml_page)
{
	setcUrlOptions();
	// we pass our 'xml_page' struct to the callback function
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml_page);

	if(curl)
	{
		// URL to get
		std::string getxml_url = "http://freesound.iua.upf.edu/samplesViewSingleXML.php?id=";
		getxml_url += ID;

		curl_easy_setopt(curl, CURLOPT_URL, getxml_url.c_str() );
 		
		// get it!
		CURLcode res = curl_easy_perform(curl);
		if(strcmp(curl_easy_strerror(res), "no error") == 0) connection = 1;
		else connection = 0;
	}
}
//------------------------------------------------------------------------
std::string	Mootcher::getXmlFile(std::string ID)
{
	struct MemoryStruct xml_page;
	xml_page.memory = NULL;
	xml_page.size = NULL;

	std::string xmlFileName;
	std::string audioFileName;
	std::string filename;
	
	// check if file is already downloaded
	filename += localCheck(ID.c_str());

	if(filename.empty())
	{
		if(connection != 0)
		{
			// download the xmlfile into xml_page
			GetXml(ID, xml_page);

			// if sample ID does not exist on the freesound website
			if(strcmp(xml_page.memory, "sample non existant") == 0){
				sprintf(message, "getXmlFile: sample with ID:%s does not exist!\n", ID.c_str() );
				toLog(message);
				return filename;
			}
			else
			{
				// storage for xml file
				// parse the doc, to get the original filename from the xml file, the ogg, mp3, aiff or wav file
				TiXmlDocument doc2(xmlLocation.c_str());
				doc2.Parse( xml_page.memory );

				TiXmlNode* node = 0;
				TiXmlElement *root = doc2.RootElement();
				TiXmlElement *sample = root->FirstChildElement();
				
				std::string error = root->Value();
				
				// if the page is not a valid xml document with a 'freesound' root
				if(error != "freesound"){
					sprintf(message, "searchtext: There is no valid root in the xml file");
					toLog(message);
				}
				else
				{
					// get the originalFilename from xml file
					while( node = sample->IterateChildren(node) ){
						if( strcmp( node->Value(), "originalFilename") == 0 ){
							audioFileName = node->FirstChild()->Value();
							sprintf(message, "getXmlFile: %s needs to be downloaded\n", audioFileName.c_str() );
							toLog(message);
						}
					}

					// create new filename with the ID number
					filename = basePath;
					filename += "snd\\";
					filename += sample->Attribute("id");
					filename += "-";
					filename += audioFileName;
					// change the extention into .xml
					xmlFileName = changeExtension( filename );

					sprintf(message, "getXmlFile: saveing XML: %s\n", xmlFileName.c_str() );
					toLog(message);
					
					// save the file to disk
					doc2.SaveFile(xmlFileName.c_str());

					// clear the memory
					if(xml_page.memory){
						free( xml_page.memory );
						xml_page.memory = NULL;
						xml_page.size = 0;
					}
					return xmlFileName;
				}
			}
		}
		else {
			return xmlFileName;
		}
	}
	// if the file is locally available
	return changeExtension( filename ); 
}
//------------------------------------------------------------------------
std::string	Mootcher::getProperty(std::string ID, std::string property)
{
	std::string fileName;
	std::string property_result;
	
	// check if the requested id is higher than 0
	if(atoi(ID.c_str()) > 0)
	{
		// get the xml file (if not already downloaded)
		fileName = getXmlFile(ID.c_str());

		if(!fileName.empty())
		{
			sprintf(message, "getProperty: opening: %s\n", fileName.c_str() );
			toLog(message);

			// extract the requiered property from the file's xml
			TiXmlDocument doc2(fileName.c_str());
			doc2.LoadFile(fileName.c_str());
			doc2.Parse(fileName.c_str());

			TiXmlElement *root2 = doc2.RootElement();
			TiXmlElement *sample2 = root2->FirstChildElement();

			TiXmlNode* node2 = 0;
			int size1, size2, total_size;
			total_size = size1 = size2 = 0;

			if( property == "uploader" ){
				TiXmlNode* user = sample2->FirstChild("user");
				while( node2 = user->IterateChildren(node2) ){
					if( strcmp( node2->Value(), "name") == 0 ){
						property_result = node2->FirstChild()->Value();
					}
				}
			}
			else if( property == "downloads" ){
				TiXmlNode* stats = sample2->FirstChild("statistics");
				while( node2 = stats->IterateChildren(node2) ){
					if( strcmp( node2->Value(), "downloads") == 0 ){
						property_result = node2->FirstChild()->Value();
					}
				}
			}
			else if( property == "rating" ){
				TiXmlNode* stats = sample2->FirstChild("statistics");
				while( node2 = stats->IterateChildren(node2) ){
					if( strcmp( node2->Value(), "rating") == 0 ){
						if( node2->FirstChild() != 0 ){
							property_result = node2->FirstChild()->Value(); // TODO: extract attribute
						}
					}
				}
				if(property_result.empty()) property_result = "no rating available";
			}
			else if( property == "parent" ){
				while( node2 = sample2->IterateChildren(node2) ){
					if( strcmp( node2->Value(), "parent") == 0 ){
						if( node2->FirstChild() != 0 ){
						property_result = node2->FirstChild()->Value();
						}
					}
				}
			}
			else if( property == "descriptions" ){
				TiXmlNode* descr = sample2->FirstChild("descriptions");
				while( node2 = descr->IterateChildren(node2) ){
					if( strcmp( node2->Value(), "description") == 0 ){
						property_result += node2->FirstChild("user")->FirstChild("username")->FirstChild()->Value();
						property_result += ":\n";
						property_result += node2->FirstChild("text")->FirstChild()->Value();
						property_result += "\n";
					}
				}
			}
			else if( property == "comments" ){
				TiXmlNode* comment = sample2->FirstChild("comments");
				while( node2 = comment->IterateChildren(node2) ){
					if( strcmp( node2->Value(), "comment") == 0 ){
						property_result += node2->FirstChild("user")->FirstChild("username")->FirstChild()->Value();
						property_result += ":\n";
						property_result += node2->FirstChild("text")->FirstChild()->Value();
						property_result += "\n";
					}
				}
				if(property_result.empty()) property_result = "no comments available";
			}
			else
			{
				while( node2 = sample2->IterateChildren(node2) ){
					if( property == node2->Value() ){
						property_result = node2->FirstChild()->Value();
					}
				}
			}
			if(property_result.empty()){
				property_result = "no valid tag";
			}
			return property_result;
		}
		else 
		{
			property_result = "data not available";
			return property_result;
		}
	}
	else
	{
		property_result = "requested ID must be higher than 0";
		return property_result;
	}

}
//------------------------------------------------------------------------
std::string	Mootcher::localCheck(std::string ID)	
{
	// check if the given ID already exists in freesound.xml
	std::string filename;
	TiXmlDocument *doc1 = NULL;
	doc1 = new TiXmlDocument(xmlLocation.c_str());
	//TiXmlDocument doc1("freesound.xml");//xmlLocation.c_str()

	bool xmlError = doc1->LoadFile(xmlLocation.c_str());
	
	doc1->Parse(xmlLocation.c_str());

	// parse the created DOM
	TiXmlElement *root1 = doc1->RootElement();
	TiXmlElement *sample1 = 0;
	TiXmlNode *node1 = 0;

	while( node1 = root1->IterateChildren(node1) ){
		if( strcmp( node1->Value(), "sample") == 0 ){
			sample1 = (TiXmlElement *) node1;
			if(ID == sample1->Attribute("id") ){
				filename = basePath; 
				filename += sample1->FirstChild()->Value();
				sprintf(message, "localCheck: sound %s, is already downloaded\n", filename.c_str() );
				toLog(message);
				return filename;
			}
		}
	}
	filename = "";
	return filename;
}
//------------------------------------------------------------------------
std::string	Mootcher::getFile(std::string ID)
{
	struct MemoryStruct xml_page3;
	xml_page3.memory = NULL;
	xml_page3.size = NULL;

	CURLcode result_curl;

	struct OnlineFile Myfile;
	Myfile.stream = NULL;
	
	std::string audioFileName;
	std::string audioFileNameSolo;
	std::string xmlFileName;

	// check if audio and xmlfile exists locally
	// check if the given ID already exists in freesound.xml
	TiXmlDocument doc1(xmlLocation.c_str());
	
	doc1.LoadFile();
	doc1.Parse(xmlLocation.c_str());

	// parse the created DOM
	TiXmlElement *root1 = doc1.RootElement();
	TiXmlElement *sample1 = 0;
	TiXmlNode *node1 = 0;

	while( node1 = root1->IterateChildren(node1) ){
		if( strcmp( node1->Value(), "sample") == 0 ){
			sample1 = (TiXmlElement *) node1;
			if( ID == sample1->Attribute("id") ){
				audioFileName = sample1->FirstChild()->Value();
				sprintf(message, "localCheck: sound %s, is already downloaded\n", audioFileName.c_str() );
				toLog(message);
			}
		}
	}
	
	if(audioFileName.empty())
	{
		if(connection != 0)
		{
			// download the xml page into 'xml_page3'
			GetXml(ID, xml_page3);

			// Create a DOM from the file we've downloaded in the struct
			TiXmlDocument doc(xmlLocation.c_str());
			doc.Parse( xml_page3.memory );

			if(strcmp(xml_page3.memory, "sample non existant") == 0){
				sprintf(message, "getFile: sample with ID:%s does not exist!\n", ID.c_str());
				toLog(message);
			}
			else
			{
				// storage for audio file
				// parse the doc get a new url from xml file for  the ogg, mp3, aiff or wav file */
				sprintf(message, "getFile: sound with id:%s needs to be downloaded\n", ID.c_str() );
				toLog(message);

				// create new filename with the ID number
				audioFileName += basePath;
				audioFileName += "snd\\";
				audioFileName += ID;
				audioFileName += "-";
				
				audioFileNameSolo = "snd\\"; // for storeing in the freesound.xml
				audioFileNameSolo += ID;
				audioFileNameSolo += "-";

				// get the original audio filename from xml file 
				TiXmlNode* node = 0;
				TiXmlElement *root = doc.RootElement();
				TiXmlElement *sample = root->FirstChildElement();
				while( node = sample->IterateChildren(node) ){
					if( strcmp( node->Value(), "originalFilename") == 0 ){
						audioFileName += node->FirstChild()->Value();
						audioFileNameSolo += node->FirstChild()->Value();
					}
				}

				// save the name in the struct
				Myfile.filename = new char[audioFileName.size()+1];
				strcpy(Myfile.filename, audioFileName.c_str() );

				// save the XML document of the particular soundfile
				xmlFileName = changeExtension(audioFileName);
				sprintf(message, "getFile: saveing file: %s\n", xmlFileName.c_str() );
				toLog(message);
				doc.SaveFile(xmlFileName.c_str());

				// create the download url, this url will also update the download statics on the site
				std::string audioURL;
				audioURL += "http://freesound.iua.upf.edu/samplesDownload.php?id=";
				audioURL += ID;

				// specify URL to get
				curl_easy_setopt(curl, CURLOPT_URL, audioURL.c_str() );
				// Define our callback to get called when there's data to be written
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, audioFileWrite);
				// Set a pointer to our struct to pass to the callback
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &Myfile);
			
				bar.dlnowMoo = 0;
				bar.dltotalMoo = 0;
		curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 0); // turn on the process bar thingy
		curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
		curl_easy_setopt (curl, CURLOPT_PROGRESSDATA, &bar);


				sprintf(message, "getFile: ** Starting download: %s ** \n", audioURL.c_str());
				toLog(message);
				
				// preform download
				result_curl = curl_easy_perform(curl);
				if(strcmp(curl_easy_strerror(result_curl), "no error") == 0) connection = 1;
				else connection = 0;

			/*	float dlSpeed;
				curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &dlSpeed);
				sprintf(message, "getFile: download of %s done! With average speed: %f\n", audioFileName.c_str(), dlSpeed );
				toLog(message);
			*/
				if(Myfile.stream)
				{
					// close the downloaded file
					// clear the memory where the audio file was saved
					fclose(Myfile.stream);
					delete[] Myfile.filename;
					Myfile.filename = NULL;
					
					// The file is downloaded, so save the xmlFileName in a tag in the freesound.xml 
					TiXmlDocument doc2(xmlLocation.c_str());
					doc2.LoadFile(xmlLocation.c_str());
					doc2.Parse(xmlLocation.c_str());
					// i.e. <sample id="0">snd\name.wav</sample>
					TiXmlElement *root2 = doc2.RootElement();
					TiXmlElement sample( "sample" );
					sample.SetAttribute( "id", ID.c_str() );
					
					TiXmlText text( audioFileNameSolo.c_str() );
					sample.InsertEndChild( text );
					root2->InsertEndChild( sample );
					doc2.SaveFile(xmlLocation.c_str());
				}
				
				// clear the memory where the xml file was saved
				if(xml_page3.memory){
					free( xml_page3.memory );
					xml_page3.memory = NULL;
					xml_page3.size = 0;
				}
			}
		}
		else
		{
			// if file is not local and there is no connection with the internet
			audioFileName = "";
		}
	}
	return audioFileName;
}
//---------
int Mootcher::progress_callback(void *bar, double dltotal, double dlnow, double ultotal, double ulnow)
{
	struct dlprocess *lbar = (struct dlprocess *) bar;
	lbar->dltotalMoo = dltotal;
	lbar->dlnowMoo = dlnow;
	return 0;
}

//------------------------------------------------------------------------
int			Mootcher::deleteFile(std::string ID)
{
	// check if file ID exists in freesound.xml and get the filename
	TiXmlDocument doc1(xmlLocation.c_str());
	int Dcheck = 0;
	std::string filename;
	filename = basePath;
	
	doc1.LoadFile();
	doc1.Parse(xmlLocation.c_str());

	TiXmlElement *root1 = doc1.RootElement();
	TiXmlElement *sample1 = 0;
	TiXmlNode *node1 = 0;

	while( node1 = root1->IterateChildren(node1) ){
		if( strcmp( node1->Value(), "sample") == 0 ){
			sample1 = (TiXmlElement *) node1;
			if(ID == sample1->Attribute("id")){
				filename += sample1->FirstChild()->Value();
				sprintf(message, "\nSound:\"%s\", exists on HardDrive\n", filename.c_str());
				toLog(message);
				Dcheck = 1;
			}
		}
	}

	if(Dcheck == 1)
	{
	// delete the soundfile
		if( remove( filename.c_str() ) == -1 ){
			sprintf(message, "Error deleting file: %s\n", filename.c_str() );
			toLog(message);
		}
		sprintf(message, "%s successfully deleted\n", filename.c_str() );
		toLog(message);
		
		// check if aiff, change the extention and delete xml file
		std::string name = changeExtension(filename);
		
		// delete the xml file
		if( remove( name.c_str() ) == -1 ){
			sprintf(message, "deletefile: Error deleting file: %s\n", name.c_str() );
			toLog(message);
		}
		else {
			sprintf(message, "deletefile: %s successfully deleted\n", name.c_str() );
			toLog(message);
		}
		// remove the ID node in freesound.xml
		root1 = doc1.RootElement();
		sample1 = 0;
		node1 = 0;

		while( node1 = root1->IterateChildren(node1) ){
			if( strcmp( node1->Value(), "sample") == 0 ){
				sample1 = (TiXmlElement *) node1;
				if(ID == sample1->Attribute("id")){
					root1->RemoveChild(node1);
					doc1.SaveFile();

					sprintf(message, "deletefile: Node with ID:%s removed from freesound.xml\n", ID.c_str() );
					toLog(message);
					return 0;
				}
			}
			else break;
		}
	}
	else {
		sprintf(message, "delete: file with id:%s does not exist on the Hard Disk\n", ID.c_str() );
		toLog(message);
		return -1;
	}
	return -2;
}
//------------------------------------------------------------------------
int			Mootcher::deleteAll()
{
	localFilesTotal();
	int size= DBsize;
	if(size <= 0) return -1;
	else {
		for(int i=1; i<=size; i++){
			Mootcher::deleteFile( Mootcher::myLocalFile(1) );
		}
		return 0;
	}
}
//------------------------------------------------------------------------
float *		Mootcher::openFile(std::string ID, int &sizeInFrames)
{
	float *openFilePointer = NULL;
	struct OnlineFile localFile;
	localFile.filename = NULL;
	localFile.stream = NULL;
	
	// makes sure the audio- and xmlfile is downloaded
	std::string audioFilename;
	audioFilename = localCheck(ID.c_str());
	localFile.filename = new char[audioFilename.size()+1];
	strcpy(localFile.filename, audioFilename.c_str() );

	// check if sound file is already 'opened' in allSounds
	int i = 0;
	for( std::vector <int>::iterator allSoundsIterator = allSoundsID.begin(); 
			allSoundsIterator != allSoundsID.end();
			allSoundsIterator++)
	{
		if(*allSoundsIterator == atol(ID.c_str())){
			sprintf(message, "Open sound: sound with ID: %s is already loaded\n", ID.c_str());
			toLog(message);
			return allSoundsP[i];
		}
		i++;
	}
	
	if(!audioFilename.empty())
	{
		// theFreesoundProject contains samples in 4 formats: wav, aiff, mp3 & ogg.
		// The experimental libSnd.dll that I use can open all 4 types. the official one cannot open mp3 & ogg files.
		SNDFILE *soundFile = NULL;
		SF_INFO *soundFileInfo = NULL;

		soundFileInfo = (SF_INFO *) malloc(sizeof(SF_INFO));
		
		// use extracted filename & open file
		if( !(soundFile = sf_open(localFile.filename, SFM_READ, soundFileInfo) )){
			sprintf(message, "Open sound: Error opening file\n");
			toLog(message);
		}
		// create an array of "frames * channels" big
		openFilePointer = new float[(int)(soundFileInfo->frames * soundFileInfo->channels)];
		
		// open the file
		sf_count_t readSamples = sf_readf_float(soundFile, openFilePointer, soundFileInfo->frames);
		// tell outside how many samples are available
		sizeInFrames = (int)readSamples;
		
		opensounds++;
		allSoundsP.push_back(openFilePointer);
		allSoundsID.push_back( atol(ID.c_str()) );
		allSoundsS.push_back(sizeInFrames);

		// clear the memory
		sf_close(soundFile);
		free(soundFileInfo);
	
		// return pointer to memory with audiofile
		return openFilePointer;
	}
	else {
		sprintf(message, "Open sound: please, download file with id:%s before opening. Please login.\n", ID.c_str() );
		toLog(message);
		return openFilePointer;
	}
}
//------------------------------------------------------------------------
int			Mootcher::closeFile(std::string ID)
{
	// search for the ID in 'allSoundsID'
	int i = 0;
	for( std::vector <int>::iterator allSoundsIterator = allSoundsID.begin(); 
			allSoundsIterator != allSoundsID.end();
			allSoundsIterator++)
	{
		sprintf(message, "closeFile : allSoundsID[%d] = %d\n", i, allSoundsID[i]);
		toLog(message);
		// check if sound file is 'opened' in allSounds
		if(allSoundsID[i] == atol(ID.c_str()))
		{
			sprintf(message, "closeFile: removing sound %d with ID:%s from internal memory\n",i , ID.c_str());
			toLog(message);
			delete allSoundsP[i];
			allSoundsP[i] = NULL;
			allSoundsP.erase(allSoundsP.begin()+i, allSoundsP.begin()+i+1);
			allSoundsID.erase(allSoundsID.begin()+i, allSoundsID.begin()+i+1);
			allSoundsS.erase(allSoundsS.begin()+i, allSoundsS.begin()+i+1);
			opensounds--;
			return 0;
		}
		i++;
	}
	sprintf(message, "closeFile: sound with ID:%s not found in internal memory\n", ID.c_str());
	toLog(message);
	return -1;
}
