/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

#include <iostream>
#include <algorithm>
#include "httplib.h"
#include <unordered_map>

using namespace httplib;
Server m_svr;
const bool istest = true;

std::mutex _mtx_Checkversion;
std::mutex _mtx_RWFile;

std::mutex _mtx_access;

struct AccessInfo
{
	AccessInfo()
	{
		lastCallTime = juce::Time::getMillisecondCounterHiRes();
	}

	AccessInfo(double t)
	{
		lastCallTime = t;
	}
	// miliseconds
	double lastCallTime;

 
};
std::unordered_map<std::string, AccessInfo>  callmap;


auto JSONHelper =
[](juce::var& object, juce::String key, juce::var value) {
	if (object.isObject()) {
		object.getDynamicObject()->setProperty(key, value);
	}
};


void setError(Response& res, String error)
{
	var errObj(new DynamicObject());
	JSONHelper(errObj, "error", error);
	res.status = 400;
	res.set_content(JSON::toString(errObj).toStdString(), "application/json; charset=utf-8");
	//std::cout << error << std::endl;
}

void setSuccess(Response& res, String error)
{
	var errObj(new DynamicObject());
	JSONHelper(errObj, "success", error);
	res.set_content(JSON::toString(errObj).toStdString(), "application/json; charset=utf-8");
	//std::cout << error << std::endl;
}

void log(const char * s)
{
	std::cout << s << std::endl;
}
void logerror(const char * s)
{
	std::cout << "error: " << s << std::endl;
}




bool restrictAccess(const Request& req, Response& res)
{
	std::lock_guard<std::mutex> lockGuard(_mtx_access);

	auto curtime = juce::Time::getMillisecondCounterHiRes();
	std::string remote_addr;
	if (req.has_header("REMOTE_ADDR"))
	{
		remote_addr = req.get_header_value("REMOTE_ADDR");
		const auto& it = callmap.find(remote_addr);
		if (it != callmap.end())
		{
			if (curtime - it->second.lastCallTime < 3000)
			{
				it->second.lastCallTime = curtime;
				setError(res, "Server is busy.");
				return false;
			}
			else
			{
				it->second.lastCallTime = curtime;
			}

		}
		else
		{
			callmap.emplace(std::pair<std::string, AccessInfo>(remote_addr, { curtime }));
		}

	}
	return true;
}



// singleton
class FileDB
{
public:

	enum FindEnum
	{
		SUCCESS,
		TREE_INVALID,
		NOT_FIND_NAME,
		INVALID_PASSWORD,

	};
public:
	FileDB() : _facc(File::getCurrentWorkingDirectory().getChildFile("account.txt")), _inAccStream(_facc)
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_db);
	 
		if (!_facc.existsAsFile())
		{
			logerror("Account is not exist");
		}
		else
		{
			_actree =  ValueTree::readFromStream(_inAccStream);
			if (_actree.getType().toString() == "account")
			{
				log("Account load successfully!");
				
			}
			else
			{
				logerror("Can not load account!");			
					
			}
		}
	}


	static FileDB& ins()
	{
		static FileDB fdb;
		return fdb;
	}

	void init()
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_db);
		ValueTree init("account");
		//ValueTree c("adminbz");
		//c.setProperty("pwd", "17", nullptr);
		//init.addChild(c, -1, nullptr);

		FileOutputStream os(_facc);
		if (os.openedOk())
		{
			os.setPosition(0);
			os.truncate();
			init.writeToStream(os);
			os.flush();
		}
	}

	


	FindEnum getUserPwdByName(const String& name, String & pwd)
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_db);
		auto nameId = Identifier(name);
		if (_actree.isValid())
		{
			bool find = false;
			for (auto i = 0; i < _actree.getNumChildren(); i++)
			{
				Identifier curNameIdenty = _actree.getChild(i).getType();
			/*	log("nameId");
				log(nameId.toString().getCharPointer());
				log("_actree.getChild(i)");
				log(_actree.getChild(i).getType().toString().getCharPointer());*/
				if (curNameIdenty == nameId)
				{
					//log("find");
					pwd = _actree.getChild(i).getProperty("pwd").toString();
					
					find = true;
					break;
				}
				/*else
					log("no find");*/
			}
			if (!find)
				return NOT_FIND_NAME;
			else
			{
				if (pwd.length() > 3)
				{
					return SUCCESS;
				}
				else 
					return INVALID_PASSWORD;
			}
				

		}
		else
			return TREE_INVALID;
		 
	}


	bool isUserExist(const String& name)
	{
		std::lock_guard<std::mutex> lockGuard(_mtx_db);

		auto nameId = Identifier(name);
		if (_actree.isValid())
		{
			for (auto i = 0; i < _actree.getNumChildren(); i++)
			{
				Identifier curNameIdenty = _actree.getChild(i).getType();
				if (curNameIdenty == nameId)
				{
					return true;
				}
			}
		}
		
		return false;
	}



	// must check if the user is exist first!
	bool insertUser(const String &name, const  String&pwd, const String &email)
	{

		std::lock_guard<std::mutex> lockGuard1(_mtx_db);
		if (_actree.isValid())
		{
			ValueTree c(name);
			c.setProperty("pwd", pwd, nullptr);
			c.setProperty("email", email, nullptr);
			_actree.addChild(c, -1, nullptr);

			FileOutputStream os(_facc);
			if (os.openedOk())
			{
				os.setPosition(0);
				os.truncate();
				_actree.writeToStream(os);
				os.flush();
				return true;
			}

		}
		return false;
		
	}


	

	void getAllAcount(String & s)
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_db);
		s = _actree.toXmlString();
	}




	File _facc;
	FileInputStream _inAccStream;
	ValueTree _actree;
	std::mutex _mtx_db;
};




// singleton
class SheetDB
{
public:

	enum FindEnum
	{
		SUCCESS,
		TREE_INVALID,
		NOT_FIND_NAME,
		INVALID_PASSWORD,

	};
public:
	SheetDB() : _facc(File::getCurrentWorkingDirectory().getChildFile("sheet.txt")), _inAccStream(_facc)
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_sheet);

		if (!_facc.existsAsFile())
		{
			logerror("Sheet is not exist");
		}
		else
		{
			_sheettree = ValueTree::readFromStream(_inAccStream);
			if (_sheettree.getType().toString() == "sheet")
			{
				log("Sheet load successfully!");

			}
			else
			{
				logerror("Can not load Sheet!");
			}
		}
	}


	static SheetDB& ins()
	{
		static SheetDB fdb;
		return fdb;
	}

 
	void init()
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_sheet);
		ValueTree init("sheet");
		//ValueTree c("adminbz");
		//c.setProperty("pwd", "17", nullptr);
		//init.addChild(c, -1, nullptr);

		FileOutputStream os(_facc);
		if (os.openedOk())
		{
			os.setPosition(0);
			os.truncate();
			init.writeToStream(os);
			os.flush();
		}
	}



	FindEnum getSheetByTitle(const String& title, String & sheet, String &author)
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_sheet);
		auto titleid = Identifier(title);
		if (_sheettree.isValid())
		{
			bool find = false;
			for (auto i = 0; i < _sheettree.getNumChildren(); i++)
			{
				Identifier curNameIdenty = _sheettree.getChild(i).getType();
				if (curNameIdenty == titleid)
				{
					sheet = _sheettree.getChild(i).getProperty("sheet").toString();
					author = _sheettree.getChild(i).getProperty("author").toString();

					find = true;
					break;
				}
			}
			if (!find)
				return NOT_FIND_NAME;
			else
			{
				if (sheet.length() > 3)
				{
					return SUCCESS;
				}
				else
					return INVALID_PASSWORD;
			}


		}
		else
			return TREE_INVALID;

	}


	//bool isSheetExist(const String& title)
	//{
	//	std::lock_guard<std::mutex> lockGuard(_mtx_sheet);

	//	auto nameId = Identifier(title);
	//	if (_sheettree.isValid())
	//	{
	//		for (auto i = 0; i < _sheettree.getNumChildren(); i++)
	//		{
	//			Identifier curNameIdenty = _sheettree.getChild(i).getType();
	//			if (curNameIdenty == nameId)
	//			{
	//				return true;
	//			}
	//		}
	//	}

	//	return false;
	//}



	// must check if the user is exist first!
	bool insertSheet(const String &title, const  String&author, const String &info)
	{
		std::lock_guard<std::mutex> lockGuard(_mtx_sheet);
		bool isExist = false;
		{
			auto nameId = Identifier(title);
			if (_sheettree.isValid())
			{
				for (auto i = 0; i < _sheettree.getNumChildren(); i++)
				{
					Identifier curNameIdenty = _sheettree.getChild(i).getType();
					if (curNameIdenty == nameId)
					{
						isExist =  true;
						auto curChild = _sheettree.getChild(i);
						curChild.setProperty("author", author, nullptr);
						curChild.setProperty("info", info, nullptr);

						FileOutputStream os(_facc);
						if (os.openedOk())
						{
							os.setPosition(0);
							os.truncate();
							_sheettree.writeToStream(os);
							os.flush();
							return true;
						}

						break;
					}
				}
			}
		}
		if (!isExist)
		{
			if (_sheettree.isValid())
			{
				ValueTree c(title);
				c.setProperty("author", author, nullptr);
			 
				c.setProperty("info", info, nullptr);
				_sheettree.addChild(c, -1, nullptr);

				FileOutputStream os(_facc);
				if (os.openedOk())
				{
					os.setPosition(0);
					os.truncate();
					_sheettree.writeToStream(os);
					os.flush();
					return true;
				}

			}

		}

		return false;

	}


	size_t getAllSheet(MemoryBlock &mb)
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_sheet);
		_inAccStream.setPosition(0);
		size_t l = _inAccStream.readIntoMemoryBlock(mb);
		return l;
	}


	String getAllSheetToString()
	{
		std::lock_guard<std::mutex> lockGuard1(_mtx_sheet);
		return _sheettree.toXmlString();
	}


	File _facc;
	FileInputStream _inAccStream;
	ValueTree _sheettree;
	std::mutex _mtx_sheet;
};


// the efficient is extreamly slow! need rewrite!
String MemroyToHexString(uint8 * data, size_t len) //convertBuf's length is double of the data length.
{
	String sp;
	for (int i = 0; i < len; i++)
	{
		String si = String::toHexString(*(data + i)).toUpperCase();
		if (si.length() == 1)
			si = "0" + si;
		sp += si;
	}
	return sp;
}




//==============================================================================
int main (int argc, char* argv[])
{
	if (argc > 1)
	{
		if (strcmp(argv[1],  "init") == 0)
		{
			log("init account");
			FileDB::ins().init();
		}
		else 
			if (strcmp(argv[1], "initsheet") == 0)
			{
				log("init account");
				SheetDB::ins().init();
			}
	}
	//String pwd;
	//String name = "adminbz";
	//FileDB::ins().getUserPwdByName(name, pwd);
    
	std::thread t([]() {


		m_svr.Get("/allacc", [](const Request& req, Response& res) {

			String acctxt;
			FileDB::ins().getAllAcount(acctxt);
			res.set_content(acctxt.toStdString(), "text/plain");
		
		});

		m_svr.Get("/allsheet", [](const Request& req, Response& res) {

			String acctxt;
			String ss = SheetDB::ins().getAllSheetToString();
			res.set_content(ss.toStdString(), "text/plain");

		});

		m_svr.Get("/upsheet", [](const Request& req, Response& res) {
			auto h = R"(
						<!DOCTYPE html>
						<html lang="en">
						<head>
							<meta charset="UTF-8">
							<title>Upload sheet</title>
						</head>
						<body>
 
						<h1>Please upload the Version File!</h1>
						<form action="/_upsheet" method="post" enctype="multipart/form-data">


							<label for="title"><b>title</b></label>
							<input type="text" placeholder="1" name="title" required>

							<br/>
	
							<label for="author"><b>author</b></label>
							<input type="text" placeholder="1" name="author" required>

							<br/>


							<p>Sheet Descirbe File:<input type="file" name="upload"></p>	<br/>
							<p>Texture:<input type="file" name="tex"> </p>		<br/>

							<p>Particle Frag:<input type="file" name="frag"> </p>		<br/>
							<p>Particle Vetex:<input type="file" name="vetex"> </p>		<br/>

							<p>Staff line Frag:<input type="file" name="fragStaff"> </p>		<br/>
							<p>Staff line Vetex:<input type="file" name="vetexStaff"> </p>		<br/>


			
							<p><input type="submit" value="upload"></p>	<br/>


								<textarea cols="200" rows="40" name="info">

additional information

								</textarea>
						</form>
 
						</body>
						</html>
				)";
			res.set_content(h, "text/html");
		});


	

		m_svr.Get("/upapp", [](const Request& req, Response& res) {
			auto h = R"(
						<!DOCTYPE html>
						<html lang="en">
						<head>
							<meta charset="UTF-8">
							<title>Up load APP</title>
						</head>
						<body>
 
						<h1>Please upload the Version File!</h1>
						<form action="/_upload" method="post" enctype="multipart/form-data">

							<input type="radio" name="systemtype" value="win" ><label>win</label><br/>
							<input type="radio" name="systemtype" value="osx" ><label>osx</label><br/>

							<br/>

							<label for="Major"><b>Major</b></label>
							<input type="text" placeholder="1" name="Major" required>

							<br/>
	
							<label for="Minor"><b>Minor</b></label>
							<input type="text" placeholder="1" name="Minor" required>

							<br/>


							<p><input type="file" name="upload"></p>
							<p><input type="submit" value="upload"></p>


								<textarea cols="200" rows="40" name="notes">

Bug Fixes
1,Players impulsed by a Shockwave Grenade will no longer destroy a nearby trap without first destroying the building piece it is attached to. 

2,The Zapper Trap's info card now shows the appropriate 4 stars (indicating Epic Rarity) rather than 2 (indicating Uncommon Rarity).

3,Resolved an issue in which the Zapper Trap would not build properly when thrown on slopes or uneven terrain.

4,B.R.U.T.E.s will now be launched away if they touch the Floating Island's Cube.

5,The B.R.U.T.E.'s Stomp attacks now deal consistent damage to other vehicles. 

6,Previously, they would deal either double or triple damage to other vehicles.

7,The color of the B.R.U.T.E.'s cooldown meter now updates properly to correspond with the cooldown value.

									</textarea>
						</form>
 
						</body>
						</html>
				)";
			res.set_content(h, "text/html");
		});

		m_svr.Post("/_upload", [](const Request& req, Response& res) {
			log("wait _upload");
			std::lock_guard<std::mutex> lockGuard1(_mtx_RWFile);
			std::lock_guard<std::mutex> lockGuard2(_mtx_Checkversion);
			log("enter _upload");
			String ostype = "";
			
			if (req.has_file("systemtype"))
			{
				const auto& systemtype = req.get_file_value("systemtype");
				ostype = req.body.substr(systemtype.offset, systemtype.length);

			}
			else
			{
				setError(res, "Please set the OS type");
				return;
			}

			String notes;
			if (req.has_file("notes"))
			{
				const auto& _notes = req.get_file_value("notes");
				notes = req.body.substr(_notes.offset, _notes.length);
			}
			else
			{
				setError(res, "Please write some note about the update!");
				return;
			}
		
			int imajor, iminor;
			if (req.has_file("Major") && req.has_file("Minor"))
			{
				const auto& major = req.get_file_value("Major");
				String smajor = req.body.substr(major.offset, major.length);
				imajor = smajor.getIntValue();

				const auto& minor = req.get_file_value("Minor");
				String sminor = req.body.substr(minor.offset, minor.length);
				iminor = sminor.getIntValue();

			}
			else
			{
				setError(res, "Please set the Major and Minor version");
				return;
			}


			File fapp;
			if (req.has_file("upload"))
			{
				const auto& file = req.get_file_value("upload");

				String fileName = file.filename;
				/*juce::MemoryBlock uploadBlock;
				uploadBlock.replaceWith(req.body.c_str() + file.offset, file.length);*/
		 
					fapp = File::getCurrentWorkingDirectory().getChildFile(ostype);
					if (!fapp.exists())
					{
						fapp.createDirectory();
					}
					if (fapp.isDirectory())
					{
						String sversion = String(imajor) + "." + String(iminor);

						
						auto appfile = fapp.getChildFile(fileName);
						auto fwe = appfile.getFileNameWithoutExtension();
						if (!fwe.contains(sversion))
						{
							setError(res, "App name do not contains version string. version: " + sversion + " appname: " + fileName + "!");						 
							return;
						}
						else
						{
							bool b = appfile.replaceWithData(req.body.c_str() + file.offset, file.length);
							if (!b)
							{
								setError(res, "Failed to write app bin to disk !");
								return;
							}

							auto notesfile = fapp.getChildFile("notes.txt");
							b = notesfile.replaceWithText(notes);
							if (!b)
							{
								setError(res, "Failed to write notes file !");
								return;
							}
							auto versionFile = fapp.getChildFile("version.txt");

							unsigned int v = imajor << 16 | iminor;
							if (!versionFile.replaceWithText(String(v)))
							{
								setError(res, "Failed to write notes file !");
								return;
							}

							auto nameFile = fapp.getChildFile("name.txt");
							if (!nameFile.replaceWithText(fileName))
							{
								setError(res, "Failed to write name file !");
								return;
							}
						}
						
					}
					else
					{
						setError(res , "Can not find app dir " + ostype + " !");
						return;
					}
			}
			else
			{
				 
				setError(res, "Please upload the new version of app!");
				return;
			}


			var resObj(new DynamicObject());
			JSONHelper(resObj, "Major", String(imajor));
			JSONHelper(resObj, "Minor", String(iminor));
			JSONHelper(resObj, "systemtype", ostype);
			JSONHelper(resObj, "notes", notes);
			
			res.set_content(JSON::toString(resObj).toStdString(), "application/json; charset=utf-8");
			return;
		});


		m_svr.Post("/_upsheet", [](const Request& req, Response& res) {

			String info;
			if (req.has_file("info"))
			{
				const auto& _info = req.get_file_value("info");
				info = req.body.substr(_info.offset, _info.length);
			}
			else
			{
				setError(res, "Please upload sheet!");
				return;
			}

			String stitle, sauthor, sinfo, sfrag, svetex, sfragStaff, svetexStaff;
			if (req.has_file("title") && req.has_file("author") && req.has_file("info") && req.has_file("frag") && req.has_file("vetex")
				&& req.has_file("fragStaff") && req.has_file("vetexStaff")
				)
			{
				const auto& title = req.get_file_value("title");
				stitle = req.body.substr(title.offset, title.length);

				const auto& author = req.get_file_value("author");
				sauthor = req.body.substr(author.offset, author.length);		

				const auto& info = req.get_file_value("info");
				sinfo = req.body.substr(info.offset, info.length);

				const auto& frag = req.get_file_value("frag");
				sfrag = req.body.substr(frag.offset, frag.length);

				const auto& vetex = req.get_file_value("vetex");
				svetex = req.body.substr(vetex.offset, vetex.length);

				const auto& fragStaff = req.get_file_value("fragStaff");
				sfragStaff = req.body.substr(fragStaff.offset, fragStaff.length);

				const auto& vetexStaff = req.get_file_value("vetexStaff");
				svetexStaff = req.body.substr(vetexStaff.offset, vetexStaff.length);


			}
			else
			{
				setError(res, "Please set the title  author and info of the sheet.");
				return;
			}


			File fapp;
			if (req.has_file("upload") && req.has_file("tex"))
			{
				const auto& file = req.get_file_value("upload");
				const auto& filetex = req.get_file_value("tex");

				const String fileName = file.filename;
				const String texfileName = filetex.filename;
	

				fapp = File::getCurrentWorkingDirectory();
				if (!fapp.exists())
				{
					setError(res, "Current dir is not exist?");
					return;
				}
				if (fapp.isDirectory())
				{
					const String sheet = String(req.body.c_str() + file.offset, file.length);



					juce::MemoryBlock textureBlock;
					textureBlock.replaceWith(req.body.c_str() + filetex.offset, filetex.length);


					auto dirOfTexture = File::getCurrentWorkingDirectory().getChildFile("tex");
					if (!dirOfTexture.isDirectory())
					{
						setError(res, "Please create the tex dir.");
						return;
					}
					{
						// create the texture named as title.png
						File texFileToWrite = fapp.getChildFile("tex").getChildFile( stitle +".png");
						if (texFileToWrite.existsAsFile())
						{
							texFileToWrite.deleteFile();
						}
						if (texFileToWrite.existsAsFile())
						{
							setError(res,   " can not delete " + texFileToWrite.getFullPathName());
							return;
						}
				
						FileOutputStream os(texFileToWrite);

						if (os.openedOk())
						{
							os.setPosition(0);
							os.truncate();
							os.write(textureBlock.getData(), textureBlock.getSize());
							os.flush();

						}
						else
						{
							setError(res, "Failed to write texture to disk : " + stitle + texfileName);
							return;
						}
					}



					{
						// we write the sheet describe file to the disk.
						File describeFileToWrite = fapp.getChildFile("tex").getChildFile(stitle + ".txt");
						if (describeFileToWrite.existsAsFile())
						{
							describeFileToWrite.deleteFile();
						}
						if (describeFileToWrite.existsAsFile())
						{
							setError(res, " can not delete " + describeFileToWrite.getFullPathName());
							return;
						}

						FileOutputStream os(describeFileToWrite);

						if (os.openedOk())
						{
							os.setPosition(0);
							os.truncate();
							os.write(sheet.getCharPointer(), sheet.length());
							/*os.write(textureBlock.getData(), textureBlock.getSize());*/
							os.flush();

						}
						else
						{
							setError(res, "Failed to write texture to disk : " + describeFileToWrite.getFullPathName());
							return;
						}
					}
					
					{
						ValueTree sheetBin(stitle);  // contain memoryblock of png and string of sheet describe.
						sheetBin.setProperty("png", textureBlock, nullptr);
						sheetBin.setProperty("sheet", sheet, nullptr);
						sheetBin.setProperty("frag", sfrag, nullptr);
						sheetBin.setProperty("vetex", svetex, nullptr);
						sheetBin.setProperty("fragStaff", sfragStaff, nullptr);
						sheetBin.setProperty("vetexStaff", svetexStaff, nullptr);

						File sheetDescribePngBin = fapp.getChildFile("tex").getChildFile(stitle + ".bin");
						if (sheetDescribePngBin.existsAsFile())
						{
							sheetDescribePngBin.deleteFile();
						}
						if (sheetDescribePngBin.existsAsFile())
						{
							setError(res, " can not delete " + sheetDescribePngBin.getFullPathName());
							return;
						}
						FileOutputStream os(sheetDescribePngBin);
						if (os.openedOk())
						{
							os.setPosition(0);
							os.truncate();
							sheetBin.writeToStream(os);
							os.flush();
						}

					}

					if (!SheetDB::ins().insertSheet(stitle, sauthor, sinfo))
					{
						setError(res, "Failed to write to sheet DBB !");
						return;
					}

				}
				else
				{
					setError(res, "Current dir is a file not a dir?");
					return;
				}
			}
			else
			{

				setError(res, "Please upload the sheet and the texture!");
				return;
			}


			var resObj(new DynamicObject());
			JSONHelper(resObj, "success", "write success to sheet DB");
			res.set_content(JSON::toString(resObj).toStdString(), "application/json; charset=utf-8");
			return;
		});

	


		m_svr.Get("/checkver", [](const Request& req, Response& res) {
			log("wait checkver");
			std::lock_guard<std::mutex> lockGuard(_mtx_Checkversion);
			log("enter checkver");
			if (req.has_param("os") && req.has_param("currentversion"))
			{


				String s_ostype = req.get_param_value("os");
				String s_version = req.get_param_value("currentversion");
			

				if (s_ostype != "osx"  && s_ostype != "win" && s_ostype != "linux")
				{
					setError(res, s_ostype + " is not supproted!");
					return;
				}


				File versionf = File::getCurrentWorkingDirectory().getChildFile(s_ostype).getChildFile("version.txt");
				if (!versionf.existsAsFile())
				{
					setError(res, "Can not get version!");
					return;
				}
				unsigned int version = versionf.loadFileAsString().getIntValue();
			


				File notef = File::getCurrentWorkingDirectory().getChildFile(s_ostype).getChildFile("notes.txt");
				if (!notef.existsAsFile())
				{
					setError(res, "Can not get note!");
					return;
				}
				String notes = notef.loadFileAsString();


				var errObj(new DynamicObject());
				JSONHelper(errObj, "version", String(version));
				JSONHelper(errObj, "notes", notes);

				res.set_redirect("/download");
				res.set_content(JSON::toString(errObj).toStdString(), "application/json; charset=utf-8");

			}
			else
			{
 
				setError(res, "Wrong argument for getetversion api.");
		 
			}

		});
	
		m_svr.Get("/downloadsheetbytitle", [&](const Request &req, Response &res) {
 
			if (req.has_param("title"))
			{
				String stitle = req.get_param_value("title");
			


				if (stitle.length() > 50)
				{
					setError(res, stitle + " is too long!");
					return;
				}

				auto sheetbinFile = File::getCurrentWorkingDirectory().getChildFile("tex").getChildFile(stitle+".bin");
				if (sheetbinFile.existsAsFile())
				{
				 
					 
							juce::MemoryBlock mb;
							sheetbinFile.loadFileAsData(mb);
							if (mb.getSize() > 0)
							{
								mb.getData();
								res.set_content_provider(
									mb.getSize(), // Content length
									[mb](uint64_t offset, uint64_t length, DataSink sink) {
									const auto &d = mb;
									sink(&mb[offset], std::min(mb.getSize(), mb.getSize()));
								},

									[mb] {});
							}
							else
							{
								setError(res, "file bin size is 0!");
								return;
							}
 
 
				}
				else
				{
					setError(res, "Can not find file bin !" + stitle);
					return;
				}

			}
			else
			{
				setError(res, "downloadsheetbytitle must contain title name!");
				return;
			}
		});
		m_svr.Get("/download", [&](const Request &req, Response &res) {
			log("wait download");
			std::lock_guard<std::mutex> lockGuard(_mtx_RWFile);
			log("enter download");
			if (req.has_param("os") && req.has_param("clientversion"))
			{
				String s_ostype = req.get_param_value("os");
				String s_version = req.get_param_value("clientversion");


				if (s_ostype != "osx"  && s_ostype != "win" && s_ostype != "linux")
				{
					setError(res, s_ostype + " is not supproted!");
					return;
				}

				auto filedir = File::getCurrentWorkingDirectory().getChildFile(s_ostype);
				if (filedir.isDirectory())
				{
					auto namef = filedir.getChildFile("name.txt");
					if (namef.existsAsFile())
					{
						String appname = namef.loadFileAsString();
						File appf = filedir.getChildFile(appname);
						if (appf.existsAsFile())
						{
							juce::MemoryBlock mb;
							appf.loadFileAsData(mb);
							if (mb.getSize() > 0)
							{
								mb.getData();
								res.set_content_provider(
									mb.getSize(), // Content length
									[mb](uint64_t offset, uint64_t length, DataSink sink) {
									const auto &d = mb;
									sink(&mb[offset], std::min(mb.getSize(), mb.getSize()));
								},

									[mb] {});
							}
						}
						else
						{
							setError(res,  "App file not exist!");
							
							return;
						}
					}
					else
					{
						setError(res, "Can not find name file!");
						return;
					}
				}
				else
				{
					setError(res,  "Can not find file directory!");
					return;
				}
			
			}
			else
			{
				setError(res, "Download must contain operator system and version!");
				return;
			}
		});

		m_svr.Get("/getsheetbytitle", [](const Request& req, Response& res) {

			if (!restrictAccess(req, res))
				return;
			if (req.has_param("name")/* username */ && req.has_param("title") && req.has_param("token"))
			{
				String name = req.get_param_value("name");
				String title = req.get_param_value("title");
				String token = req.get_param_value("token");
				


				if (name.length() > 50 || title.length() > 50 || token.length() > 50)
				{
					setError(res, "Get sheet parameter is two long!");
					return;
				}

				String author;
				String sheet;
				auto r = SheetDB::ins().getSheetByTitle(title, author, sheet);
				if (r == SheetDB::FindEnum::TREE_INVALID)
				{
					setError(res, "Internal DB Error!");
					return;
				}
				else if (r == SheetDB::FindEnum::INVALID_PASSWORD)
				{
					setError(res, "Sheet length < 3?");
					return;
				}
				else if (r == SheetDB::FindEnum::NOT_FIND_NAME)
				{
					setError(res, "Title not exist!");
					return;
				}
				else if (r == SheetDB::FindEnum::SUCCESS)
				{
					//
					var errObj(new DynamicObject());
					JSONHelper(errObj, "author", author);
					JSONHelper(errObj, "sheet", sheet);

					res.set_content(JSON::toString(errObj).toStdString(), "application/json; charset=utf-8");

					return;
				}

			}
			else
			{

				setError(res, "Wrong argument for get sheet api.");
				return;

			}

		});
		/*
		
		
					JSONHelper(errObj, "sheetdescribe", MemroyToHexString((juce::uint8*)(mb.getData()), mb.getSize()));
					JSONHelper(errObj, "sheettexture", MemroyToHexString((juce::uint8*)(mb.getData()), mb.getSize()));
					*/

		m_svr.Get("/getallsheet", [](const Request& req, Response& res) {

			if(!istest)
			if (!restrictAccess(req, res) )
				return;
			if (req.has_param("token") || istest)
			{
				String name = req.get_param_value("name");
			
				String token = req.get_param_value("token");

				if (name.length() > 50 || token.length() > 50)
				{
					setError(res, "Getallsheet parameter is two long!");
					return;
				}

				MemoryBlock mb;
					
				auto l = SheetDB::ins().getAllSheet(mb);

				
					var errObj(new DynamicObject());
					JSONHelper(errObj, "readLength", String(l));
					JSONHelper(errObj, "content", MemroyToHexString((juce::uint8*)(mb.getData()), mb.getSize()  ));
					
					res.set_content(JSON::toString(errObj).toStdString(), "application/json; charset=utf-8");
					//std::cout << "get content!" << std::endl;
					return;
			

			}
			else
			{

				setError(res, "Wrong argument for get sheet api.");
				return;

			}

		});


		m_svr.Get("/login", [](const Request& req, Response& res) {

			if (!restrictAccess(req, res))
				return;
			if (req.has_param("name") && req.has_param("pwd"))
			{
				String name = req.get_param_value("name");
				name = name.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

				String pwd = req.get_param_value("pwd");
				pwd = pwd.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");


				if (name.length() > 50 || pwd.length() > 50 )
				{
					setError(res, "Login parameter is two long!");
					return;
				}
				String pwdInDB;
				auto r = FileDB::ins().getUserPwdByName(name, pwdInDB);
				if (r == FileDB::FindEnum::TREE_INVALID)
				{
					setError(res, "Internal DB Error!");
					return;
				}
				else if (r == FileDB::FindEnum::INVALID_PASSWORD)
				{
					setError(res, "Password is not right!");
					return;
				}
				else if (r == FileDB::FindEnum::NOT_FIND_NAME)
				{
					setError(res, "User not exist!");
					return;
				}
				else if (r == FileDB::FindEnum::SUCCESS)
				{
					//
					if (pwdInDB == pwd)
					{
						setSuccess(res, "Login success!");
					}
					else
					{
						setError(res, "Wrong Password!");
					}
					
					return;
				}
		
			}
			else
			{

				setError(res, "Wrong argument for login api.");
				return;

			}

		});


		m_svr.Get("/reg", [](const Request& req, Response& res) {
		
			if (!restrictAccess( req, res))
				return;


			if (req.has_param("name") && req.has_param("pwd") && req.has_param("email"))
			{
			
				String name = req.get_param_value("name");
				name = name.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

				String pwd = req.get_param_value("pwd");
				pwd = pwd.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

				String email = req.get_param_value("email");

				if (name.length() > 50 || pwd.length() > 50 || email.length() > 50)
				{
					setError(res,  "Register parameter is two long!");
					return;
				}

				if (FileDB::ins().isUserExist(name))
				{
					setError(res, "Name Already exist!");
					return;
				}


				if (FileDB::ins().insertUser(name, pwd, email))
				{
					setSuccess(res, "Register success");
					return;
				}
				else
				{
					setError(res, "Internal DB error!");
					return;
				}

			}
			else
			{

				setError(res, "Wrong argument for login api.");

			}

		});


		m_svr.listen("0.0.0.0", 6060);
	
	});

	t.detach();
	getchar();
	std::cout << "Server is started!" <<std::endl;
	while (true)
	{
		
		juce::Time::waitForMillisecondCounter(5000);
	}

    return 0;
}
