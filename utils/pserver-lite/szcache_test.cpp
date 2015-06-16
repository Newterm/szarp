#include <iostream>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <cassert>
#include <stdexcept>

#include "szcache.h"

std::string time2str( time_t rtime)
{
	struct tm * timeinfo;
	char buff [100];
 	timeinfo = std::localtime(&rtime);
  	std::strftime(buff,100,"%c",timeinfo);
  	return std::string(buff);
}

void createSzCacheFile(const std::string& path, const std::vector<int16_t>& records)
{
	std::ofstream fs(path, std::ios::binary);
	if (fs.rdstate() & std::ifstream::failbit) 
		throw std::runtime_error("SzCacheFile Test: Failed to open file: " + path);

	std::for_each(records.begin(), records.end(), 
		[&] (const int16_t& val) {
			fs.write(reinterpret_cast<const char*>(&val), sizeof val);
		});
	fs.close();
}

int removeSzCacheFile(const std::string& path) 
{
	return std::remove(path.c_str());	
}

void setupTestTree()
{
	mkdir("./TestA",		0755);
	mkdir("./TestA/TestB",		0755);
	mkdir("./TestA/TestB/TestC",	0755);
}

void removeTestTree()
{
	rmdir("./TestA/TestB/TestC");
	rmdir("./TestA/TestB");
	rmdir("./TestA");
}

int main() {

	/** Setup test environ */

	setupTestTree();
	
	std::vector<int16_t> v201501, v201502, v201503;
	/** (1420070400) NO_DATA (1420074000) 75 (1420077600) NO_DATA (1422748800) */
	for (int i = 0; i < 267840; ++i) {
		if (i < 360 || i > 720) v201501.push_back(-32768);
		else v201501.push_back(75);
	}
	for (int i = 0; i < 241920; ++i) {
		if (i > 720 && i < 1080) v201502.push_back(-32768);
		else v201502.push_back(85);

	}
	for (int i = 0; i < 267840; ++i) {
		if (i < 360) v201503.push_back(-32768);
		else v201503.push_back(95);
		//v201503.push_back(-32768);
	}

	createSzCacheFile("TestA/TestB/TestC/201501.szc", v201501);
	createSzCacheFile("TestA/TestB/TestC/201502.szc", v201502);
	createSzCacheFile("TestA/TestB/TestC/201503.szc", v201503);

	/** Test */

	SzCache szc("./",2);

	/** availableRange() */
	SzCache::SzRange szr = szc.availableRange();
	std::cout << "SzCache::availableRange(): (" << szr.first << "," << szr.second << ")\n";
	std::cout << "= ( " << time2str(szr.first) << " , " << time2str(szr.second) << " )\n";
	std::cout << "\n";

	SzCache::SzTime t1(1420070400 );
	SzCache::SzTime t2(1420070400 + 3600);
	SzCache::SzTime t3(1420070400 + 3600 + 3600);
	SzCache::SzTime t4(1420070400 + 2678400);
	SzCache::SzTime t5(1420070400 + 2678400 + 3600 + 3600);
	SzCache::SzTime t6(1420070400 + 2678400 + 3600 + 3600 + 3600);
	SzCache::SzTime t7(1420070400 + 2678400 + 2419200);
	SzCache::SzTime t8(1420070400 + 2678400 + 2419200 + 3600);
	SzCache::SzPath szp1("TestA/TestB/TestC");

	try {
		SzCache::SzSearchResult ssr = szc.searchInPlace(t1,szp1);
		std::cout << "SzCache::searchInPlace("<<t1<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t2,szp1);
		std::cout << "SzCache::searchInPlace("<<t2<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != t2) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t2-10,szp1);
		std::cout << "SzCache::searchInPlace("<<t2-10<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t3,szp1);
		std::cout << "SzCache::searchInPlace("<<t3<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != t3) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t3+10,szp1);
		std::cout << "SzCache::searchInPlace("<<t3+10<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t4,szp1);
		std::cout << "SzCache::searchInPlace("<<t4<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != t4) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t5,szp1);
		std::cout << "SzCache::searchInPlace("<<t5<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != t5) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t5+10,szp1);
		std::cout << "SzCache::searchInPlace("<<t5+10<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t6,szp1);
		std::cout << "SzCache::searchInPlace("<<t6<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != t6) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t6-10,szp1);
		std::cout << "SzCache::searchInPlace("<<t6-10<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t7,szp1);
		std::cout << "SzCache::searchInPlace("<<t7<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t8,szp1);
		std::cout << "SzCache::searchInPlace("<<t8<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != t8) throw std::logic_error("Assertion failed!");

		ssr = szc.searchInPlace(t8-10,szp1);
		std::cout << "SzCache::searchInPlace("<<t8-10<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

		ssr = szc.searchRight(t2-50,t3,szp1); 
		std::cout << "SzCache::searcRight("<<t2-50<<","<<t3<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != t2) throw std::logic_error("Assertion failed!");

		ssr = szc.searchRight(t2+50,t4,szp1); 
		std::cout << "SzCache::searcRight("<<t2+50<<","<<t4<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != (t2+50)) throw std::logic_error("Assertion failed!");
		
		ssr = szc.searchRight(t3+10,t3+50,szp1); 
		std::cout << "SzCache::searcRight("<<t3+10<<","<<t3+50<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

		ssr = szc.searchRight(t4-100,t5,szp1); 
		std::cout << "SzCache::searcRight("<<t4-100<<","<<t5<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != t4) throw std::logic_error("Assertion failed!");

		ssr = szc.searchLeft(t7+100,t6,szp1); 
		std::cout << "SzCache::searchLeft("<<t7+100<<","<<t6<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != (t7-10)) throw std::logic_error("Assertion failed!");

		ssr = szc.searchLeft(t6+100,t5,szp1); 
		std::cout << "SzCache::searchLeft("<<t6+100<<","<<t5<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != (t6+100)) throw std::logic_error("Assertion failed!");

		ssr = szc.searchLeft(t3+100,t3+10,szp1); 
		std::cout << "SzCache::searchLeft("<<t3+100<<","<<t3+10<<","<<szp1<<"):\n"
			<< "= ( " << std::get<0>(ssr) << " , " << std::get<1>(ssr) << " , " << std::get<2>(ssr) << " )\n\n";
		if(std::get<0>(ssr) != -1) throw std::logic_error("Assertion failed!");

	} catch (std::logic_error& le) {
		removeSzCacheFile("TestA/TestB/TestC/201501.szc");
		removeSzCacheFile("TestA/TestB/TestC/201502.szc");
		removeSzCacheFile("TestA/TestB/TestC/201503.szc");
		removeTestTree();

		std::cout << "Assertion has failed!" << std::endl;
		return -1;
	}
		
	std::cout << "All assertions valid: Test PASSED" << std::endl;
	/** Remove test environ */

	removeSzCacheFile("TestA/TestB/TestC/201501.szc");
	removeSzCacheFile("TestA/TestB/TestC/201502.szc");
	removeSzCacheFile("TestA/TestB/TestC/201503.szc");
	removeTestTree();
	
	return 0;
}
